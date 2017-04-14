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
//  Replace packet payload with a binary pattern on selected PID's
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsStringUtils.h"
#include "tsByteBlock.h"



//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PatternPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        PatternPlugin (TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        uint8_t     _offset_pusi;      // Start offset in packets with PUSI
        uint8_t     _offset_non_pusi;  // Start offset in packets without PUSI
        ByteBlock _pattern;          // Binary pattern to apply
        PIDSet    _pid_list;         // Array of pid values to filter
    };
}

TSPLUGIN_DECLARE_VERSION;
TSPLUGIN_DECLARE_PROCESSOR (ts::PatternPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PatternPlugin::PatternPlugin (TSP* tsp_) :
    ProcessorPlugin (tsp_, "Replace packet payload with a binary pattern on selected PID's.", "[options] pattern")
{
    option ("",                 0, STRING, 1, 1);
    option ("negate",          'n');
    option ("offset-non-pusi", 'o', INTEGER, 0, 1, 0, PKT_SIZE - 4);
    option ("offset-pusi",     'u', INTEGER, 0, 1, 0, PKT_SIZE - 4);
    option ("pid",             'p', PIDVAL,  0, UNLIMITED_COUNT);

    setHelp ("Pattern:\n"
             "  Specifies the binary pattern to apply on TS packets payload.\n"
             "  The value must be a string of hexadecimal digits specifying any\n"
             "  number of bytes.\n"
             "\n"
             "Options:\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -n\n"
             "  --negate\n"
             "      Negate the PID filter: modify packets on all PID's, expect the\n"
             "      specified ones.\n"
             "\n"
             "  -o value\n"
             "  --offset-non-pusi value\n"
             "      Specify starting offset in payload of packets with the PUSI (payload.\n"
             "      unit start indicator) not set. By default, the pattern replacement\n"
             "      starts at the beginning of the packet payload (offset 0).\n"
             "\n"
             "  -u value\n"
             "  --offset-pusi value\n"
             "      Specify starting offset in payload of packets with the PUSI (payload.\n"
             "      unit start indicator) set. By default, the pattern replacement\n"
             "      starts at the beginning of the packet payload (offset 0).\n"
             "\n"
             "  -p value\n"
             "  --pid value\n"
             "      Select packets with this PID value. Several -p or --pid options\n"
             "      may be specified to select multiple PID's. If no such option is\n"
             "      specified, packets from all PID's are modified.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PatternPlugin::start()
{
    _offset_pusi = intValue<uint8_t> ("offset-pusi", 0);
    _offset_non_pusi = intValue<uint8_t> ("offset-non-pusi", 0);

    getPIDSet (_pid_list, "pid", true);
    if (present ("negate")) {
        _pid_list.flip();
    }

    if (!HexaDecode (_pattern, value()) || _pattern.size() == 0) {
        tsp->error ("invalid hexadecimal pattern");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PatternPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // If the packet has no payload, or not in a selected PID, leave it unmodified
    if (!pkt.hasPayload() || !_pid_list[pkt.getPID()]) {
        return TSP_OK;
    }

    // Compute start of payload area to replace
    uint8_t* pl = pkt.b + pkt.getHeaderSize() + (pkt.getPUSI() ? _offset_pusi : _offset_non_pusi);

    // Compute remaining size to replace (maybe negative if starting offset is beyond the end of the packet).
    int remain = int (pkt.b + PKT_SIZE - pl);

    // Replace the payload with the pattern
    while (remain > 0) {
        int cursize = std::min (remain, int (_pattern.size()));
        ::memcpy (pl, _pattern.data(), cursize);
        pl += cursize;
        remain -= cursize;
    }

    return TSP_OK;
}
