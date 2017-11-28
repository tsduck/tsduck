//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Check continuity counters
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
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
        UString       _tag;            // Message tag
        PacketCounter _packet_count;   // TS packet count
        uint8_t       _cc[PID_MAX];    // Continuity counter by PID

        // Inaccessible operations
        ContinuityPlugin() = delete;
        ContinuityPlugin(const ContinuityPlugin&) = delete;
        ContinuityPlugin& operator=(const ContinuityPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::ContinuityPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ContinuityPlugin::ContinuityPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Check continuity counters on TS packets.", u"[options]"),
    _tag(),
    _packet_count(0)
{
    option(u"tag", 't', STRING);

    setHelp(u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -t 'string'\n"
            u"  --tag 'string'\n"
            u"      Message tag to be displayed when packets are missing. Useful when\n"
            u"      the plugin is used several times in the same process.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::ContinuityPlugin::start()
{
    // Command line arguments
    _tag = value(u"tag");
    if (!_tag.empty()) {
        _tag += ": ";
    }

    // Preset continuity counters to invalid values
    ::memset(_cc, 0xFF, sizeof(_cc));

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::ContinuityPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();
    const uint8_t cc = pkt.getCC();

    // Continuity counters (CC) are located in the TS header. This is a 4-bit
    // field, incremented at each packet on a PID, wrapping from 15 to 0.
    // Null packets are not subject to CC. Adjacent identical CC on a PID
    // are allowed and indicate a duplicated packet.

    if (pid != PID_NULL &&              // not a null packet
        _cc[pid] < 16 &&                // not first packet on PID
        _cc[pid] != cc &&               // not a duplicated packet
        ((_cc[pid] + 1) & 0x0F) != cc)  // wrong CC
    {
        tsp->info("%sTS: %'d, PID: 0x%X, missing: %d", {_tag, _packet_count, pid, (cc < _cc[pid] ? 16 : 0) + cc - _cc[pid] - 1});
    }

    _packet_count++;
    _cc[pid] = cc;

    return TSP_OK;
}
