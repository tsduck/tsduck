//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Debug packet processor plugin for tsp, display various traces.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Debug packet processor plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL DebugPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(DebugPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        DebugPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        UString _tag;
        char*   _null;
        bool    _segfault;
        bool    _exit;
        int     _exit_code;
    };
}
