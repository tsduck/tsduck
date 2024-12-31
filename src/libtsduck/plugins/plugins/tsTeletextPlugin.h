//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Extract Teletext subtitles plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSignalizationHandlerInterface.h"
#include "tsTeletextDemux.h"
#include "tsServiceDiscovery.h"
#include "tsSubRipGenerator.h"

namespace ts {
    //!
    //! Extract Teletext subtitles plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL TeletextPlugin: public ProcessorPlugin, private SignalizationHandlerInterface, private TeletextHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(TeletextPlugin);
    public:
        // Implementation of plugin API
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool             _abort = false;                // Error (service not found, etc).
        PID              _pid = PID_NULL;               // Teletext PID.
        int              _page {-1};                    // Teletext page.
        int              _maxFrames = 0;                // Max number of Teletext frames to generate.
        UString          _language {};                  // Language to select.
        fs::path         _outFile {};                   // Output file name.
        ServiceDiscovery _service {duck, this};         // Service name & id.
        TeletextDemux    _demux {duck, this, NoPID()};  // Teletext demux to extract subtitle frames.
        SubRipGenerator  _srtOutput {};                 // Generate SRT output file.
        std::set<int>    _pages {};                     // Set of all Teletext pages in the PID (for information only).

        // Implementation of interfaces.
        virtual void handlePMT(const PMT&, PID) override;
        virtual void handleTeletextMessage(TeletextDemux&, const TeletextFrame&) override;
    };
}
