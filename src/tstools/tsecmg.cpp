//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Minimal generic DVB SimulCrypt compliant ECMG for CAS head-end integration.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsAsyncReport.h"
#include "tsFatal.h"
#include "tsMutex.h"
#include "tsThread.h"
#include "tsECMGSCS.h"
#include "tsTCPServer.h"
#include "tstlvConnection.h"
#include "tsVariable.h"
#include "tsOneShotPacketizer.h"
#include "tsVersionInfo.h"
TSDUCK_SOURCE;

namespace {
    // Command line default arguments.
    static const uint16_t DEFAULT_SERVER_PORT       = 2222;
    static const uint16_t DEFAULT_REPETITION        = 100;
    static const int16_t  DEFAULT_DELAY_START       = -200;
    static const int16_t  DEFAULT_DELAY_STOP        = -200;
    static const int16_t  DEFAULT_TRANS_DELAY_START = -500;
    static const int16_t  DEFAULT_TRANS_DELAY_STOP  = 0;

    // Stack size for execution of the client connection thread
    static const size_t CLIENT_STACK_SIZE = 128 * 1024;

    // Instantiation of a TCP connection in a multi-thread context for TLV messages.
    typedef ts::tlv::Connection<ts::Mutex> ECMGConnection;
    typedef ts::SafePtr<ECMGConnection, ts::Mutex> ECMGConnectionPtr;
}


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct ECMGOptions: public ts::Args
{
    ECMGOptions(int argc, char *argv[]);

    bool                       once;           // Accept only one client.
    ts::SocketAddress          serverAddress;  // TCP server local address.
    ts::ecmgscs::ChannelStatus channelStatus;  // Standard parameters required by this ECMG.
    ts::ecmgscs::StreamStatus  streamStatus;   // Standard parameters required by this ECMG.
};

ECMGOptions::ECMGOptions(int argc, char *argv[]) :
    ts::Args(u"Minimal generic DVB SimulCrypt-compliant ECMG.", u"[options]"),
    once(false),
    serverAddress(),
    channelStatus(),
    streamStatus()
{
    option(u"ac-delay-start",         0,  INT16);
    option(u"ac-delay-stop",          0,  INT16);
    option(u"cw-per-ecm",            'c', INTEGER, 0, 1, 1, 255);
    option(u"delay-start",            0,  INT16);
    option(u"delay-stop",             0,  INT16);
    option(u"ecmg-scs-version",       0,  INTEGER, 0, 1, 2, 3);
    option(u"once",                  'o');
    option(u"port",                  'p', UINT16);
    option(u"repetition",            'r', UINT16);
    option(u"section-mode",          's');
    option(u"transition-delay-start", 0,  INT16);
    option(u"transition-delay-stop",  0,  INT16);

    setHelp(u"Options:\n"
            u"\n"
            u"  --ac-delay-start value\n"
            u"      This option sets the DVB SimulCrypt option 'AC_delay_start', in\n"
            u"      milliseconds. By default, use the same value as --delay-start.\n"
            u"\n"
            u"  --ac-delay-stop value\n"
            u"      This option sets the DVB SimulCrypt option 'AC_delay_stop', in\n"
            u"      milliseconds. By default, use the same value as --delay-stop.\n"
            u"\n"
            u"  -c value\n"
            u"  --cw-per-ecm value\n"
            u"      Specify the required number of control words per ECM. This option sets\n"
            u"      the DVB SimulCrypt option 'CW_per_msg'. It also set 'lead_CW' to\n"
            u"      'CW_per_msg' - 1. By default, use 2 control words per ECM, the current\n"
            u"      one and next one.\n"
            u"\n"
            u"  --delay-start value\n"
            u"      This option sets the DVB SimulCrypt option 'delay_start', in milliseconds.\n"
            u"      Default: " + ts::UString::Decimal(DEFAULT_DELAY_START, 0, true, u"") + u" ms.\n"
            u"\n"
            u"  --delay-stop value\n"
            u"      This option sets the DVB SimulCrypt option 'delay_stop', in milliseconds.\n"
            u"      Default: " + ts::UString::Decimal(DEFAULT_DELAY_STOP, 0, true, u"") + u" ms.\n"
            u"\n"
            u"  --ecmg-scs-version value\n"
            u"      Specifies the version of the ECMG <=> SCS DVB SimulCrypt protocol.\n"
            u"      Valid values are 2 and 3. The default is 2.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -o\n"
            u"  --once\n"
            u"      Accept only one client and exit at the end of the session.\n"
            u"\n"
            u"  -p value\n"
            u"  --port value\n"
            u"      TCP port number of the ECMG server. Default: " + ts::UString::Decimal(DEFAULT_SERVER_PORT, 0, true, u"") + u".\n"
            u"\n"
            u"  -r value\n"
            u"  --repetition value\n"
            u"      This option sets the DVB SimulCrypt option 'ECM_rep_period', the requested\n"
            u"      repetition period of ECM's, in milliseconds. Default: " + ts::UString::Decimal(DEFAULT_REPETITION, 0, true, u"") + u" ms.\n"
            u"\n"
            u"  -s\n"
            u"  --section-mode\n"
            u"      Return ECM's in section format. This option sets the DVB SimulCrypt\n"
            u"      parameter 'section_TSpkt_flag' to zero. By default, ECM's are returned\n"
            u"      in TS packet format.\n"
            u"\n"
            u"  --transition-delay-start value\n"
            u"      This option sets the DVB SimulCrypt option 'transition_delay_start', in\n"
            u"      milliseconds. Default: " + ts::UString::Decimal(DEFAULT_TRANS_DELAY_START, 0, true, u"") + u" ms.\n"
            u"\n"
            u"  --transition-delay-stop value\n"
            u"      This option sets the DVB SimulCrypt option 'transition_delay_stop', in\n"
            u"      milliseconds. Default: " + ts::UString::Decimal(DEFAULT_TRANS_DELAY_STOP, 0, true, u"") + u" ms.\n"
            u"\n"
            u"  -v\n"
            u"  --verbose\n"
            u"      Produce verbose output.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    analyze(argc, argv);

    serverAddress.setPort(intValue<uint16_t>(u"port", DEFAULT_SERVER_PORT));
    once = present(u"once");
    const ts::tlv::VERSION protocolVersion = intValue<ts::tlv::VERSION>(u"ecmg-scs-version", 2);
    channelStatus.section_TSpkt_flag = !present(u"section-mode");
    channelStatus.CW_per_msg = intValue<uint8_t>(u"cw-per-ecm", 2);
    channelStatus.lead_CW = channelStatus.CW_per_msg - 1;
    channelStatus.ECM_rep_period = intValue<uint16_t>(u"repetition", DEFAULT_REPETITION);
    channelStatus.delay_start = intValue<int16_t>(u"delay-start", DEFAULT_DELAY_START);
    channelStatus.delay_stop = intValue<int16_t>(u"delay-stop", DEFAULT_DELAY_STOP);
    channelStatus.has_AC_delay_start = true;
    channelStatus.AC_delay_start = intValue<int16_t>(u"ac-delay-start", channelStatus.delay_start);
    channelStatus.has_AC_delay_stop = true;
    channelStatus.AC_delay_stop = intValue<int16_t>(u"ac-delay-stop", channelStatus.delay_stop);
    channelStatus.has_transition_delay_start = true;
    channelStatus.transition_delay_start = intValue<int16_t>(u"transition-delay-start", DEFAULT_TRANS_DELAY_START);
    channelStatus.has_transition_delay_stop = true;
    channelStatus.transition_delay_stop = intValue<int16_t>(u"transition-delay-stop", DEFAULT_TRANS_DELAY_STOP);

    // Specify which ECMG <=> SCS version to use.
    ts::ecmgscs::Protocol::Instance()->setVersion(protocolVersion);
    channelStatus.forceProtocolVersion(protocolVersion);
    streamStatus.forceProtocolVersion(protocolVersion);

    // Other hard-coded ECMG parameters.
    channelStatus.max_streams = 0;       // No specified max number of streams per channel.
    channelStatus.max_comp_time = 1;     // ECM computation time in ms, very fast here, no crypto.
    channelStatus.min_CP_duration = 10;  // Minimum crypto period in 100 x ms, 1 second here.
    streamStatus.access_criteria_transfer_mode = false;  // We don't really need access criteria.

    exitOnError();
}


//----------------------------------------------------------------------------
// A class implementing the ECMG shared data, used from all threads.
//----------------------------------------------------------------------------

class ECMGSharedData : public ts::AsyncReport
{
public:
    // Constructor.
    ECMGSharedData(const ECMGOptions& opt);

    // Declare a new ECM_channel_id. Return false if already active.
    bool openChannel(uint16_t id);

    // Release a ECM_channel_id. Return false if not active.
    bool closeChannel(uint16_t id);

private:
    ts::Mutex          _mutex;     // Protect shared data.
    std::set<uint16_t> _channels;  // Active channels.
};


//----------------------------------------------------------------------------
// Implementation of ECMGSharedData.
//----------------------------------------------------------------------------

// Constructor.
ECMGSharedData::ECMGSharedData(const ECMGOptions& opt) :
    ts::AsyncReport(opt.maxSeverity()),
    _mutex(),
    _channels()
{
}

// Declare a new ECM_channel_id. Return false if already active.
bool ECMGSharedData::openChannel(uint16_t id)
{
    ts::Guard lock(_mutex);
    const bool ok = _channels.count(id) == 0;
    _channels.insert(id);
    return ok;
}

// Release a ECM_channel_id. Return false if not active.
bool ECMGSharedData::closeChannel(uint16_t id)
{
    ts::Guard lock(_mutex);
    const bool ok = _channels.count(id) != 0;
    _channels.erase(id);
    return ok;
}


//----------------------------------------------------------------------------
// A class implementing a thread which manages a client connection.
//----------------------------------------------------------------------------

class ECMGClient: public ts::Thread
{
public:
    // Constructor.
    // When deleteWhenTerminated is true, this object is automatically deleted
    // when the thread terminates.
    ECMGClient(const ECMGOptions& opt, const ECMGConnectionPtr& conn, ECMGSharedData* shared, bool deleteWhenTerminated);

    // Main code of the thread.
    virtual void main() override;

private:
    const ECMGOptions&          _opt;
    ECMGSharedData*             _shared;
    ECMGConnectionPtr           _conn;
    ts::UString                 _peer;
    ts::Variable<uint16_t>      _channel;  // Current channel id.
    std::map<uint16_t,uint16_t> _streams;  // Map of current stream id => ECM id.

    // Handle the various ECMG client messages.
    bool handleChannelSetup(ts::ecmgscs::ChannelSetup* msg);
    bool handleChannelTest(ts::ecmgscs::ChannelTest* msg);
    bool handleChannelClose(ts::ecmgscs::ChannelClose* msg);
    bool handleStreamSetup(ts::ecmgscs::StreamSetup* msg);
    bool handleStreamTest(ts::ecmgscs::StreamTest* msg);
    bool handleStreamCloseRequest(ts::ecmgscs::StreamCloseRequest* msg);
    bool handleCWProvision(ts::ecmgscs::CWProvision* msg);

    // Send a response message.
    bool send(const ts::tlv::Message* msg);

    // Send an error related to the msg.
    bool sendErrorResponse(const ts::tlv::Message* msg, uint16_t errorStatus);

    // Format a timestamp.
    static ts::UString TimeStamp()
    {
        return ts::Time::CurrentLocalTime().format(ts::Time::DATE | ts::Time::TIME);
    }

    // Deleted operations.
    ECMGClient(const ECMGClient&) = delete;
    ECMGClient& operator=(const ECMGClient&) = delete;
};


//----------------------------------------------------------------------------
// ECMG client constructor.
//----------------------------------------------------------------------------

ECMGClient::ECMGClient(const ECMGOptions& opt, const ECMGConnectionPtr& conn, ECMGSharedData* shared, bool deleteWhenTerminated) :
    ts::Thread(),
    _opt(opt),
    _shared(shared),
    _conn(conn),
    _peer(),
    _channel(),
    _streams()
{
    // Set thread attributes. Beware of deleteWhenTerminated...
    ts::ThreadAttributes attr;
    attr.setStackSize(CLIENT_STACK_SIZE);
    attr.setDeleteWhenTerminated(deleteWhenTerminated);
    setAttributes(attr);
}


//----------------------------------------------------------------------------
// Main code of the client connection thread.
//----------------------------------------------------------------------------

void ECMGClient::main()
{
    _peer = _conn->peerName();
    _shared->verbose(u"%s: %s: session started", {_peer, TimeStamp()});

    // Normally, an ECMG should handle incoming and outgoing messages independently.
    // However, here we have a minimal implementation. We never send any request to
    // the client and the ECM generation is instantaneous. So, we simply wait for
    // requests from the client and respond to them immediately.

    // Loop on message reception
    ts::tlv::MessagePtr msg;
    bool ok = true;
    while (ok && _conn->receive(msg, 0, *_shared)) {
        // Display received messages.
        _shared->verbose(u"%s: %s: received message:\n%s", {_peer, TimeStamp(), msg->dump(4)});

        switch (msg->tag()) {
            case ts::ecmgscs::Tags::channel_setup:
                ok = handleChannelSetup(dynamic_cast<ts::ecmgscs::ChannelSetup*>(msg.pointer()));
                break;
            case ts::ecmgscs::Tags::channel_test:
                ok = handleChannelTest(dynamic_cast<ts::ecmgscs::ChannelTest*>(msg.pointer()));
                break;
            case ts::ecmgscs::Tags::channel_close:
                ok = handleChannelClose(dynamic_cast<ts::ecmgscs::ChannelClose*>(msg.pointer()));
                break;
            case ts::ecmgscs::Tags::stream_setup:
                ok = handleStreamSetup(dynamic_cast<ts::ecmgscs::StreamSetup*>(msg.pointer()));
                break;
            case ts::ecmgscs::Tags::stream_test:
                ok = handleStreamTest(dynamic_cast<ts::ecmgscs::StreamTest*>(msg.pointer()));
                break;
            case ts::ecmgscs::Tags::stream_close_request:
                ok = handleStreamCloseRequest(dynamic_cast<ts::ecmgscs::StreamCloseRequest*>(msg.pointer()));
                break;
            case ts::ecmgscs::Tags::CW_provision:
                ok = handleCWProvision(dynamic_cast<ts::ecmgscs::CWProvision*>(msg.pointer()));
                break;
            case ts::ecmgscs::Tags::channel_status:
            case ts::ecmgscs::Tags::stream_status:
            case ts::ecmgscs::Tags::channel_error:
            case ts::ecmgscs::Tags::stream_error:
                // Silently ignore unsollicited status or error messages.
                break;
            default:
                // Received an invalid message for ECMG.
                ok = sendErrorResponse(msg.pointer(), ts::ecmgscs::Errors::inv_message);
                break;
        }
    }

    // Error while receiving or sending messages, most likely a client disconnection.
    _conn->disconnect(NULLREP);
    _conn->close(*_shared);

    // Make sure to release the channel if not done by the clients.
    if (_channel.set()) {
        _shared->closeChannel(_channel.value());
        _channel.reset();
    }

    _shared->verbose(u"%s: %s: session completed", {_peer, TimeStamp()});
}


//----------------------------------------------------------------------------
// Send a response message.
//----------------------------------------------------------------------------

bool ECMGClient::send(const ts::tlv::Message* msg)
{
    // Display messages.
    _shared->verbose(u"%s: %s: sending message:\n%s", {_peer, TimeStamp(), msg->dump(4)});
    return _conn->send(*msg, *_shared);
}


//----------------------------------------------------------------------------
// Send an error related to the msg.
//----------------------------------------------------------------------------

bool ECMGClient::sendErrorResponse(const ts::tlv::Message* msg, uint16_t errorStatus)
{
    const ts::tlv::ChannelMessage* channelMsg = 0;
    const ts::tlv::StreamMessage* streamMsg = 0;
    ts::ecmgscs::ChannelError channelError;
    ts::ecmgscs::StreamError streamError;
    ts::tlv::Message* resp = 0;

    // Build the appropriate response.
    if ((streamMsg = dynamic_cast<const ts::tlv::StreamMessage*>(msg)) != 0) {
        // Response to a stream message.
        streamError.channel_id = streamMsg->channel_id;
        streamError.stream_id = streamMsg->stream_id;
        streamError.error_status.push_back(errorStatus);
        resp = &streamError;
    }
    else if ((channelMsg = dynamic_cast<const ts::tlv::ChannelMessage*>(msg)) != 0) {
        // Response to a channel message.
        channelError.channel_id = channelMsg->channel_id;
        channelError.error_status.push_back(errorStatus);
        resp = &channelError;
    }
    else {
        // Response to garbage.
        channelError.channel_id = 0;
        channelError.error_status.push_back(errorStatus);
        resp = &channelError;
    }

    // Send the response.
    return send(resp);
}


//----------------------------------------------------------------------------
// Handle the various types of messages from the client.
//----------------------------------------------------------------------------

bool ECMGClient::handleChannelSetup(ts::ecmgscs::ChannelSetup* msg)
{
    assert(msg != 0);
    if (_channel.set()) {
        // Channel already set in this session.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::inv_channel_id);
    }
    else if (!_shared->openChannel(msg->channel_id)) {
        // Channel id already in use.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::channel_id_in_use);
    }
    else {
        // Channel accepted.
        _channel = msg->channel_id;
        ts::ecmgscs::ChannelStatus resp(_opt.channelStatus);
        resp.channel_id = msg->channel_id;
        return send(&resp);
    }
}


bool ECMGClient::handleChannelTest(ts::ecmgscs::ChannelTest* msg)
{
    assert(msg != 0);
    if (_channel != msg->channel_id) {
        // Not the right channel.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::inv_channel_id);
    }
    else {
        // Channel ok.
        _channel = msg->channel_id;
        ts::ecmgscs::ChannelStatus resp(_opt.channelStatus);
        resp.channel_id = msg->channel_id;
        return send(&resp);
    }
}


bool ECMGClient::handleChannelClose(ts::ecmgscs::ChannelClose* msg)
{
    assert(msg != 0);
    if (_channel != msg->channel_id) {
        // Not the right channel.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::inv_channel_id);
    }
    else {
        // Channel ok, close everything, no response expected.
        _shared->closeChannel(msg->channel_id);
        _channel.reset();
        _streams.clear();
        return true;
    }
}


bool ECMGClient::handleStreamSetup(ts::ecmgscs::StreamSetup* msg)
{
    assert(msg != 0);
    if (_channel != msg->channel_id) {
        // Not the right channel.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::inv_channel_id);
    }
    else if (_streams.count(msg->stream_id) != 0) {
        // Stream already in use in this channel.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::stream_id_in_use);
    }
    else {
        // Stream ok.
        _streams[msg->stream_id] = msg->ECM_id;
        ts::ecmgscs::StreamStatus resp(_opt.streamStatus);
        resp.channel_id = msg->channel_id;
        resp.stream_id = msg->stream_id;
        resp.ECM_id = msg->ECM_id;
        return send(&resp);
    }
}


bool ECMGClient::handleStreamTest(ts::ecmgscs::StreamTest* msg)
{
    assert(msg != 0);
    if (_channel != msg->channel_id) {
        // Not the right channel.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::inv_channel_id);
    }
    else if (_streams.count(msg->stream_id) == 0) {
        // Stream not in use in this channel.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::inv_stream_id);
    }
    else {
        // Stream ok.
        ts::ecmgscs::StreamStatus resp(_opt.streamStatus);
        resp.channel_id = msg->channel_id;
        resp.stream_id = msg->stream_id;
        resp.ECM_id = _streams[msg->stream_id];
        return send(&resp);
    }
}


bool ECMGClient::handleStreamCloseRequest(ts::ecmgscs::StreamCloseRequest* msg)
{
    assert(msg != 0);
    if (_channel != msg->channel_id) {
        // Not the right channel.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::inv_channel_id);
    }
    else if (_streams.count(msg->stream_id) == 0) {
        // Stream not in use in this channel.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::inv_stream_id);
    }
    else {
        // Stream ok, close it.
        _streams.erase(msg->stream_id);
        ts::ecmgscs::StreamCloseResponse resp;
        resp.channel_id = msg->channel_id;
        resp.stream_id = msg->stream_id;
        return send(&resp);
    }
}


bool ECMGClient::handleCWProvision(ts::ecmgscs::CWProvision* msg)
{
    assert(msg != 0);
    if (_channel != msg->channel_id) {
        // Not the right channel.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::inv_channel_id);
    }
    else if (_streams.count(msg->stream_id) == 0) {
        // Stream not in use in this channel.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::inv_stream_id);
    }
    else if (msg->CP_CW_combination.size() != _opt.channelStatus.CW_per_msg) {
        // Not the right number of CW in the request.
        return sendErrorResponse(msg, ts::ecmgscs::Errors::not_enough_CW);
    }
    else {
        // Start to build the response.
        ts::ecmgscs::ECMResponse resp;
        resp.channel_id = msg->channel_id;
        resp.stream_id = msg->stream_id;
        resp.CP_number = msg->CP_number;

        // Build the ECM section payload.
        ts::ByteBlock ecm;
        ecm.appendUInt16(ts::tlv::MSG_ECM);
        ecm.appendUInt16(0); // placeholder for size

        // Add all CW's in the ECM (in the clear, yeah, but that's a fake/test ECMG).
        for (auto it = msg->CP_CW_combination.begin(); it != msg->CP_CW_combination.end(); ++it) {
            if (it->CP < msg->CP_number || it->CP > msg->CP_number + _opt.channelStatus.lead_CW) {
                // Incorrect CP/CW combination.
                return sendErrorResponse(msg, ts::ecmgscs::Errors::not_enough_CW);
            }
            ecm.appendUInt16((it->CP & 0x01) == 0 ? ts::tlv::PRM_CW_EVEN : ts::tlv::PRM_CW_ODD);
            ecm.appendUInt16(uint16_t(it->CW.size()));
            ecm.append(it->CW);
        }

        // Add optional access criteria in ECM.
        if (msg->has_access_criteria) {
            ecm.appendUInt16(ts::tlv::PRM_ACCESS_CRITERIA);
            ecm.appendUInt16(uint16_t(msg->access_criteria.size()));
            ecm.append(msg->access_criteria);
        }

        // Update length in message TLV.
        ts::PutUInt16(ecm.data() + 2, uint16_t(ecm.size() - 4));

        // Compute the table id for the ECM, 0x80 or 0x81. There are two incompatible possibilities.
        // First method is to copy the parity of the crypto period number. Second method is to
        // alternate between the two, request after request in the stream. There is no requirement
        // that the table id has the same parity as the CP. However, it is safe to do it just in
        // case some CAS relies on it. On the other hand, if the SCS sends non-consecutive CP
        // numbers, it is possible that two adjacent CP have the same parity. Anyway, since there
        // is no perfect solution, we use the first one since it is simpler.
        const ts::TID tid = ts::TID(ts::TID_ECM_80 | (msg->CP_number & 0x01));

        // Build the ECM section.
        ts::SectionPtr ecmSection(new ts::Section(tid, true, ecm.data(), ecm.size()));

        // Format ECM for the response message.
        if (_opt.channelStatus.section_TSpkt_flag) {
            // Send ECM as TS packets, packetize the section.
            ts::TSPacketVector ecmPackets;
            ts::OneShotPacketizer zer;
            zer.addSection(ecmSection);
            zer.getPackets(ecmPackets);
            if (!ecmPackets.empty()) {
                resp.ECM_datagram.copy(ecmPackets[0].b, ecmPackets.size() * ts::PKT_SIZE);
            }
        }
        else {
            // Send ECM as a section.
            resp.ECM_datagram.copy(ecmSection->content(), ecmSection->size());
        }

        return send(&resp);
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    TSDuckLibCheckVersion();
    ECMGOptions opt(argc, argv);

    // IP initialization.
    if (!ts::IPInitialize(opt)) {
        return EXIT_FAILURE;
    }

    // Create ECMG shared data (including the asynchronous report).
    ECMGSharedData shared(opt);

    // Initialize a TCP server.
    ts::TCPServer server;
    if (!server.open(shared) ||
        !server.reusePort(true, shared) ||
        !server.bind(opt.serverAddress, shared) ||
        !server.listen(5, shared))
    {
        return EXIT_FAILURE;
    }
    shared.verbose(u"TCP server listening on %s, using ECMG <=> SCS protocol version %d",
                   {opt.serverAddress.toString(), ts::ecmgscs::Protocol::Instance()->version()});

    // Manage incoming client connections.
    for (;;) {

        // Accept one incoming connection.
        ts::SocketAddress clientAddress;
        ECMGConnectionPtr conn(new ECMGConnection(ts::ecmgscs::Protocol::Instance(), true, 3));
        ts::CheckNonNull(conn.pointer());
        if (!server.accept(*conn, clientAddress, shared)) {
            break;
        }

        // Process the connection.
        if (opt.once) {
            // If --once is specified, run once in the context of the main thread and exit.
            ECMGClient client(opt, conn, &shared, false);
            client.main();
            break;
        }
        else {
            // Otherwise, create a thread and forget about it.
            // The thread will deallocate itself automatically when it completes.
            ECMGClient* client = new ECMGClient(opt, conn, &shared, true);
            ts::CheckNonNull(client);
            client->start();
        }
    }

    return EXIT_SUCCESS;
}
