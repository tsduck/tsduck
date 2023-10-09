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
        TS_NOBUILD_NOCOPY(NullInputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        NullInputPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool abortInput() override;
        virtual bool setReceiveTimeout(MilliSecond timeout) override;

    private:
        PacketCounter _max_count;   // Number of packets to generate
        PacketCounter _count;       // Number of generated packets
        PacketCounter _limit;       // Current max number of packets
    };
}
