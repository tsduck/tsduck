//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Memory output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"

namespace ts {
    //!
    //! Memory output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL MemoryOutputPlugin: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(MemoryOutputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        MemoryOutputPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        uint32_t _event_code {0};
    };
}
