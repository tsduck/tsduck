//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  File input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"
#include "tsTSFileInputArgs.h"

namespace ts {
    //!
    //! File input plugin for tsp.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL FileInputPlugin: public InputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(FileInputPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool abortInput() override;

    private:
        TSFileInputArgs _file {};
    };
}
