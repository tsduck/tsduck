//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Reliable Internet Stream Transport (RIST) output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsTSDatagramOutputHandlerInterface.h"

namespace ts {
    //!
    //! Reliable Internet Stream Transport (RIST) output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL RISTOutputPlugin: public OutputPlugin, private TSDatagramOutputHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(RISTOutputPlugin);
    public:
        //! Destructor.
        virtual ~RISTOutputPlugin() override;

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        // The actual implementation is private to the body of the class.
        class Guts;
        Guts* _guts = nullptr;

        // Implementation of TSDatagramOutputHandlerInterface.
        virtual bool sendDatagram(const void* address, size_t size, Report& report) override;
    };
}
