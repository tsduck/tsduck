//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsTablesLogger.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TablesPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        TablesPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        TablesDisplayArgs _display_options;
        TablesLoggerArgs  _logger_options;
        TablesDisplay     _display;
        TablesLoggerPtr   _logger;

        // Inaccessible operations
        TablesPlugin() = delete;
        TablesPlugin(const TablesPlugin&) = delete;
        TablesPlugin& operator=(const TablesPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(tables, ts::TablesPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TablesPlugin::TablesPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Collect PSI/SI Tables", u"[options]"),
    _display_options(),
    _logger_options(),
    _display(_display_options, *tsp),
    _logger()
{
    _logger_options.defineOptions(*this);
    _display_options.defineOptions(*this);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TablesPlugin::start()
{
    // Decode command line options.
    if (!_logger_options.load(*this) || !_display_options.load(*this)) {
        return false;
    }

    // Create the logger.
    _logger = new TablesLogger(_logger_options, _display, *tsp);
    return !_logger->hasErrors();
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::TablesPlugin::stop()
{
    _logger->close();
    _logger.clear();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TablesPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    _logger->feedPacket (pkt);
    return _logger->completed() ? TSP_END : TSP_OK;
}
