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
//  Check or fix continuity counters
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsContinuityAnalyzer.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class ContinuityPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(ContinuityPlugin);
    public:
        // Implementation of plugin API
        ContinuityPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        UString            _tag;          // Message tag
        bool               _fix;          // Fix incorrect continuity counters
        int                _log_level;    // Log level for discontinuity messages
        PIDSet             _pids;         // PID values to check or fix
        ContinuityAnalyzer _cc_analyzer;  // Continuity counters analyzer
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"continuity", ts::ContinuityPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ContinuityPlugin::ContinuityPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Check or fix continuity counters on TS packets", u"[options]"),
    _tag(),
    _fix(),
    _log_level(Severity::Info),
    _pids(),
    _cc_analyzer(NoPID, tsp)
{
    option(u"fix", 'f');
    help(u"fix",
         u"Fix incorrect continuity counters. By default, only display discontinuities.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Check or fix continuity counters only in packets with this PID value or range of values. "
         u"Several -p or --pid options may be specified. By default, all PID's "
         u"are checked or fixed.");

    option(u"tag", 't', STRING);
    help(u"tag", u"'string'",
         u"Message tag to be displayed when packets are missing. Useful when "
         u"the plugin is used several times in the same process.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::ContinuityPlugin::getOptions()
{
    // Command line arguments
    getIntValues(_pids, u"pid", true);
    _fix = present(u"fix");
    _tag = value(u"tag");
    if (!_tag.empty()) {
        _tag += u": ";
    }

    // Null packets are not subject to continuity counters. Never check the null PID.
    _pids.reset(PID_NULL);

    // Without --fix, all discontinuities are always reported.
    // With --fix, this is only a verbose message.
    _log_level = _fix ? Severity::Verbose : Severity::Info;

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::ContinuityPlugin::start()
{
    _cc_analyzer.reset();
    _cc_analyzer.setPIDFilter(_pids);
    _cc_analyzer.setDisplay(true);
    _cc_analyzer.setMessagePrefix(_tag);
    _cc_analyzer.setMessageSeverity(_log_level);
    _cc_analyzer.setFix(_fix);
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::ContinuityPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _cc_analyzer.feedPacket(pkt);
    return TSP_OK;
}
