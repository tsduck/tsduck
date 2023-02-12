//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
        TS_NOBUILD_NOCOPY(TeletextPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        TeletextPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool             _abort;      // Error (service not found, etc).
        PID              _pid;        // Teletext PID.
        int              _page;       // Teletext page.
        int              _maxFrames;  // Max number of Teletext frames to generate.
        UString          _language;   // Language to select.
        UString          _outFile;    // Output file name.
        ServiceDiscovery _service;    // Service name & id.
        TeletextDemux    _demux;      // Teletext demux to extract subtitle frames.
        SubRipGenerator  _srtOutput;  // Generate SRT output file.
        std::set<int>    _pages;      // Set of all Teletext pages in the PID (for information only).

        // Implementation of interfaces.
        virtual void handlePMT(const PMT&, PID) override;
        virtual void handleTeletextMessage(TeletextDemux&, const TeletextFrame&) override;
    };
}
