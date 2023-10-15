//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Skip packet processor plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Skip packet processor plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL SkipPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(SkipPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        SkipPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        PacketCounter _skip_count {0};
        bool          _use_stuffing {false};
    };
}
