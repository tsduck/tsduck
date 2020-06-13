//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//
//  Transport stream processor shared library:
//  Display PSI/SI information from a transport stream
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsPSILogger.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PSIPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(PSIPlugin);
    public:
        // Implementation of plugin API
        PSIPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        TablesDisplay _display;
        PSILogger     _logger;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"psi", ts::PSIPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PSIPlugin::PSIPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract PSI Information", u"[options]"),
    _display(duck),
    _logger(_display)
{
    duck.defineArgsForCAS(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForStandards(*this);
    duck.defineArgsForCharset(*this);
    _logger.defineArgs(*this);
    _display.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Start / stop methods
//----------------------------------------------------------------------------

bool ts::PSIPlugin::getOptions()
{
    return duck.loadArgs(*this) && _logger.loadArgs(duck, *this) && _display.loadArgs(duck, *this);
}


bool ts::PSIPlugin::start()
{
    return _logger.open();
}

bool ts::PSIPlugin::stop()
{
    _logger.close();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PSIPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _logger.feedPacket(pkt);
    return _logger.completed() ? TSP_END : TSP_OK;
}
