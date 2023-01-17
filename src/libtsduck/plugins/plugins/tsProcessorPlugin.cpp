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

#include "tsProcessorPlugin.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::ProcessorPlugin::ProcessorPlugin(TSP* tsp_, const UString& description, const UString& syntax) :
    Plugin(tsp_, description, syntax)
{
    // The option --label is defined in all packet processing plugins.
    option(u"only-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"only-label", u"label1[-label2]",
         u"Invoke this plugin only for packets with any of the specified labels. "
         u"Other packets are transparently passed to the next plugin, without going through this one. "
         u"Several --only-label options may be specified. "
         u"This is a generic option which is defined in all packet processing plugins.");
}


//----------------------------------------------------------------------------
// Get the content of the --only-label options (packet processing plugins).
//----------------------------------------------------------------------------

ts::TSPacketLabelSet ts::ProcessorPlugin::getOnlyLabelOption() const
{
    TSPacketLabelSet labels;
    getIntValues(labels, u"only-label");
    return labels;
}


//----------------------------------------------------------------------------
// Default implementations of virtual methods.
//----------------------------------------------------------------------------

ts::PluginType ts::ProcessorPlugin::type() const
{
    return PluginType::PROCESSOR;
}

size_t ts::ProcessorPlugin::getPacketWindowSize()
{
    return 0;
}

ts::ProcessorPlugin::Status ts::ProcessorPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    return TSP_OK;
}


//----------------------------------------------------------------------------
// Default implementations of packet window processing interface.
//----------------------------------------------------------------------------

size_t ts::ProcessorPlugin::processPacketWindow(TSPacketWindow& win)
{
    // The default implementation calls processPacket() for each packet.
    // Thus, if a plugin accidentally returns a non-zero window size without
    // overriding processPacketWindow(), the packet processing still applies
    // with the default method.

    // Warning: dirty hack :(
    // The values for tsp->pluginPackets() and tsp->totalPacketsInThread()
    // are updated by the plugin executor, after returning from processPacketWindow().
    // But if we emulate processPacketWindow() using processPacket(), we need the
    // packet counters to be incremented after each packet. We emulate this by
    // directly hacking into the TSP object. Naughty, naughty, naughty...
    // So, we need to save and restore the packet counters.
    const PacketCounter saved_total_packets = tsp->_total_packets;
    const PacketCounter saved_plugin_packets = tsp->_plugin_packets;

    TSPacket* pkt = nullptr;
    TSPacketMetadata* mdata = nullptr;
    size_t processed_packets = 0;

    while (processed_packets < win.size()) {
        if (win.get(processed_packets, pkt, mdata)) {
            const Status status = processPacket(*pkt, *mdata);
            if (status == TSP_NULL) {
                win.nullify(processed_packets);
            }
            else if (status == TSP_DROP) {
                win.drop(processed_packets);
            }
            else if (status == TSP_END) {
                break;
            }
            if (mdata->getBitrateChanged()) {
                tsp->_tsp_bitrate = getBitrate();
                tsp->_tsp_bitrate_confidence = getBitrateConfidence();
            }
            tsp->_plugin_packets++;
        }
        tsp->_total_packets++;
        processed_packets++;
    }

    // Restore hacked values.
    tsp->_total_packets = saved_total_packets;
    tsp->_plugin_packets = saved_plugin_packets;

    return processed_packets;
}
