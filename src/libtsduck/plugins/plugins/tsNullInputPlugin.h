//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Null packet input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"

namespace ts {
    //!
    //! Null packet input plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL NullInputPlugin: public InputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(NullInputPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool abortInput() override;
        virtual bool setReceiveTimeout(std::chrono::milliseconds timeout) override;

    private:
        PacketCounter _max_count = 0;   // Number of packets to generate
        PacketCounter _count = 0;       // Number of generated packets
        PacketCounter _limit = 0;       // Current max number of packets
    };
}
