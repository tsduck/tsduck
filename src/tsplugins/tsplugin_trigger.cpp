//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  Trigger actions on selected labeled TS packets.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TriggerPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        TriggerPlugin (TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        UString                    _execute;      // Command to execute on trigger.
        bool                       _allPackets;   // Trigger on all packets in the stream.
        bool                       _allLabels;    // Need all labels to be set.
        TSPacketMetadata::LabelSet _labels;       // Trigger on packets with these labels.

        // Working data:
        PacketCounter  _unselectedCount;  // Number of consecutive unselected packets.
        
        // Trigger all actions.
        void trigger();

        // Inaccessible operations
        TriggerPlugin() = delete;
        TriggerPlugin(const TriggerPlugin&) = delete;
        TriggerPlugin& operator=(const TriggerPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(trigger, ts::TriggerPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TriggerPlugin::TriggerPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Trigger actions on selected labeled TS packets", u"[options]"),
    _allPackets(false),
    _allLabels(false),
    _labels(),
    _unselectedCount(0)
{
    option(u"all-labels", 'a');
    help(u"all-labels",
         u"All labels from options --label shall be set on a packet to be selected (logical 'and'). "
         u"By default, a packet is selected if any label is set (logical 'or').");

    option(u"execute", 'e', STRING);
    help(u"execute", u"'command'",
         u"Run the specified command the current packet triggers the actions.");

    option(u"label", 'l', INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"label", u"label1[-label2]",
         u"Trigger the actions on packets with any of the specified labels. "
         u"Labels should have typically be set by a previous plugin in the chain. "
         u"By default, without option --label, the actions are triggered on all packets in the stream. "
         u"Several --label options may be specified.\n\n"
         u"Note that the option --label is different from the generic option --only-label. "
         u"The generic option --only-label acts at tsp level and controls which packets are "
         u"passed to the plugin. All other packets are directly passed to the next plugin "
         u"without going through this plugin. The option --label, on the other hand, "
         u"is specific to the trigger plugin and selects packets with specific labels "
         u"among the packets which are passed to this plugin.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::TriggerPlugin::getOptions()
{
    getValue(_execute, u"execute");
    getIntValues(_labels, u"label");
    _allLabels = present(u"all-labels");
    _allPackets = _labels.none();
    return true;
}


//----------------------------------------------------------------------------
// Start method.
//----------------------------------------------------------------------------

bool ts::TriggerPlugin::start()
{
    _unselectedCount = 0;
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TriggerPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Check if the packet shall be selected.
    if (_allPackets || (_allLabels && pkt_data.hasAllLabels(_labels)) || (!_allLabels && pkt_data.hasAnyLabel(_labels))) {
        // The packet shall be selected
        _unselectedCount = 0;
        trigger();
    }
    else {
        // The packet shall not be selected.
        ++_unselectedCount;
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Trigger all actions.
//----------------------------------------------------------------------------

void ts::TriggerPlugin::trigger()
{

}
