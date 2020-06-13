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
//  Collect selected PSI/SI tables from a transport stream.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTablesLogger.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TablesPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(TablesPlugin);
    public:
        // Implementation of plugin API
        TablesPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        TablesDisplay _display;
        TablesLogger  _logger;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"tables", ts::TablesPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TablesPlugin::TablesPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Collect PSI/SI Tables", u"[options]"),
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
// Start /stop methods
//----------------------------------------------------------------------------

bool ts::TablesPlugin::getOptions()
{
    duck.reset();
    return duck.loadArgs(*this) && _logger.loadArgs(duck, *this) && _display.loadArgs(duck, *this);
}

bool ts::TablesPlugin::start()
{
    duck.resetStandards();  // reset accumulated standards (not command line ones).
    return _logger.open();
}

bool ts::TablesPlugin::stop()
{
    _logger.close();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TablesPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _logger.feedPacket(pkt);
    return _logger.completed() ? TSP_END : TSP_OK;
}
