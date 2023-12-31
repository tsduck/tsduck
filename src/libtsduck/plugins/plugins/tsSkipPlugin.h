//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Skip packet processor plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Skip packet processor plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL SkipPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(SkipPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        PacketCounter _skip_count = 0;
        bool          _use_stuffing = false;
    };
}
