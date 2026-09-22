//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to extract DSM-CC Object Carousel content.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsDSMCCExtractor.h"
#include "tsDSMCCExtractorArgs.h"

namespace ts {
    //!
    //! Plugin to extract DSM-CC Object Carousel content.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL DSMCCPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DSMCCPlugin);
    public:
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool getOptions() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        DSMCCExtractorArgs              _args {};
        std::shared_ptr<DSMCCExtractor> _extractor {};
    };
}
