//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IP packet processor plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSDatagramOutput.h"

namespace ts {
    //!
    //! IP output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL IPPacketPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(IPPacketPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        IPPacketPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        TSDatagramOutput _datagram {TSDatagramOutputOptions::ALLOW_RTP | TSDatagramOutputOptions::ALWAYS_BURST};
    };
}
