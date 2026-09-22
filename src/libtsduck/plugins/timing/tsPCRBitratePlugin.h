//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to permanently recompute bitrate based on PCR analysis.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsPCRAnalyzer.h"

namespace ts {
    //!
    //! Plugin to permanently recompute bitrate based on PCR analysis.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PCRBitratePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PCRBitratePlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        PCRAnalyzer _pcr_analyzer {}; // PCR analysis context
        BitRate     _bitrate = 0;     // Last remembered bitrate (keep it signed)
        UString     _pcr_name {};     // Time stamp type name

        static constexpr size_t DEFAULT_MIN_PCR_COUNT = 128;
        static constexpr size_t DEFAULT_MIN_PID_COUNT = 1;

        // PCR analysis is done permanently. Typically, the analysis of a
        // constant stream will produce different results quite often. But
        // the results vary by a few bits only. This is a normal behavior
        // which would generate useless activity if reported. Consequently,
        // once a bitrate is statistically computed, we keep it as long as
        // the results are not significantly different. We ignore new results
        // which vary only by less than the following factor.

        static constexpr BitRate::int_t REPORT_THRESHOLD = 500'000; // 100 b/s on a 50 Mb/s stream
    };
}
