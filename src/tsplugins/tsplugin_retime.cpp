//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
// Copyright (c) 2019, Lars The (lars18th)
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
//  Change PTS/DTS values on-the-fly
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class ReTimerPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(ReTimerPlugin);
    public:
        // Implementation of plugin API
        ReTimerPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        size_t        _offset;   // Offset over current PTS/DTS
        PIDSet        _pids;     // PID values to apply changes
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(retime, ts::ReTimerPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ReTimerPlugin::ReTimerPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Change PTS/DTS values of a stream", u"[options]"),
    _offset(0),
    _pids()
{
    option(u"offset", 'o', INT32);
    help(u"offset",
         u"Specify the offset value to apply to all PTS/DTS marks of the "
         u"packets of selected pids.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"PID: apply changes to packets with these PID values. "
         u"Several -p or --pid options may be specified.");

}

//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::ReTimerPlugin::start()
{
    // Get option values
    _offset = intValue<size_t>(u"offset", 0) & PTS_DTS_MASK;
    getIntValues(_pids, u"pid");

    return true;
}

//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::ReTimerPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    bool ok = false;
    const PID pid = pkt.getPID();
    const bool haspts = pkt.hasPTS();
    const bool hasdts = pkt.hasDTS();
    uint64_t currentPTS = 0;
    uint64_t currentDTS = 0;

    if (_pids[pid] && haspts) {
        currentPTS = pkt.getPTS();
        pkt.setPTS((currentPTS + _offset) & PTS_DTS_MASK);
        ok = true;
    }

    if (_pids[pid] && hasdts) {
        currentDTS = pkt.getDTS();
        pkt.setDTS((currentDTS + _offset) & PTS_DTS_MASK);
        ok = true;
    }

    if (ok) {
        tsp->debug(u"pid:%d PTS/DTS Changed(%d): [%d/%d : %d/%d]",
                {pid, _offset,
                 haspts ? currentPTS : INVALID_PTS, haspts ? pkt.getPTS() : INVALID_PTS,
                 hasdts ? currentDTS : INVALID_PTS, hasdts ? pkt.getDTS() : INVALID_PTS});
    }

    return TSP_OK;
}
