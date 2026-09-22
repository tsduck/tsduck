//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream fuzzing (random corruption) plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSFuzzing.h"

namespace ts {
    //!
    //! Transport stream fuzzing (random corruption) plugin.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL FuzzPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(FuzzPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        TSFuzzingArgs _fuzz_opt {};
        TSFuzzing     _fuzzer {duck};
    };
}
