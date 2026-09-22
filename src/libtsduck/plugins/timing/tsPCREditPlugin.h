//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to edit PCR, PTS and DTS values in various ways.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSystemRandomGenerator.h"

namespace ts {
    //!
    //! Plugin to edit PCR, PTS and DTS values in various ways.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PCREditPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PCREditPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Type of units for PCR, PTS, DTS values.
        enum {UNIT_DEFAULT, UNIT_PCR, UNIT_PTS, UNIT_MILLISEC, UNIT_NANOSEC};

        // Command line options.
        bool    _ignore_scrambled = false;
        bool    _random = false;
        int64_t _add_pcr = 0;
        int64_t _add_pts = 0;
        int64_t _add_dts = 0;
        PIDSet  _pids {};
        SystemRandomGenerator _prng {};

        // Return actual value to apply.
        int64_t adjust(int64_t value);
    };
}
