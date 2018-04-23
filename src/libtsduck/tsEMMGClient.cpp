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

#include "tsEMMGClient.h"
#include "tsGuard.h"
#include "tsGuardCondition.h"
#include "tsOneShotPacketizer.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::EMMGClient::RECEIVER_STACK_SIZE;
const ts::MilliSecond ts::EMMGClient::RESPONSE_TIMEOUT;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::EMMGClient::EMMGClient() :
    Thread(ThreadAttributes().setStackSize(RECEIVER_STACK_SIZE)),
    _state(INITIAL),
    _total_bytes(0),
    _abort(0),
    _report(0),
    _connection(emmgmux::Protocol::Instance(), true, 3),
    _channel_status(),
    _stream_status(),
    _mutex(),
    _work_to_do(),
    _got_response(),
    _last_response(0),
    _allocated_bw(0),
    _error_status(),
    _error_info()
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::EMMGClient::~EMMGClient()
{
    {
        GuardCondition lock(_mutex, _work_to_do);

        // Break connection, if not already done
        _abort = 0;
        _report = NullReport::Instance();
        _connection.disconnect(NULLREP);
        _connection.close(NULLREP);

        // Notify receiver thread to terminate
        _state = DESTRUCTING;
        lock.signal();
    }
    waitForTermination();
}


//----------------------------------------------------------------------------
// Report specified error message if not empty, abort connection and return false
//----------------------------------------------------------------------------

bool ts::EMMGClient::abortConnection(const UString& message)
{
    if (!message.empty()) {
        _report->error(message);
    }

    GuardCondition lock(_mutex, _work_to_do);
    _state = DISCONNECTED;
    _connection.disconnect(*_report);
    _connection.close(*_report);
    lock.signal();

    return false;
}


//----------------------------------------------------------------------------
// Prepare and wait for response.
//----------------------------------------------------------------------------

void ts::EMMGClient::cleanupResponse()
{
    Guard lock(_mutex);
    _last_response = 0;
}

ts::tlv::TAG ts::EMMGClient::waitResponse()
{
    // Lock the mutex
    GuardCondition lock(_mutex, _got_response, RESPONSE_TIMEOUT);
    if (!lock.isLocked()) {
        // Timeout, no response.
        return 0;
    }

    while (_last_response == 0) {
        // Release the mutex and wait for response.
        // Automatically reacquire the mutex when condition is signaled.
        if (!lock.waitCondition(RESPONSE_TIMEOUT)) {
            // Timeout, no response.
            return 0;
        }
    }

    // Mutex still held
    return _last_response;
    // Automatically release mutex
}


//----------------------------------------------------------------------------
// Connect to a remote EMMG. Perform all initial channel and stream negotiation.
//----------------------------------------------------------------------------

bool ts::EMMGClient::connect(const SocketAddress& mux,
                             uint32_t client_id,
                             uint16_t data_channel_id,
                             uint16_t data_stream_id,
                             uint16_t data_id,
                             uint8_t data_type,
                             bool section_format,
                             emmgmux::ChannelStatus& channel_status,
                             emmgmux::StreamStatus& stream_status,
                             const AbortInterface* abort,
                             Report* report)
{
    // Initial state check
    {
        Guard lock(_mutex);
        // Start receiver thread if first time
        if (_state == INITIAL) {
            _state = DISCONNECTED;
            Thread::start();
        }
        if (_state != DISCONNECTED) {
            if (report != 0) {
                report->error(u"EMMG client already connected");
            }
            return false;
        }
        _abort = abort;
        _report = report != 0 ? report : NullReport::Instance();
    }

    // Perform TCP connection to EMMG server
    if (!_connection.open(*_report)) {
        return false;
    }
    if (!_connection.connect(mux, *_report)) {
        _connection.close(*_report);
        return false;
    }

    // Automatic response to channel_test.
    _channel_status.channel_id = data_channel_id;
    _channel_status.client_id = client_id;
    _channel_status.section_TSpkt_flag = !section_format;

    // Automatic response to stream_test.
    _stream_status.channel_id = data_channel_id;
    _stream_status.stream_id = data_stream_id;
    _stream_status.client_id = client_id;
    _stream_status.data_id = data_id;
    _stream_status.data_type = data_type;

    // Cleanup response state.
    cleanupResponse();

    // Send a channel_setup message to MUX
    emmgmux::ChannelSetup channel_setup;
    channel_setup.channel_id = data_channel_id;
    channel_setup.client_id = client_id;
    channel_setup.section_TSpkt_flag = !section_format;
    if (!_connection.send(channel_setup, *_report)) {
        return abortConnection();
    }

    // Tell the receiver thread to start listening for incoming messages
    {
        GuardCondition lock(_mutex, _work_to_do);
        _state = CONNECTING;
        lock.signal();
    }

    // Wait for a channel_status from the MUX.
    tlv::TAG response = waitResponse();
    if (response == 0) {
        return abortConnection(u"MUX channel_setup response timeout");
    }
    if (response != emmgmux::Tags::channel_status) {
        return abortConnection(UString::Format(u"unexpected response 0x%X from MUX (expected channel_status)", {response}));
    }

    // Cleanup response state.
    cleanupResponse();

    // Send a stream_setup message to MUX.
    emmgmux::StreamSetup stream_setup;
    stream_setup.channel_id = data_channel_id;
    stream_setup.stream_id = data_stream_id;
    stream_setup.client_id = client_id;
    stream_setup.data_id = data_id;
    stream_setup.data_type = data_type;
    if (!_connection.send(stream_setup, *_report)) {
        return abortConnection();
    }

    // Wait for a stream_status from the MUX
    response = waitResponse();
    if (response == 0) {
        return abortConnection(u"MUX stream_setup response timeout");
    }
    if (response != emmgmux::Tags::stream_status) {
        return abortConnection(UString::Format(u"unexpected response 0x%X from MUX (expected stream_status)", {response}));
    }

    // Data stream now established
    _total_bytes = 0;
    {
        Guard lock(_mutex);
        _state = CONNECTED;
    }

    return true;
}


//----------------------------------------------------------------------------
// Disconnect from remote MUX. Close stream and channel.
//----------------------------------------------------------------------------

bool ts::EMMGClient::disconnect()
{
    // Mark disconnection in progress
    State previous_state;
    {
        Guard lock(_mutex);
        previous_state = _state;
        if (_state == CONNECTING || _state == CONNECTED) {
            _state = DISCONNECTING;
        }
    }

    // Disconnection sequence
    bool ok = previous_state == CONNECTED;
    if (ok) {
        // Cleanup response state.
        cleanupResponse();

        // Politely send a stream_close_request and wait for a stream_close_response
        emmgmux::StreamCloseRequest req;
        req.channel_id = _stream_status.channel_id;
        req.stream_id = _stream_status.stream_id;
        req.client_id = _stream_status.client_id;
        ok = _connection.send(req, *_report) && waitResponse() == emmgmux::Tags::stream_close_response;
        
        // If we get a polite reply, send a channel_close
        if (ok) {
            emmgmux::ChannelClose cc;
            cc.channel_id = _channel_status.channel_id;
            cc.client_id = _channel_status.client_id;
            ok = _connection.send(cc, *_report);
        }
    }

    // TCP disconnection
    GuardCondition lock(_mutex, _work_to_do);
    if (previous_state == CONNECTING || previous_state == CONNECTED) {
        _state = DISCONNECTED;
        ok = _connection.disconnect(*_report) && ok;
        ok = _connection.close(*_report) && ok;
        lock.signal();
    }

    return ok;
}


//----------------------------------------------------------------------------
// Send a bandwidth request.
//----------------------------------------------------------------------------

bool ts::EMMGClient::requestBandwidth(uint16_t bandwidth, bool synchronous)
{
    // Cleanup response state.
    cleanupResponse();

    // Send a stream_BW_request message to MUX.
    emmgmux::StreamBWRequest request;
    request.channel_id = _stream_status.channel_id;
    request.stream_id = _stream_status.stream_id;
    request.client_id = _stream_status.client_id;
    request.has_bandwidth = true;
    request.bandwidth = bandwidth;
    if (!_connection.send(request, *_report)) {
        return false;
    }

    // In asynchronous mode, we are done.
    if (!synchronous) {
        return true;
    }

    // Wait for a response from the MUX in synchronous mode.
    tlv::TAG response = waitResponse();
    switch (response) {
        case 0:
            _report->error(u"MUX stream_BW_request response timeout");
            return false;
        case emmgmux::Tags::channel_error:
        case emmgmux::Tags::stream_error:
            // Explicit error.
            return false;
        case emmgmux::Tags::stream_BW_allocation:
            // Valid response.
            return true;
        default:
            _report->error(u"unexpected response 0x%X from MUX (expected stream_status)", {response});
            return false;
    }
}


//----------------------------------------------------------------------------
// Get the last allocated bandwidth as returned by the MUX.
//----------------------------------------------------------------------------

uint16_t ts::EMMGClient::allocatedBandwidth()
{
    Guard lock(_mutex);
    return _allocated_bw;
}


//----------------------------------------------------------------------------
// Send data provision.
//----------------------------------------------------------------------------

bool ts::EMMGClient::dataProvision(const ByteBlockPtr& data)
{
    std::vector<ByteBlockPtr> chunks(1, data);
    return dataProvision(chunks);
}

bool ts::EMMGClient::dataProvision(const void* data, size_t size)
{
    return dataProvision(new ByteBlock(data, size));
}

bool ts::EMMGClient::dataProvision(const std::vector<ByteBlockPtr>& data)
{
    // Build a data provision message.
    emmgmux::DataProvision request;
    request.channel_id = _stream_status.channel_id;
    request.stream_id = _stream_status.stream_id;
    request.client_id = _stream_status.client_id;
    request.data_id = _stream_status.data_id;
    request.datagram = data;

    // Eliminate null pointers, count total data bytes.
    for (auto it = request.datagram.begin(); it != request.datagram.end(); ) {
        if (it->isNull()) {
            it = request.datagram.erase(it);
        }
        else {
            _total_bytes += (*it)->size();
            ++it;
        }
    }

    return _connection.send(request, *_report);
}


//----------------------------------------------------------------------------
// Send data provision in section format.
//----------------------------------------------------------------------------

bool ts::EMMGClient::dataProvision(const SectionPtrVector& sections)
{
    if (_channel_status.section_TSpkt_flag) {
        // Send data in TS packet format, packetize the sections.
        ts::OneShotPacketizer zer;
        zer.addSections(sections);

        ts::TSPacketVector packets;
        zer.getPackets(packets);

        return dataProvision(packets.data(), packets.size() * PKT_SIZE);
    }
    else {
        // Send data in section format.
        std::vector<ByteBlockPtr> chunks;
        for (size_t i = 0; i < sections.size(); ++i) {
            if (!sections[i].isNull()) {
                chunks.push_back(new ByteBlock(sections[i]->content(), sections[i]->size()));
            }
        }
        return dataProvision(chunks);
    }
}

//----------------------------------------------------------------------------
// Get the last error response.
//----------------------------------------------------------------------------

void ts::EMMGClient::getLastErrorResponse(std::vector<uint16_t>& error_status, std::vector<uint16_t>& error_information)
{
    Guard lock(_mutex);
    error_status = _error_status;
    error_information = _error_info;
}


//----------------------------------------------------------------------------
// Receiver thread main code
//----------------------------------------------------------------------------

void ts::EMMGClient::main()
{
    // Main loop
    for (;;) {

        TS_UNUSED const AbortInterface* abort = 0;
        Report* report = 0;

        // Wait for a connection to be managed
        {
            // Lock the mutex, get object state
            GuardCondition lock(_mutex, _work_to_do);
            while (_state == DISCONNECTED) {
                // Release the mutex and wait for something to do.
                // Automatically reacquire the mutex when condition is signaled.
                lock.waitCondition();
            }
            // Mutex still held, check if thread must terminate
            if (_state == DESTRUCTING) {
                return;
            }
            // Get abort and report handler
            abort = _abort;
            report = _report;
            // Automatically release mutex
        }

        // Loop on message reception
        tlv::MessagePtr msg;
        bool ok = true;
        while (ok && _connection.receive(msg, _abort, *report)) {
            // Is this kind of response worth reporting to the application?
            bool reportResponse = true;

            switch (msg->tag()) {
                case emmgmux::Tags::channel_test: {
                    // Automatic reply to channel_test
                    reportResponse = false;
                    ok = _connection.send(_channel_status, *report);
                    break;
                }
                case emmgmux::Tags::stream_test: {
                    // Automatic reply to stream_test
                    reportResponse = false;
                    ok = _connection.send(_stream_status, *report);
                    break;
                }
                case emmgmux::Tags::stream_BW_allocation: {
                    // Store returned bandwidth.
                    emmgmux::StreamBWAllocation* const resp = dynamic_cast<emmgmux::StreamBWAllocation*>(msg.pointer());
                    assert(resp != 0);
                    {
                        Guard lock(_mutex);
                        _allocated_bw = resp->has_bandwidth ? resp->bandwidth : 0;
                    }
                    break;
                }
                case emmgmux::Tags::stream_error: {
                    // Store returned error.
                    emmgmux::StreamError* const resp = dynamic_cast<emmgmux::StreamError*>(msg.pointer());
                    assert(resp != 0);
                    {
                        Guard lock(_mutex);
                        _error_status = resp->error_status;
                        _error_info = resp->error_information;
                    }
                    break;
                }
                case emmgmux::Tags::channel_error: {
                    // Store returned error.
                    emmgmux::ChannelError* const resp = dynamic_cast<emmgmux::ChannelError*>(msg.pointer());
                    assert(resp != 0);
                    {
                        Guard lock(_mutex);
                        _error_status = resp->error_status;
                        _error_info = resp->error_information;
                    }
                    break;
                }
                default: {
                    // Nothing to do on other messages.
                    break;
                }
            }

            // Notify application thread that a response has arrived.
            if (reportResponse) {
                GuardCondition lock(_mutex, _got_response);
                _last_response = msg->tag();
                lock.signal();
            }
        }

        // Error while receiving messages, most likely a disconnection
        {
            Guard lock(_mutex);
            if (_state == DESTRUCTING) {
                return;
            }
            if (_state != DISCONNECTED) {
                _state = DISCONNECTED;
                _connection.disconnect(NULLREP);
                _connection.close(NULLREP);
            }
        }
    }
}
