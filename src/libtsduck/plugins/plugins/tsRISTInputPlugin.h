//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Reliable Internet Stream Transport (RIST) input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"

namespace ts {
    //!
    //! Reliable Internet Stream Transport (RIST) input plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL RISTInputPlugin: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(RISTInputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        RISTInputPlugin(TSP* tsp);

        //!
        //! Destructor.
        //!
        virtual ~RISTInputPlugin() override;

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool isRealTime() override;
        virtual bool setReceiveTimeout(MilliSecond timeout) override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;

    private:
        // The actual implementation is private to the body of the class.
        class Guts;
        Guts* _guts = nullptr;
    };
}
