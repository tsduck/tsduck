//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to merge PSI/SI from mixed transport streams.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsPSIMerger.h"

namespace ts {
    //!
    //! Plugin to merge PSI/SI from mixed transport streams.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PSIMergePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PSIMergePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        PSIMerger _psi_merger {duck, PSIMerger::NONE};  // Engine to merge PSI/SI.
        size_t    _main_label = NPOS;                   // Label of packets from main stream or greater than LABEL_MAX if none.
        size_t    _merge_label = NPOS;                  // Label of packets from main stream or greater than LABEL_MAX if none.
    };
}
