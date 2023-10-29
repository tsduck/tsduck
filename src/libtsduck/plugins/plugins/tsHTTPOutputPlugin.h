//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  HTTP output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"

namespace ts {
    //!
    //! HTTP output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL HTTPOutputPlugin: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(HTTPOutputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        HTTPOutputPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        // Command line options:

        // Working data:
    };
}
