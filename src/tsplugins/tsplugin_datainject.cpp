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
//  Transport stream processor shared library:
//  DVB SimulCrypt data injector using EMMG/PDG <=> MUX protocol.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsOneShotPacketizer.h"
#include "tsEMMGMUX.h"
#include "tstlvConnection.h"
#include "tsTCPServer.h"
#include "tsUDPReceiver.h"
#include "tsMessageQueue.h"
#include "tstlvMessageFactory.h"
#include "tsThread.h"
TSDUCK_SOURCE;

#define DEFAULT_PACKET_QUEUE_SIZE 100  // Maximum number of TS packets in queue
#define SERVER_BACKLOG            1    // One connection at a time
#define SERVER_THREAD_STACK_SIZE  (128 * 1024)


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DataInjectPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        DataInjectPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // TS packets are passed from the server thread to the plugin thread using a message queue.
        typedef MessageQueue<TSPacket, Mutex> TSPacketQueue;

        // Message queues enqueue smart pointers to the message type.
        typedef TSPacketQueue::MessagePtr TSPacketPtr;

        // TCP listener thread.
        class TCPListener : public Thread
        {
        public:
            // Constructor.
            TCPListener(DataInjectPlugin* plugin);

            // Terminate the thread.
            void stop();

        private:
            DataInjectPlugin* const _plugin;
            TSP* const              _tsp;
            tlv::Connection<Mutex>  _client;

            // Invoked in the context of the server thread.
            virtual void main() override;

            // Inaccessible operations.
            TCPListener() = delete;
            TCPListener(const TCPListener&) = delete;
            TCPListener& operator=(const TCPListener&) = delete;
        };

        // UDP listener thread.
        class UDPListener : public Thread
        {
        public:
            // Constructor.
            UDPListener(DataInjectPlugin* plugin);

            // Open the UDP socket.
            bool open();

            // Terminate the thread.
            void stop();

        private:
            DataInjectPlugin* const _plugin;
            TSP* const              _tsp;
            UDPReceiver             _client;

            // Invoked in the context of the server thread.
            virtual void main() override;

            // Inaccessible operations.
            UDPListener() = delete;
            UDPListener(const UDPListener&) = delete;
            UDPListener& operator=(const UDPListener&) = delete;
        };

        // Plugin private data
        PacketCounter   _pkt_current;          // Current TS packet index
        PacketCounter   _pkt_next_data;        // Next data insertion point
        PID             _data_pid;             // PID for data (constant after start)
        uint8_t         _data_cc;              // Continuity counter in data PID.
        BitRate         _max_bitrate;          // Max data PID's bitrate (constant after start)
        SocketAddress   _server_address;       // TCP/UDP port and optional local address.
        bool            _reuse_port;           // Reuse port option.
        size_t          _sock_buf_size;        // Socket receive buffer size.
        TCPServer       _server;               // EMMG/PDG <=> MUX TCP server
        TCPListener     _tcp_listener;         // TCP listener thread.
        UDPListener     _udp_listener;         // UDP listener thread.
        TSPacketQueue   _queue;                // Queue of incoming TS packets
        volatile bool   _channel_established;  // Data channel open.
        volatile bool   _stream_established;   // Data stream open.
        volatile bool   _req_bitrate_changed;  // Requested bitrate has changed.
        // Start of protected area.
        Mutex           _mutex;                // Mutex for access to protected area
        uint32_t        _client_id;            // DVB SimilCrypt client id.
        uint16_t        _data_id;              // DVB SimilCrypt data id.
        bool            _section_mode;         // Datagrams are sections.
        BitRate         _req_bitrate;          // Requested bitrate
        size_t          _lost_packets;         // Lost packets (queue full)

        // Process bandwidth request. Invoked in the server thread.
        bool processBandwidthRequest(const tlv::MessagePtr&, emmgmux::StreamBWAllocation&);

        // Process data provision. Invoked in the server thread.
        bool processDataProvision(const tlv::MessagePtr&);

        // Enqueue a TS packet. Invoked in the server thread.
        bool enqueuePacket(const TSPacketPtr&);

        // Inaccessible operations
        DataInjectPlugin() = delete;
        DataInjectPlugin(const DataInjectPlugin&) = delete;
        DataInjectPlugin& operator=(const DataInjectPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(datainject, ts::DataInjectPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DataInjectPlugin::DataInjectPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"DVB SimulCrypt data injector using EMMG/PDG <=> MUX protocol.", u"[options]"),
    _pkt_current(0),
    _pkt_next_data(0),
    _data_pid(PID_NULL),
    _data_cc(0),
    _max_bitrate(0),
    _server_address(),
    _reuse_port(false),
    _sock_buf_size(0),
    _server(),
    _tcp_listener(this),
    _udp_listener(this),
    _queue(),
    _channel_established(false),
    _stream_established(false),
    _req_bitrate_changed(false),
    _mutex(),
    _client_id(0),
    _data_id(0),
    _section_mode(false),
    _req_bitrate(0),
    _lost_packets(0)
{
    option(u"bitrate-max",      'b', POSITIVE);
    option(u"buffer-size",       0,  UNSIGNED);
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
            u"  --buffer-size value\n"
            u"      Specify the TCP and UDP socket receive buffer size (socket option).\n"
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
    _reuse_port = present(u"reuse-port");
    _sock_buf_size = intValue<size_t>(u"buffer-size");

    // Specify which EMMG/PDG <=> MUX version to use.
    emmgmux::Protocol::Instance()->setVersion(intValue<tlv::VERSION>(u"emmg-mux-version", 2));

    // Initialize the TCP server.
    if (!_server_address.resolve(value(u"server"), *tsp)) {
        return false;
    }
    if (!_server.open(*tsp)) {
        return false;
    }
    if (!_server.reusePort(_reuse_port, *tsp) || !_server.bind(_server_address, *tsp) || !_server.listen(SERVER_BACKLOG, *tsp)) {
        _server.close(*tsp);
        return false;
    }

    // Initialize the UDP receiver.
    if (!_udp_listener.open()) {
        _server.close(*tsp);
        return false;
    }

    // Initial bandwidth allocation (zero means unlimited)
    _req_bitrate = _max_bitrate;
    _req_bitrate_changed = false;
    tsp->verbose(u"initial bandwidth allocation is %s", {_req_bitrate == 0 ? u"unlimited" : UString::Decimal(_req_bitrate) + u" b/s"});

    // TS processing state
    _data_cc = 0;
    _lost_packets = 0;
    _pkt_current = 0;
    _pkt_next_data = 0;

    // Start the internal threads.
    _channel_established = false;
    _stream_established = false;
    _tcp_listener.start();
    _udp_listener.start();

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::stop()
{
    // Stop the internal threads.
    _tcp_listener.stop();
    _udp_listener.stop();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DataInjectPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
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
    if (_req_bitrate_changed) {
        // Reinitialize insertion point when bitrate changes
        _pkt_next_data = _pkt_current;
        _req_bitrate_changed = false;
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
            Guard lock(_mutex);
            if (_req_bitrate != 0) {
                // TODO: refine this, works only for low injection bitrates.
                _pkt_next_data += tsp->bitrate() / _req_bitrate;
            }
        }
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Process bandwidth request. Invoked in the server thread
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::processBandwidthRequest(const tlv::MessagePtr& request, emmgmux::StreamBWAllocation& response)
{
    // Interpret the message as a stream_BW_request.
    emmgmux::StreamBWRequest* m = dynamic_cast<emmgmux::StreamBWRequest*>(request.pointer());
    if (m == 0) {
        tsp->error(u"incorrect message, expected stream_BW_request");
        return false;
    }

    // Check that the stream is established.
    if (!_stream_established) {
        tsp->error(u"unexpected stream_BW_request, stream not setup");
        return false;
    }

    Guard lock(_mutex);

    // Compute new bandwidth
    if (m->has_bandwidth) {
        BitRate requested = 1000 * BitRate(m->bandwidth); // protocol unit is kb/s
        _req_bitrate = _max_bitrate == 0 ? requested : std::min(requested, _max_bitrate);
        _req_bitrate_changed = true;
        tsp->verbose(u"requested bandwidth %'d b/s, allocated %'d b/s", {requested, _req_bitrate});
    }

    // Build the response
    response.channel_id = m->channel_id;
    response.stream_id = m->stream_id;
    response.client_id = m->client_id;
    response.has_bandwidth = _req_bitrate > 0;
    response.bandwidth = uint16_t(_req_bitrate / 1000); // protocol unit is kb/s
    return true;
}


//----------------------------------------------------------------------------
// Process data provision. Invoked in the server threads.
//----------------------------------------------------------------------------

bool ts::DataInjectPlugin::processDataProvision(const tlv::MessagePtr& msg)
{
    // Interpret the message as a stream_BW_request.
    emmgmux::DataProvision* m = dynamic_cast<emmgmux::DataProvision*>(msg.pointer());
    if (m == 0) {
        tsp->error(u"incorrect message, expected data_provision");
        return false;
    }

    // Check that the stream is established.
    if (!_stream_established) {
        tsp->error(u"unexpected data_provision, stream not setup");
        return false;
    }

    Guard lock(_mutex);

    // Check that the client and data id are expected.
    if (m->client_id != _client_id) {
        tsp->error(u"unexpected client id 0x%X in data_provision, expected 0x%X", {m->client_id, _client_id});
        return false;
    }
    if (m->data_id != _data_id) {
        tsp->error(u"unexpected data id 0x%X in data_provision, expected 0x%X", {m->data_id, _data_id});
        return false;
    }

    // Check that the stream is established.
    bool ok = true;
    if (_section_mode) {
        // Feed a packetizer with all sections (one section per datagram parameter)
        OneShotPacketizer pzer;
        for (size_t i = 0; i < m->datagram.size(); ++i) {
            SectionPtr sp(new Section(m->datagram[i]));
            if (sp->isValid()) {
                pzer.addSection(sp);
            }
            else {
                tsp->error(u"received an invalid section (%d bytes)", {m->datagram[i]->size()});
            }
        }
        // Extract all packets and enqueue them
        TSPacketVector pv;
        pzer.getPackets(pv);
        for (size_t i = 0; i < pv.size(); ++i) {
            ok = enqueuePacket(new TSPacket(pv[i])) && ok;
        }
    }
    else {
        // Packet mode, locate packets and enqueue them
        for (size_t i = 0; i < m->datagram.size(); ++i) {
            const uint8_t* data = m->datagram[i]->data();
            size_t size = m->datagram[i]->size();
            while (size >= PKT_SIZE) {
                if (*data != SYNC_BYTE) {
                    tsp->error(u"invalid TS packet");
                }
                else {
                    TSPacketPtr p(new TSPacket());
                    ::memcpy(p->b, data, PKT_SIZE);
                    ok = enqueuePacket(p) && ok;
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

bool ts::DataInjectPlugin::enqueuePacket(const TSPacketPtr& pkt)
{
    // Enqueue packet immediately or fail.
    const bool ok = _queue.enqueue(pkt, 0);

    Guard lock(_mutex);

    if (!ok && _lost_packets++ == 0) {
        tsp->warning(u"internal queue overflow, losing packets, consider using --queue-size");
    }
    else if (ok && _lost_packets != 0) {
        tsp->info(u"retransmitting after %'d lost packets", {_lost_packets});
        _lost_packets = 0;
    }

    return ok;
}


//----------------------------------------------------------------------------
// TCP listener thread.
//----------------------------------------------------------------------------

ts::DataInjectPlugin::TCPListener::TCPListener(DataInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin),
    _tsp(plugin->tsp),
    _client(emmgmux::Protocol::Instance(), true, 3)
{
}

void ts::DataInjectPlugin::TCPListener::stop()
{
    // Close the server, then break client connection.
    // This will force the server thread to terminate.
    _plugin->_server.close(*_tsp);
    _client.disconnect(NULLREP);
    _client.close(NULLREP);

    // Wait for actual thread termination
    Thread::waitForTermination();
}

void ts::DataInjectPlugin::TCPListener::main()
{
    _tsp->debug(u"TCP server thread started");

    SocketAddress client_address;
    emmgmux::ChannelStatus channel_status;
    emmgmux::StreamStatus stream_status;

    // Loop on client acceptance (accept only one client at a time).
    while (_plugin->_server.accept(_client, client_address, *_tsp)) {

        _tsp->verbose(u"incoming connection from %s", {client_address.toString()});

        // Connection state
        bool ok = true;
        tlv::MessagePtr msg;

        _plugin->_channel_established = false;
        _plugin->_stream_established = false;

        // Loop on message reception from the client
        while (ok && _client.receive(msg, _tsp, *_tsp)) {

            // Message handling.
            // We do not send errors back to client, we just disconnect
            // (not too polite, but we don't care!)
            switch (msg->tag()) {

                case emmgmux::Tags::channel_setup: {
                    if (_plugin->_channel_established) {
                        _tsp->error(u"received channel_setup when channel is already setup");
                        ok = false;
                    }
                    else {
                        emmgmux::ChannelSetup* m = dynamic_cast<emmgmux::ChannelSetup*>(msg.pointer());
                        assert (m != 0);
                        // Build and send the channel_status
                        channel_status.channel_id = m->channel_id;
                        channel_status.client_id = m->client_id;
                        channel_status.section_TSpkt_flag = m->section_TSpkt_flag;
                        ok = _client.send (channel_status, *_tsp);
                        Guard lock(_plugin->_mutex);
                        _plugin->_client_id = m->client_id;
                        _plugin->_channel_established = true;
                    }
                    break;
                }

                case emmgmux::Tags::channel_test: {
                    if (_plugin->_channel_established) {
                        // Automatic reply to channel_test
                        ok = _client.send(channel_status, *_tsp);
                    }
                    else {
                        _tsp->error(u"unexpected channel_test, channel not setup");
                        ok = false;
                    }
                    break;
                }

                case emmgmux::Tags::channel_close: {
                    _plugin->_channel_established = false;
                    _plugin->_stream_established = false;
                    break;
                }

                case emmgmux::Tags::stream_setup: {
                    if (!_plugin->_channel_established) {
                        _tsp->error(u"unexpected stream_setup, channel not setup");
                        ok = false;
                    }
                    else if (_plugin->_stream_established) {
                        _tsp->error(u"received stream_setup when stream is already setup");
                        ok = false;
                    }
                    else {
                        emmgmux::StreamSetup* m = dynamic_cast<emmgmux::StreamSetup*>(msg.pointer());
                        assert(m != 0);
                        // Build and send the stream_status
                        stream_status.channel_id = m->channel_id;
                        stream_status.stream_id = m->stream_id;
                        stream_status.client_id = m->client_id;
                        stream_status.data_id = m->data_id;
                        stream_status.data_type = m->data_type;
                        ok = _client.send(stream_status, *_tsp);
                        Guard lock(_plugin->_mutex);
                        _plugin->_data_id = m->data_id;
                        _plugin->_stream_established = true;
                    }
                    break;
                }

                case emmgmux::Tags::stream_test: {
                    if (_plugin->_stream_established) {
                        // Automatic reply to stream_test
                        ok = _client.send(stream_status, *_tsp);
                    }
                    else {
                        _tsp->error(u"unexpected stream_test, stream not setup");
                        ok = false;
                    }
                    break;
                }

                case emmgmux::Tags::stream_close_request: {
                    if (!_plugin->_stream_established) {
                        _tsp->error(u"unexpected stream_close_request, stream not setup");
                        ok = false;
                    }
                    else {
                        // Send the stream_close_response
                        emmgmux::StreamCloseResponse resp;
                        emmgmux::StreamCloseRequest* m = dynamic_cast<emmgmux::StreamCloseRequest*>(msg.pointer());
                        assert (m != 0);
                        resp.channel_id = m->channel_id;
                        resp.stream_id = m->stream_id;
                        resp.client_id = m->client_id;
                        ok = _client.send(resp, *_tsp);
                        _plugin->_stream_established = false;
                    }
                    break;
                }

                case emmgmux::Tags::stream_BW_request: {
                    emmgmux::StreamBWAllocation response;
                    ok = _plugin->processBandwidthRequest(msg, response) && _client.send(response, *_tsp);
                    break;
                }

                case emmgmux::Tags::data_provision: {
                    ok = _plugin->processDataProvision(msg);
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

    _tsp->debug(u"TCP server thread completed");
}


//----------------------------------------------------------------------------
// UDP listener thread.
//----------------------------------------------------------------------------

ts::DataInjectPlugin::UDPListener::UDPListener(DataInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin),
    _tsp(plugin->tsp),
    _client(*plugin->tsp)
{
}

bool ts::DataInjectPlugin::UDPListener::open()
{
    _client.setParameters(_plugin->_server_address, _plugin->_reuse_port, _plugin->_sock_buf_size);
    return _client.open(*_tsp);
}

void ts::DataInjectPlugin::UDPListener::stop()
{
    // Close the UDP receiver.
    // This will force the server thread to terminate.
    _client.close(NULLREP);

    // Wait for actual thread termination
    Thread::waitForTermination();
}

void ts::DataInjectPlugin::UDPListener::main()
{
    _tsp->debug(u"UDP server thread started");

    uint8_t inbuf[65536];
    size_t insize = 0;
    SocketAddress sender;
    SocketAddress destination;

    // Loop on incoming messages.
    while (_client.receive(inbuf, sizeof(inbuf), insize, sender, destination, _tsp, *_tsp)) {

        // Analyze the message
        tlv::MessageFactory mf(inbuf, insize, emmgmux::Protocol::Instance());
        const tlv::MessagePtr msg(mf.factory());

        if (mf.errorStatus() != tlv::OK || msg.isNull()) {
            _tsp->error(u"received invalid message from %s, %d bytes", {sender.toString(), insize});
        }
        else {
            // Log the message
            if (_tsp->debug()) {
                _tsp->debug(u"received message from %s\n%s", {sender.toString(), msg->dump(4)});
            }
            // The only accepted message is data_provision.
            _plugin->processDataProvision(msg);
        }
    }

    _tsp->debug(u"UDP server thread completed");
}
