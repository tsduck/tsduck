//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Check or fix continuity counters plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsContinuityAnalyzer.h"

namespace ts {
    //!
    //! Check or fix continuity counters plugin for tsp.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL ContinuityPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(ContinuityPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options.
        UString _tag {};                      // Message tag
        bool    _fix = false;                 // Fix incorrect continuity counters
        bool    _no_replicate = false;        // Option --no-replicate-duplicated
        bool    _json_line = false;           // Use JSON log style.
        UString _json_prefix {};              // Prefix before JSON line.
        int     _log_level = Severity::Info;  // Log level for discontinuity messages
        PIDSet  _pids {};                     // PID values to check or fix

        // Working data.
        ContinuityAnalyzer _cc_analyzer {NoPID(), this};
    };
}
