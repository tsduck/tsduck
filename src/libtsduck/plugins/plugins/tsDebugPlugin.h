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
        TS_PLUGIN_CONSTRUCTORS(DebugPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        UString _tag {};
        char*   _null = nullptr;
        bool    _segfault = false;
        bool    _exit = false;
        int     _exit_code = EXIT_SUCCESS;
    };
}
