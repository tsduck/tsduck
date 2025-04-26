//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL DebugPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DebugPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        UString       _tag {};
        PacketCounter _packet = 0;
        char*         _null = nullptr;
        bool          _segfault = false;
        bool          _bad_alloc = false;
        bool          _exception = false;
        bool          _exit = false;
        int           _exit_code = EXIT_SUCCESS;
        UString       _env_name {};
        UString       _env_value {};
    };
}
