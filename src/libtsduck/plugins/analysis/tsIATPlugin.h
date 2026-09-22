//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IAT analysis plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsIATAnalyzer.h"
#include "tsTime.h"

namespace ts {
    //!
    //! IAT analysis plugin for tsp.
    //! Analyze Inter-packet Arrival Time (IAT) for datagram-based inputs.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL IATPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(IATPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Default values.
        static constexpr cn::seconds DEFAULT_INTERVAL = cn::seconds(5);  // Default logging interval in seconds.

        // Command line options:
        cn::seconds _log_interval {};

        // Working data:
        Time        _due_time {};
        IATAnalyzer _iat_analyzer {*this};
    };
}
