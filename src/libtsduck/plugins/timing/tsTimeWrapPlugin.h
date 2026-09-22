//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to detect PCR, PTS, DTS wrap-down to zero.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Plugin to detect PCR, PTS, DTS wrap-down to zero.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL TimeWrapPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(TimeWrapPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one PID
        struct PIDContext
        {
            PIDContext() = default;
            PacketCounter packets = 0;                // Packets in this PID.
            uint64_t      pcr = PCRTraits::INVALID;   // Last PCR value in this PID.
            uint64_t      pts = PTSTraits::INVALID;   // Last PTS value in this PID.
            uint64_t      dts = DTSTraits::INVALID;   // Last DTS value in this PID.
        };

        // Command line options.
        bool   _check_pcr = false;
        bool   _check_pts = false;
        bool   _check_dts = false;
        PIDSet _pid_list {};

        // Working data.
        std::map<PID,PIDContext> _stats {};
        std::set<PID>            _wrap_pids {};
        PacketCounter            _wrap_count = 0;

        // Process one type of time stamp.
        template <TimeSource T>
        void processTime(TSPacket& pkt, bool TimeWrapPlugin::* check_opt, uint64_t PIDContext::* field, uint64_t (TSPacket::* get)() const);
    };
}
