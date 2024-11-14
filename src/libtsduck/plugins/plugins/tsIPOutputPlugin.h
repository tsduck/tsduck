//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
        TS_PLUGIN_CONSTRUCTORS(IPOutputPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        TSDatagramOutput _datagram {TSDatagramOutputOptions::ALLOW_RTP | TSDatagramOutputOptions::ALLOW_RS204};
    };
}
