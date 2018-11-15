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
//  Check or fix continuity counters
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class ContinuityPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        ContinuityPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        UString       _tag;             // Message tag
        bool          _fix;             // Fix incorrect continuity counters
        int           _log_level;       // Log level for discontinuity messages
        PacketCounter _packet_count;    // TS packet count
        PIDSet        _pids;            // PID values to check or fix
        uint8_t       _oldCC[PID_MAX];  // Continuity counter by PID (input)
        uint8_t       _newCC[PID_MAX];  // Continuity counter by PID (output)

        // Inaccessible operations
        ContinuityPlugin() = delete;
        ContinuityPlugin(const ContinuityPlugin&) = delete;
        ContinuityPlugin& operator=(const ContinuityPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(continuity, ts::ContinuityPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ContinuityPlugin::ContinuityPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Check or fix continuity counters on TS packets", u"[options]"),
    _tag(),
    _fix(),
    _log_level(Severity::Info),
    _packet_count(0),
    _pids()
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
// Start method
//----------------------------------------------------------------------------

bool ts::ContinuityPlugin::start()
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

    // Preset continuity counters to invalid values
    ::memset(_oldCC, 0xFF, sizeof(_oldCC));
    ::memset(_newCC, 0xFF, sizeof(_newCC));

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::ContinuityPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();

    // Check selected PID's only.
    if (_pids.test(pid)) {

        // Adjacent identical CC on a PID are allowed and indicate a duplicated packet.
        const uint8_t cc = pkt.getCC();
        const bool duplicated = _oldCC[pid] == cc;

        // Check if the CC is incorrect.
        if (_oldCC[pid] < 16 && !duplicated && ((_oldCC[pid] + 1) & 0x0F) != cc) {
            tsp->log(_log_level, u"%sTS: %'d, PID: 0x%X, missing: %d", {_tag, _packet_count, pid, (cc < _oldCC[pid] ? 16 : 0) + cc - _oldCC[pid] - 1});
        }

        // Fix CC if requested. Fixes are propagated all along the PID.
        if (_fix && _newCC[pid] < 16) {
            pkt.setCC(duplicated ? _newCC[pid] : ((_newCC[pid] + 1) & 0x0F));
        }

        _oldCC[pid] = cc;
        _newCC[pid] = pkt.getCC();
    }

    _packet_count++;
    return TSP_OK;
}
