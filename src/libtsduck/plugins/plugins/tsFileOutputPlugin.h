//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  File output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsTSFileOutputArgs.h"

namespace ts {
    //!
    //! File output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL FileOutputPlugin: public OutputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(FileOutputPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        TSFileOutputArgs _file {true}; // stdout allowed
    };
}
