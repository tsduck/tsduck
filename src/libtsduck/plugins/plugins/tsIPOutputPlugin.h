//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IP output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsTSDatagramOutput.h"

namespace ts {
    //!
    //! IP output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL IPOutputPlugin: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(IPOutputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        IPOutputPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        TSDatagramOutput _datagram;
    };
}
