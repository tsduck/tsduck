//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to extract MPE (Multi-Protocol Encapsulation) datagrams.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsMPEDemux.h"
#include "tsUDPSocket.h"

namespace ts {
    //!
    //! Plugin to extract MPE (Multi-Protocol Encapsulation) datagrams.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL MPEPlugin: public ProcessorPlugin, private MPEHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(MPEPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Identification of a UDP stream.
        class StreamId
        {
        public:
            StreamId() = default;
            bool operator<(const StreamId&) const;
            PID             mpe_pid = PID_NULL;
            IPSocketAddress source {};
            IPSocketAddress destination {};
        };

        // Characteristics of a UDP string.
        class StreamData
        {
        public:
            StreamData() = default;
            uint64_t total_bytes = 0;
            uint64_t datagram_count = 0;
        };

        // Command line options.
        bool          _log = false;            // Log MPE datagrams.
        bool          _sync_layout = false;    // Display a layout of 0x47 sync bytes.
        bool          _dump_datagram = false;  // Dump complete network datagrams.
        bool          _dump_udp = false;       // Dump UDP payloads.
        bool          _send_udp = false;       // Send all datagrams through UDP.
        bool          _log_hexa_line = false;  // Log datagrams as one hexa line in the system message log.
        bool          _signal_event = false;   // Signal a plugin event on MPE packet.
        bool          _all_mpe_pids = false;   // Extract all MPE PID's.
        bool          _summary = false;        // Display a final summary.
        bool          _outfile_append = false; // Append file.
        fs::path      _outfile_name {};        // Output file name.
        UString       _log_hexa_prefix {};     // Prefix before hexa log line.
        PacketCounter _max_datagram = 0;       // Maximum number of datagrams to extract.
        size_t        _min_net_size = 0;       // Minimum size of network datagrams.
        size_t        _max_net_size = 0;       // Maximum size of network datagrams.
        size_t        _min_udp_size = 0;       // Minimum size of UDP datagrams.
        size_t        _max_udp_size = 0;       // Maximum size of UDP datagrams.
        size_t        _dump_max = 0;           // Max dump size in bytes.
        size_t        _skip_size = 0;          // Initial bytes to skip for --dump and --output-file.
        uint32_t      _event_code = 0;         // Event code to signal.
        int           _ttl = 0;                // Time to live option.
        PIDSet        _pids {};                // Explicitly specified PID's to extract.
        IPSocketAddress _ip_source {};         // IP source filter.
        IPSocketAddress _ip_dest {};           // IP destination filter.
        IPSocketAddress _ip_forward {};        // Forwarded socket address.
        IPAddress       _local_address {};     // Local IP address for UDP forwarding.
        uint16_t        _local_port = IPAddress::AnyPort; // Local UDP source port for UDP forwarding.

        // Plugin private fields.
        bool          _abort = false;          // Error, abort asap.
        UDPSocket     _sock {this};            // Outgoing UDP socket (forwarded datagrams).
        int           _previous_uc_ttl = 0;    // Previous unicast TTL which was set.
        int           _previous_mc_ttl = 0;    // Previous multicast TTL which was set.
        PacketCounter _datagram_count = 0;     // Number of extracted datagrams.
        std::ofstream _outfile {};             // Output file for extracted datagrams.
        MPEDemux      _demux {duck, this};     // MPE demux to extract MPE datagrams.
        std::map<StreamId, StreamData> _streams {}; // Collected data on UDP streams.

        // Inherited methods.
        virtual void handleMPENewPID(MPEDemux&, const PMT&, PID) override;
        virtual void handleMPEPacket(MPEDemux&, const MPEPacket&) override;

        // Build the string for --sync-layout.
        UString syncLayoutString(const uint8_t* udp, size_t udpSize);
    };
}
