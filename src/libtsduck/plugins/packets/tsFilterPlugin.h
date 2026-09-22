//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to filter TS packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSignalizationDemux.h"

namespace ts {
    //!
    //! Plugin to filter TS packets.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL FilterPlugin: public ProcessorPlugin, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(FilterPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Packet intervals and list of them.
        using PacketRange = std::pair<PacketCounter, PacketCounter>;
        using PacketRangeList = std::list<PacketRange>;

        // Optional ranges of PCR, PTS, DTS values.
        using ClockRange = std::optional<std::pair<uint64_t, uint64_t>>;

        // Command line options:
        PacketProcessStatus _drop_status = TSP_DROP;    // Return status for unselected packets
        int                _scrambling_ctrl = 0;        // Scrambling control value (<0: no filter)
        bool               _need_demux = false;         // Need the help of the signalization demux.
        bool               _with_payload = false;       // Packets with payload
        bool               _with_af = false;            // Packets with adaptation field
        bool               _with_pes = false;           // Packets with clear PES headers
        bool               _with_pcr = false;           // Packets with PCR or OPCR
        bool               _with_pts = false;           // Packets with PTS in clear PES header
        bool               _with_dts = false;           // Packets with DTS in clear PES header
        bool               _with_splice = false;        // Packets with splice_countdown in adaptation field
        bool               _unit_start = false;         // Packets with payload unit start
        bool               _intra_frame = false;        // Packets with start of video intra-frame
        bool               _nullified = false;          // Packets which were nullified by a previous plugin
        bool               _input_stuffing = false;     // Null packets which were artificially inserted
        bool               _valid = false;              // Packets with valid sync byte and error ind
        bool               _negate = false;             // Negate filter (exclude selected packets)
        bool               _video = false;              // Part of a video PID
        bool               _audio = false;              // Part of an audio PID
        bool               _subtitles = false;          // Part of a subtitles PID
        bool               _ecm = false;                // Part of an ECM PID
        bool               _emm = false;                // Part of an EMM PID
        bool               _psi = false;                // Part of global PSI/SI PID.
        int                _min_payload = 0;            // Minimum payload size (<0: no filter)
        int                _max_payload = 0;            // Maximum payload size (<0: no filter)
        int                _min_af = 0;                 // Minimum adaptation field size (<0: no filter)
        int                _max_af = 0;                 // Maximum adaptation field size (<0: no filter)
        int                _splice = 0;                 // Exact splice_countdown value (<-128: no filter)
        int                _min_splice = 0;             // Minimum splice_countdown value (<-128: no filter)
        int                _max_splice = 0;             // Maximum splice_countdown value (<-128: no filter)
        PacketCounter      _after_packets = 0;          // Number of initial packets to skip
        PacketCounter      _every_packets = 0;          // Filter 1 out of this number of packets
        CodecType          _codec = CodecType::UNDEFINED; // Filter on codec type
        PIDSet             _explicit_pid {};            // Explicit PID values to filter
        ByteBlock          _pattern {};                 // Byte pattern to search.
        bool               _search_payload = false;     // Search pattern in payload only
        bool               _use_search_offset = false;  // Search at specified offset only
        size_t             _search_offset = 0;          // Offset where to search.
        ClockRange         _pcr_range {};               // Range of PCR values.
        ClockRange         _pts_range {};               // Range of PTS values.
        ClockRange         _dts_range {};               // Range of DTS values.
        PacketRangeList    _ranges {};                  // Ranges of packets to filter.
        std::set<uint8_t>  _stream_ids {};              // PES stream ids to filter
        std::set<uint8_t>  _isdb_layers {};             // ISDB layers to filter
        std::set<uint16_t> _service_ids {};             // Service ids to filter
        UStringVector      _service_names {};           // Service names to filter.
        TSPacketLabelSet   _labels {};                  // Select packets with any of these labels
        TSPacketLabelSet   _set_labels {};              // Labels to set on filtered packets
        TSPacketLabelSet   _reset_labels {};            // Labels to reset on filtered packets
        TSPacketLabelSet   _set_perm_labels {};         // Labels to set on all packets after getting one packet
        TSPacketLabelSet   _reset_perm_labels {};       // Labels to reset on all packets after getting one packet

        // Working data:
        PacketCounter      _filtered_packets = 0;       // Number of filtered packets
        PIDSet             _stream_id_pid {};           // PID values selected from stream ids
        std::set<uint16_t> _all_service_ids {};         // All service ids to filter, after service name resolution
        SignalizationDemux _demux {duck};               // Full signalization demux

        // Implementation of SignalizationHandlerInterface
        virtual void handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed) override;
    };
}
