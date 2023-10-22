//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  File packet processor plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSFileOutputArgs.h"

namespace ts {
    //!
    //! File packet processor plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL FilePacketPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(FilePacketPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        FilePacketPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        TSFileOutputArgs _file {false}; // stdout not allowed
    };
}
