//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  FLUTE analyzer plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractSingleMPEPlugin.h"
#include "tsmcastFluteAnalyzer.h"

namespace ts::mcast {
    //!
    //! FLUTE analyzer plugin.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL FlutePlugin: public AbstractSingleMPEPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(FlutePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual void handleSingleMPEPacket(PCR timestamp, TimeSource source, const MPEPacket& mpe) override;

    private:
        // Command line options.
        FluteAnalyzerArgs _opt_flute {};

        // Plugin private fields.
        FluteAnalyzer _flute_analyzer {duck};
    };
}
