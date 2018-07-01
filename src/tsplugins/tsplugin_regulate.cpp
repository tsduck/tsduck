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
//  Regulate (slow down) the packet flow according to a bitrate.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsBitRateRegulator.h"
TSDUCK_SOURCE;

#define DEF_PACKET_BURST 16


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RegulatePlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        RegulatePlugin(TSP*);
        virtual bool start() override;
        virtual bool isRealTime() override {return true;}
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        BitRateRegulator _regulator;

        // Inaccessible operations
        RegulatePlugin() = delete;
        RegulatePlugin(const RegulatePlugin&) = delete;
        RegulatePlugin& operator=(const RegulatePlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(regulate, ts::RegulatePlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::RegulatePlugin::RegulatePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Regulate the TS packets flow to a specified bitrate", u"[options]"),
    _regulator(tsp, Severity::Verbose)
{
    option(u"bitrate",      'b', POSITIVE);
    option(u"packet-burst", 'p', POSITIVE);

    setHelp(u"Regulate (slow down only) the TS packets flow according to a specified\n"
            u"bitrate. Useful to play a non-regulated input (such as a TS file) to a\n"
            u"non-regulated output device such as IP multicast.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -b value\n"
            u"  --bitrate value\n"
            u"      Specify the bitrate in b/s. By default, use the \"input\" bitrate,\n"
            u"      typically resulting from the PCR analysis of the input file.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -p value\n"
            u"  --packet-burst value\n"
            u"      Number of packets to burst at a time. Does not modify the average\n"
            u"      output bitrate but influence smoothing and CPU load. The default\n"
            u"      is " TS_STRINGIFY(DEF_PACKET_BURST) u" packets.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RegulatePlugin::start()
{
    _regulator.setBurstPacketCount(intValue<PacketCounter>(u"packet-burst", DEF_PACKET_BURST));
    _regulator.setFixedBitRate(intValue<BitRate>(u"bitrate", 0));
    _regulator.start();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::RegulatePlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    _regulator.regulate(tsp->bitrate(), flush, bitrate_changed);
    return TSP_OK;
}
