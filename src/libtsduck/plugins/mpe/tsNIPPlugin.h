//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB-NIP (Native IP) analyzer plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractSingleMPEPlugin.h"
#include "tsmcastNIPAnalyzer.h"

namespace ts::mcast {
    //!
    //! DVB-NIP (Native IP) analyzer plugin.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL NIPPlugin: public AbstractSingleMPEPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(NIPPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual void handleSingleMPEPacket(PCR timestamp, TimeSource source, const MPEPacket& mpe) override;

    private:
        // Command line options.
        NIPAnalyzerArgs _opt_nip {};

        // Plugin private fields.
        NIPAnalyzer _nip_analyzer {duck};
    };
}
