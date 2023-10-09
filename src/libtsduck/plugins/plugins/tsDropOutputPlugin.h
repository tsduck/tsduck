//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Drop output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"

namespace ts {
    //!
    //! Drop output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL DropOutputPlugin: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(DropOutputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        DropOutputPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;
    };
}
