//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to replace packet payload with a binary pattern.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Plugin to replace packet payload with a binary pattern on selected PID's.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PatternPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PatternPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        uint8_t   _offset_pusi = 0;      // Start offset in packets with PUSI
        uint8_t   _offset_non_pusi = 0;  // Start offset in packets without PUSI
        ByteBlock _pattern {};           // Binary pattern to apply
        PIDSet    _pid_list {};          // Array of pid values to filter
    };
}
