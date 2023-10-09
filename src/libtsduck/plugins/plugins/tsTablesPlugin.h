//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Collect selected PSI/SI tables plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTablesDisplay.h"
#include "tsTablesLogger.h"

namespace ts {
    //!
    //! Collect selected PSI/SI tables plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL TablesPlugin: public ProcessorPlugin, private SectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TablesPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        TablesPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        TablesDisplay _display;
        TablesLogger  _logger;
        bool          _signal_event;  // Signal a plugin event on section.
        uint32_t      _event_code;    // Event code to signal.

        // Implementation of SectionHandlerInterface
        virtual void handleSection(SectionDemux& demux, const Section& section) override;
    };
}
