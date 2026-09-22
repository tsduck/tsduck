//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to multiplex transport stream file in the TS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSFile.h"
#include "tsContinuityAnalyzer.h"

namespace ts {
    //!
    //! Plugin to multiplex transport stream file in the TS, stealing packets from stuffing.
    //! Note: the work "mux" is inappropriate since this is not a real multiplexer.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL MuxPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(MuxPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        TSFile        _file {this};                   // Input file
        bool          _terminate = false;             // Terminate processing after last new packet.
        bool          _update_cc = false;             // Ignore continuity counters.
        bool          _check_pid_conflict = false;    // Check new PIDs in TS
        PIDSet        _ts_pids {};                    // PID's on original TS
        bool          _force_pid = false;             // PID value to force
        PID           _force_pid_value = PID_NULL;    // PID value to force
        BitRate       _bitrate = 0;                   // Target bitrate for inserted packets
        PacketCounter _inter_pkt = 0;                 // # TS packets between 2 new PID packets
        PacketCounter _pid_next_pkt = 0;              // Next time to insert a packet
        uint64_t      _inter_time = 0;                // Milliseconds between 2 new packets, internally calculated to PTS (multiplicated by 90)
        uint64_t      _min_pts = 0;                   // Start only inserting packets when this PTS has been passed
        PID           _pts_pid = PID_NULL;            // defines the PID of min-pts setting
        uint64_t      _max_pts = 0;                   // After this PTS has been seen, stop inserting
        bool          _pts_range_ok = false;          // signal indicates if we shall insert
        uint64_t      _max_insert_count = 0;          // from userinput, maximum packets to insert
        uint64_t      _inserted_packet_count = 0;     // counts inserted packets
        uint64_t      _youngest_pts = 0;              // stores last pcr value seen (calculated from PCR to PTS value by dividing by 300)
        uint64_t      _pts_last_inserted = 0;         // stores nearest pts (actually pcr/300) of last packet insertion
        TSPacketFormat     _file_format = TSPacketFormat::AUTODETECT; // Input file format
        TSPacketLabelSet   _setLabels {};                // Labels to set on output packets.
        TSPacketLabelSet   _resetLabels {};              // Labels to reset on output packets.
        ContinuityAnalyzer _cc_fixer {AllPIDs(), this};  // To fix continuity counters in mux'ed PID's
    };
}
