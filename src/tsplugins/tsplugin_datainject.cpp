//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Transport stream processor shared library:
//  DVB SimulCrypt data injector using EMMG/PDG <=> MUX protocol.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsOneShotPacketizer.h"
#include "tsEMMGMUX.h"
#include "tstlvConnection.h"
#include "tsTCPServer.h"
#include "tsMessageQueue.h"
#include "tsDoubleCheckLock.h"
#include "tsThread.h"
TSDUCK_SOURCE;

#define DEFAULT_PACKET_QUEUE_SIZE 100  // Maximum number of TS packets in queue
#define SERVER_BACKLOG            1    // One connection at a time
#define SERVER_THREAD_STACK_SIZE  (128 * 1024)


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DataInjectPlugin: public ProcessorPlugin, private Thread
    {
    public:
        // Implementation of plugin API
        DataInjectPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        typedef MessageQueue<TSPacket, Mutex> TSPacketQueue;
        typedef TSPacketQueue::MessagePtr TSPacketPtr;

        // Plugin private data
        PacketCounter   _pkt_current;      // Current TS packet index
        PacketCounter   _pkt_next_data;    // Next data insertion point
        PID             _data_pid;         // PID for data (constant after start)
        uint8_t         _data_cc;          // Continuity counter in data PID.
        BitRate         _max_bitrate;      // Max data PID's bitrate (constant after start)
        BitRate         _req_bitrate;      // Requested bitrate (used by plugin thread only)
        BitRate         _req_bitrate_prot; // Protected reference version of _req_bitrate
                                           // (reader: plugin thread, writer: server thread)
        DoubleCheckLock _req_bitrate_lock; // Lock for _req_bitrate_prot
        size_t          _lost_packets;     // Lost packets (queue full, used by server thread only)
        TSPacketQueue   _queue;            // Queue of incoming TS packets
        TCPServer       _server;           // EMMG/PDG <=> MUX TCP server
        tlv::Connection<Mutex> _client;    // Connection with EMMG/PDG client

        // Invoked in the context of the server thread.
        virtual void main() override;

        // Process bandwidth request. Invoked in the server thread.
        // Return true on success, false on error.
        bool processBandwidthRequest(const emmgmux::StreamBWRequest&);

        // Process data provision. Invoked in the server thread.
        // Return true on success, false on error.
        bool processDataProvision(const emmgmux::DataProvision&, bool section_mode);

        // Enqueue a TS packet. Invoked in the server thread.
        // Return true on success, false on error.
        bool enqueuePacket(const TSPacketPtr&);

        // Inaccessible operations
        DataInjectPlugin() = delete;
        DataInjectPlugin(const DataInjectPlugin&) = delete;
        DataInjectPlugin& operator=(const DataInjectPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::DataInjectPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DataInjectPlugin::DataInjectPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"DVB SimulCrypt data injector using EMMG/PDG <=> MUX protocol.", u"[options]"),
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _pkt_current(0),
    _pkt_next_data(0),
    _data_pid(PID_NULL),
    _data_cc(0),
    _max_bitrate(0),
    _req_bitrate(0),
    _req_bitrate_prot(0),
    _req_bitrate_lock(),
    _lost_packets(0),
    _queue(),
    _server(),
    _client(emmgmux::Protocol::Instance(), true, 3)
{
    option(u"bitrate-max",      'b', POSITIVE);
    option(u"emmg-mux-version", 'v', INTEGER, 0, 1, 2, 3);
    option(u"pid",              'p', PIDVAL, 1, 1);
    option(u"queue-size",       'q', UINT32);
    option(u"reuse-port",       'r');
    option(u"server",           's', STRING, 1, 1);

    setHelp(u"Options:\n"
            u"\n"
            u"  -b value\n"
            u"  --bitrate-max value\n"
            u"      Specifies the maximum bitrate for the data PID in bits / second.\n"
            u"      By default, the data PID bitrate is limited by the stuffing bitrate\n"
            u"      (data insertion is performed by replacing stuffing packets).\n"
            u"\n"
            u"  -v value\n"
            u"  --emmg-mux-version value\n"
            u"      Specifies the version of the EMMG/PDG <=> MUX DVB SimulCrypt protocol.\n"
            u"      Valid values are 2 and 3. The default is 2.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      Specifies the PID for the data insertion. This option is mandatory.\n"
            u"\n"
            u"  -q value\n"
            u"  --queue-size value\n"
            u"      Specifies the maximum number of data TS packets in the internal queue,\n"
            u"      ie. packets which are received from the EMMG/PDG client but not yet\n"
            u"      inserted into the TS. The default is " TS_USTRINGIFY(DEFAULT_PACKET_QUEUE_SIZE) u".\n"
            u"\n"
            u"  -r\n"
            u"  --reuse-port\n"
            u"      Set the \"reuse port\" (or \"reuse address\") TCP option on the server.\n"
            u"\n"
            u"  -s [address:]port\n"
            u"  --server [address:]port\n"
            u"      Specifies the local TCP port on which the plugin listens for an incoming\n"
            u"      EMMG/PDG connection. This option is mandatory.\n"
            u"      When present, the optional address shall specify a local IP address or\n"
            u"      host name (by default, the plugin accepts connections on any local IP\n"
            u"      interface). This plugin behaves as a MUX, ie. a TCP server, and accepts\n"
            u"      only one EMMG/PDG connection at a time.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::start()
{
    // Command line options
    _max_bitrate = intValue<BitRate>(u"bitrate-max", 0);
    _data_pid = intValue<PID>(u"pid");
    _queue.setMaxMessages(intValue<size_t>(u"queue-size", DEFAULT_PACKET_QUEUE_SIZE));

    // Specify which EMMG/PDG <=> MUX version to use.
    emmgmux::Protocol::Instance()->setVersion(intValue<tlv::VERSION>(u"emmg-mux-version", 2));

    // Initialize the TCP server
    SocketAddress server_address;
    if (!server_address.resolve(value(u"server"), *tsp)) {
        return false;
    }
    if (!_server.open(*tsp)) {
        return false;
    }
    if (!_server.reusePort(present(u"reuse-port"), *tsp) || !_server.bind(server_address, *tsp) || !_server.listen(SERVER_BACKLOG, *tsp)) {
        _server.close(*tsp);
        return false;
    }

    // Initial bandwidth allocation (zero means unlimited)
    _req_bitrate = _max_bitrate;
    _req_bitrate_prot = _max_bitrate;
    tsp->verbose(u"initial bandwidth allocation is %s", {_req_bitrate == 0 ? u"unlimited" : UString::Decimal(_req_bitrate) + u" b/s"});

    // TS processing state
    _data_cc = 0;
    _lost_packets = 0;
    _pkt_current = 0;
    _pkt_next_data = 0;

    // Start the internal thread.
    Thread::start();

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::stop()
{
    // Close the server, then break client connection.
    // This will force the server thread to terminate.
    _server.close(*tsp);
    _client.disconnect(NULLREP);
    _client.close(NULLREP);

    // Wait for actual thread termination
    Thread::waitForTermination();

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DataInjectPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Count packets
    _pkt_current++;


    // Abort if data PID is already present in TS
    const PID pid = pkt.getPID();
    if (pid == _data_pid) {
        tsp->error(u"data PID conflict, specified %d (0x%X), now found as input PID, try another one", {pid, pid});
        return TSP_END;
    }

    // Data injection may occur only be replacing null packets
    if (pid != PID_NULL) {
        return TSP_OK;
    }

    // Update data PID bitrate
    if (_req_bitrate_lock.changed()) {
        DoubleCheckLock::Reader guard(_req_bitrate_lock);
        _req_bitrate = _req_bitrate_prot;
        // Reinitialize insertion point when bitrate changes
        _pkt_next_data = _pkt_current;
    }

    // Try to insert data
    if (_pkt_next_data <= _pkt_current) {
        // Time to insert data packet, if any is available immediately.
        TSPacketPtr pp;
        if (_queue.dequeue(pp, 0)) {
            // Update data packet
            pkt = *pp;
            // Update PID and continuity counter.
            pkt.setPID(_data_pid);
            pkt.setCC(_data_cc);
            _data_cc = (_data_cc + 1) & CC_MASK;
            // Compute next insertion point if the data PID bitrate is specified.
            // Otherwise, try to update any null packet (unbounded bitrate).
            if (_req_bitrate != 0) {
                _pkt_next_data += tsp->bitrate() / _req_bitrate;
            }
        }
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::DataInjectPlugin::main()
{
    tsp->debug(u"server thread started");

    SocketAddress client_address;
    emmgmux::ChannelStatus channel_status;
    emmgmux::StreamStatus stream_status;

    // Loop on client acceptance
    while (_server.accept (_client, client_address, *tsp)) {

        tsp->verbose(u"incoming connection from %s", {client_address.toString()});

        // Connection state
        bool ok = true;
        bool channel_ok = false;
        bool stream_ok = false;
        tlv::MessagePtr msg;

        // Loop on message reception from the client
        while (ok && _client.receive(msg, tsp, *tsp)) {

            // Message handling.
            // We do not send errors back to client, we just disconnect
            // (not too polite, but we don't care!)
            switch (msg->tag()) {

                case emmgmux::Tags::channel_setup: {
                    if (channel_ok) {
                        tsp->error(u"received channel_setup when channel is already setup");
                        ok = false;
                    }
                    else {
                        emmgmux::ChannelSetup* m = dynamic_cast <emmgmux::ChannelSetup*> (msg.pointer());
                        assert (m != 0);
                        // Build and send the channel_status
                        channel_status.channel_id = m->channel_id;
                        channel_status.client_id = m->client_id;
                        channel_status.section_TSpkt_flag = m->section_TSpkt_flag;
                        ok = _client.send (channel_status, *tsp);
                        channel_ok = true;
                    }
                    break;
                }

                case emmgmux::Tags::channel_test: {
                    if (channel_ok) {
                        // Automatic reply to channel_test
                        ok = _client.send (channel_status, *tsp);
                    }
                    else {
                        tsp->error(u"unexpected channel_test, channel not setup");
                        ok = false;
                    }
                    break;
                }

                case emmgmux::Tags::channel_close: {
                    channel_ok = false;
                    stream_ok = false;
                    break;
                }

                case emmgmux::Tags::stream_setup: {
                    if (!channel_ok) {
                        tsp->error(u"unexpected stream_setup, channel not setup");
                        ok = false;
                    }
                    else if (stream_ok) {
                        tsp->error(u"received stream_setup when stream is already setup");
                        ok = false;
                    }
                    else {
                        emmgmux::StreamSetup* m = dynamic_cast <emmgmux::StreamSetup*> (msg.pointer());
                        assert (m != 0);
                        // Build and send the stream_status
                        stream_status.channel_id = m->channel_id;
                        stream_status.stream_id = m->stream_id;
                        stream_status.client_id = m->client_id;
                        stream_status.data_id = m->data_id;
                        stream_status.data_type = m->data_type;
                        ok = _client.send (stream_status, *tsp);
                        stream_ok = true;
                    }
                    break;
                }

                case emmgmux::Tags::stream_test: {
                    if (stream_ok) {
                        // Automatic reply to stream_test
                        ok = _client.send(stream_status, *tsp);
                    }
                    else {
                        tsp->error(u"unexpected stream_test, stream not setup");
                        ok = false;
                    }
                    break;
                }

                case emmgmux::Tags::stream_close_request: {
                    if (!stream_ok) {
                        tsp->error(u"unexpected stream_close_request, stream not setup");
                        ok = false;
                    }
                    else {
                        // Send the stream_close_response
                        emmgmux::StreamCloseResponse resp;
                        emmgmux::StreamCloseRequest* m = dynamic_cast <emmgmux::StreamCloseRequest*> (msg.pointer());
                        assert (m != 0);
                        resp.channel_id = m->channel_id;
                        resp.stream_id = m->stream_id;
                        resp.client_id = m->client_id;
                        ok = _client.send (resp, *tsp);
                        stream_ok = false;
                    }
                    break;
                }

                case emmgmux::Tags::stream_BW_request: {
                    if (!stream_ok) {
                        tsp->error(u"unexpected stream_BW_request, stream not setup");
                        ok = false;
                    }
                    else {
                        emmgmux::StreamBWRequest* m = dynamic_cast <emmgmux::StreamBWRequest*> (msg.pointer());
                        assert (m != 0);
                        ok = processBandwidthRequest (*m);
                    }
                    break;
                }

                case emmgmux::Tags::data_provision: {
                    if (!stream_ok) {
                        tsp->error(u"unexpected data_provision, stream not setup");
                        ok = false;
                    }
                    else {
                        emmgmux::DataProvision* m = dynamic_cast <emmgmux::DataProvision*> (msg.pointer());
                        assert(m != 0);
                        ok = processDataProvision(*m, channel_status.section_TSpkt_flag == 0);
                    }
                    break;
                }

                default: {
                    break;
                }
            }
        }

        // Error while receiving messages during a client session, most likely a disconnection
        _client.disconnect(NULLREP);
        _client.close(NULLREP);
    }

    tsp->debug(u"server thread completed");
}


//----------------------------------------------------------------------------
// Process bandwidth request. Invoked in the server thread
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::processBandwidthRequest(const emmgmux::StreamBWRequest& request)
{
    // Compute new bandwidth
    if (request.has_bandwidth) {
        BitRate requested = 1000 * BitRate (request.bandwidth); // protocol unit is kb/s
        {
            DoubleCheckLock::Writer guard(_req_bitrate_lock);
            _req_bitrate_prot = _max_bitrate == 0 ? requested : std::min(requested, _max_bitrate);
        }
        tsp->verbose(u"requested bandwidth %'d b/s, allocated %'d b/s", {requested, _req_bitrate_prot});
    }

    // Send the response
    emmgmux::StreamBWAllocation resp;
    resp.channel_id = request.channel_id;
    resp.stream_id = request.stream_id;
    resp.client_id = request.client_id;
    resp.has_bandwidth = _req_bitrate_prot > 0;
    resp.bandwidth = uint16_t (_req_bitrate_prot / 1000); // protocol unit is kb/s
    return _client.send (resp, *tsp);
}


//----------------------------------------------------------------------------
// Process data provision. Invoked in the server thread.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::processDataProvision (const emmgmux::DataProvision& msg, bool section_mode)
{
    bool ok = true;

    if (section_mode) {
        // Feed a packetizer with all section (one section per datagram parameter)
        OneShotPacketizer pzer;
        for (size_t i = 0; i < msg.datagram.size(); ++i) {
            SectionPtr sp (new Section (msg.datagram[i]));
            if (sp->isValid()) {
                pzer.addSection (sp);
            }
            else {
                tsp->error(u"received an invalid section (%d bytes)", {msg.datagram[i]->size()});
            }
        }
        // Extract all packets and enqueue them
        TSPacketVector pv;
        pzer.getPackets (pv);
        for (size_t i = 0; i < pv.size(); ++i) {
            ok = enqueuePacket (new TSPacket (pv[i])) && ok;
        }
    }
    else {
        // Packet mode, locate packets and enqueue them
        for (size_t i = 0; i < msg.datagram.size(); ++i) {
            const uint8_t* data = msg.datagram[i]->data();
            size_t size = msg.datagram[i]->size();
            while (size >= PKT_SIZE) {
                if (*data != SYNC_BYTE) {
                    tsp->error(u"invalid TS packet");
                }
                else {
                    TSPacketPtr p (new TSPacket());
                    ::memcpy (p->b, data, PKT_SIZE);
                    ok = enqueuePacket (p) && ok;
                    data += PKT_SIZE;
                    size -= PKT_SIZE;
                }
            }
            if (size != 0) {
                tsp->error(u"extraneous %d bytes in datagram", {size});
            }
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
// Enqueue a TS packet. Invoked in the server thread.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::enqueuePacket (const TSPacketPtr& pkt)
{
    // Enqueue packet immediately or fail.
    const bool ok = _queue.enqueue(pkt, 0);

    if (!ok && _lost_packets++ == 0) {
        tsp->warning(u"internal queue overflow, losing packets, consider using --queue-size");
    }
    else if (ok && _lost_packets != 0) {
        tsp->info(u"retransmitting after %'d lost packets", {_lost_packets});
        _lost_packets = 0;
    }

    return ok;
}
