//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB SimulCrypt data injector plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSection.h"
#include "tsPacketizer.h"
#include "tsEMMGMUX.h"
#include "tsTLVStream.h"
#include "tsTCPServer.h"
#include "tsUDPReceiver.h"
#include "tsMessageQueue.h"
#include "tsContinuityAnalyzer.h"
#include "tsThread.h"

namespace ts {
    //!
    //! DVB SimulCrypt data injector plugin for tsp, using EMMG/PDG <=> MUX protocol.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL DataInjectPlugin: public ProcessorPlugin, private SectionProviderInterface
    {
        TS_PLUGIN_CONSTRUCTORS(DataInjectPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // TS packets or sections are passed from the server thread to the plugin thread using a message queue.
        using PacketQueue = MessageQueue<TSPacket>;
        using SectionQueue = MessageQueue<Section>;

        // Message queues enqueue smart pointers to the message type.
        using PacketPtr = PacketQueue::MessagePtr;
        using SectionPtr = SectionQueue::MessagePtr;

        static constexpr tlv::VERSION DEFAULT_PROTOCOL_VERSION = 2;  // Default protocol version for EMMG/PDG <=> MUX.
        static constexpr size_t DEFAULT_QUEUE_SIZE = 1000;           // Maximum number of TS packets in queue
        static constexpr size_t SERVER_BACKLOG = 1;                  // One connection at a time
        static constexpr size_t SERVER_THREAD_STACK_SIZE = 128 * 1024;

        // TCP listener thread.
        class TCPListener: public Thread
        {
            TS_NOBUILD_NOCOPY(TCPListener);
        public:
            // Constructor and destructor.
            TCPListener(DataInjectPlugin* plugin);
            virtual ~TCPListener() override;

            // Terminate the thread.
            void stop();

        protected:
            // Invoked in the context of the server thread.
            virtual void main() override;

        private:
            DataInjectPlugin* const _plugin;
            Report        _report {Severity::Info, UString(), _plugin};
            TCPConnection _tcp_client {_plugin};
            TLVStream     _tlv_client {_plugin->_logger, _plugin->_protocol, _tcp_client, true, 3};
        };

        // UDP listener thread.
        class UDPListener: public Thread
        {
            TS_NOBUILD_NOCOPY(UDPListener);
        public:
            // Constructor and destructor.
            UDPListener(DataInjectPlugin* plugin);
            virtual ~UDPListener() override;

            // Open the UDP socket.
            bool open();

            // Terminate the thread.
            void stop();

        protected:
            // Invoked in the context of the server thread.
            virtual void main() override;

        private:
            DataInjectPlugin* const _plugin;
            Report      _report {Severity::Info, UString(), _plugin};
            UDPReceiver _client {&_report};
        };

        // Plugin private data
        emmgmux::Protocol  _protocol {};                     // EMMG/PDG <=> MUX protocol instance
        PacketCounter      _pkt_next_data = 0;               // Next data insertion point
        PID                _data_pid = PID_NULL;             // PID for data (constant after start)
        ContinuityAnalyzer _cc_fixer {AllPIDs(), this};      // To fix continuity counters in injected PID
        BitRate            _max_bitrate = 0;                 // Max data PID's bitrate (constant after start)
        bool               _unregulated = false;             // Insert data packet as soon as received.
        IPSocketAddress    _tcp_address {};                  // TCP port and optional local address.
        IPSocketAddress    _udp_address {};                  // UDP port and optional local address.
        bool               _reuse_port = false;              // Reuse port option.
        size_t             _sock_buf_size = 0;               // Socket receive buffer size.
        tlv::Logger        _logger {this, Severity::Debug};  // Message logger.
        TCPServer          _server {this};                   // EMMG/PDG <=> MUX TCP server
        TCPListener        _tcp_listener {this};             // TCP listener thread.
        UDPListener        _udp_listener {this};             // UDP listener thread.
        PacketQueue        _packet_queue {};                 // Queue of incoming TS packets.
        SectionQueue       _section_queue {};                // Queue of incoming sections.
        volatile bool      _channel_established = false;     // Data channel open.
        volatile bool      _stream_established = false;      // Data stream open.
        volatile bool      _req_bitrate_changed = false;     // Requested bitrate has changed.
        // Start of protected area.
        std::mutex         _mutex {};                        // Mutex for access to protected area
        uint32_t           _client_id = 0;                   // DVB SimilCrypt client id.
        uint16_t           _data_id = 0;                     // DVB SimilCrypt data id.
        bool               _section_mode = false;            // Datagrams are sections.
        Packetizer         _packetizer {duck, PID_NULL, this}; // Generate packets in the case of incoming sections.
        BitRate            _req_bitrate = 0;                 // Requested bitrate
        size_t             _lost_packets = 0;                // Lost packets (queue full)

        // Reset all client session context information.
        void clearSession();

        // Process bandwidth request. Invoked in the server thread.
        bool processBandwidthRequest(const tlv::MessagePtr&, emmgmux::StreamBWAllocation&);

        // Process data provision. Invoked in the server thread.
        bool processDataProvision(const tlv::MessagePtr&);

        // Report packet/session loss. Invoked with _mutex held.
        void processPacketLoss(const UChar* type, bool enqueueSuccess);

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override { return false; }
    };
}
