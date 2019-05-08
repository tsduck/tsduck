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
//  Filter TS packets.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class FilterPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        FilterPlugin (TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        Status        drop_status;      // Return status for unselected packets
        int           scrambling_ctrl;  // Scrambling control value (<0: no filter)
        bool          with_payload;     // Packets with payload
        bool          with_af;          // Packets with adaptation field
        bool          with_pes;         // Packets with clear PES headers
        bool          has_pcr;          // Packets with PCR or OPCR
        bool          unit_start;       // Packets with payload unit start
        bool          nullified;        // Packets which were nullified by a previous plugin
        bool          input_stuffing;   // Null packets which were artificially inserted
        bool          valid;            // Packets with valid sync byte and error ind
        bool          negate;           // Negate filter (exclude selected packets)
        int           min_payload;      // Minimum payload size (<0: no filter)
        int           max_payload;      // Maximum payload size (<0: no filter)
        int           min_af;           // Minimum adaptation field size (<0: no filter)
        int           max_af;           // Maximum adaptation field size (<0: no filter)
        PacketCounter after_packets;    // Number of initial packets to skip
        PacketCounter every_packets;    // Filter 1 out of this number of packets
        PIDSet        pid;              // PID values to filter
        TSPacketMetadata::LabelSet labels;       // Select packets with any of these labels
        TSPacketMetadata::LabelSet set_labels;   // Labels to set on filtered packets
        TSPacketMetadata::LabelSet reset_labels; // Labels to reset on filtered packets

        // Inaccessible operations
        FilterPlugin() = delete;
        FilterPlugin(const FilterPlugin&) = delete;
        FilterPlugin& operator=(const FilterPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(filter, ts::FilterPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::FilterPlugin::FilterPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Filter TS packets according to various conditions", u"[options]"),
    drop_status(TSP_DROP),
    scrambling_ctrl(0),
    with_payload(false),
    with_af(false),
    with_pes(false),
    has_pcr(false),
    unit_start(false),
    nullified(false),
    input_stuffing(false),
    valid(false),
    negate(false),
    min_payload(0),
    max_payload(0),
    min_af(0),
    max_af(0),
    after_packets(0),
    every_packets(0),
    pid(),
    labels(),
    set_labels(),
    reset_labels()
{
    option(u"adaptation-field");
    help(u"adaptation-field", u"Select packets with an adaptation field.");

    option(u"after-packets", 0, UNSIGNED);
    help(u"after-packets", u"count",
         u"Let the first 'count' packets pass transparently without filtering. Start "
         u"to apply the filtering criteria after that number of packets.");

    option(u"clear", 'c');
    help(u"clear",
         u"Select clear (unscrambled) packets. "
         u"Equivalent to --scrambling-control 0.");

    option(u"every", 0, UNSIGNED);
    help(u"every", u"count", u"Select one packet every that number of packets.");

    option(u"label", 'l', INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"label", u"label1[-label2]",
         u"Select packets with any of the specified labels. "
         u"Labels should have typically be set by a previous plugin in the chain. "
         u"Several --label options may be specified.\n\n"
         u"Note that the option --label is different from the generic option --only-label. "
         u"The generic option --only-label acts at tsp level and controls which packets are "
         u"passed to the plugin. All other packets are directly passed to the next plugin "
         u"without going through this plugin. The option --label, on the other hand, "
         u"is specific to the filter plugin and selects packets with specific labels "
         u"among the packets which are passed to this plugin.");

    option(u"max-adaptation-field-size", 0, INTEGER, 0, 1, 0, 184);
    help(u"max-adaptation-field-size",
         u"Select packets with no adaptation field or with an adaptation field the "
         u"size (in bytes) of which is not greater than the specified value.");

    option(u"max-payload-size", 0, INTEGER, 0, 1, 0, 184);
    help(u"max-payload-size",
         u"Select packets with no payload or with a payload the size (in bytes) of "
         u"which is not greater than the specified value.");

    option(u"min-adaptation-field-size", 0, INTEGER, 0, 1, 0, 184);
    help(u"min-adaptation-field-size",
         u"Select packets with an adaptation field the size (in bytes) of which "
         u"is equal to or greater than the specified value.");

    option(u"min-payload-size", 0, INTEGER, 0, 1, 0, 184);
    help(u"min-payload-size",
         u"Select packets with a payload the size (in bytes) of which is equal "
         u"to or greater than the specified value.");

    option(u"negate", 'n');
    help(u"negate", u"Negate the filter: specified packets are excluded.");

    option(u"nullified");
    help(u"nullified",
         u"Select packets which were explicitly turned into null packets by some previous "
         u"plugin in the chain (typically using a --stuffing option).");

    option(u"input-stuffing");
    help(u"input-stuffing",
         u"Select packets which were articially inserted as stuffing before the input "
         u"plugin (using tsp options --add-start-stuffing, --add-input-stuffing and "
         u"--add-stop-stuffing). Be aware that these packets may no longer be null "
         u"packets if some previous plugin injected data, replacing stuffing.");

    option(u"payload");
    help(u"payload", u"Select packets with a payload.");

    option(u"pcr");
    help(u"pcr", u"Select packets with PCR or OPCR.");

    option(u"pes");
    help(u"pes", u"Select packets with clear PES headers.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"PID filter: select packets with these PID values. "
         u"Several -p or --pid options may be specified.");

    option(u"scrambling-control", 0, INTEGER, 0, 1, 0, 3);
    help(u"scrambling-control",
         u"Select packets with the specified scrambling control value. Valid "
         u"values are 0 (clear), 1 (reserved), 2 (even key), 3 (odd key).");

    option(u"set-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"set-label", u"label1[-label2]",
         u"Set the specified labels on the selected packets. "
         u"Do not drop unselected packets, simply mark selected ones. "
         u"Several --set-label options may be specified.");

    option(u"reset-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"reset-label", u"label1[-label2]",
         u"Clear the specified labels on the selected packets. "
         u"Do not drop unselected packets, simply clear labels on selected ones. "
         u"Several --reset-label options may be specified.");

    option(u"stuffing", 's');
    help(u"stuffing",
         u"Replace excluded packets with stuffing (null packets) instead "
         u"of removing them. Useful to preserve bitrate.");

    option(u"unit-start");
    help(u"unit-start", u"Select packets with payload unit start indicator.");

    option(u"valid", 'v');
    help(u"valid",
         u"Select valid packets. A valid packet starts with 0x47 and has "
         u"its transport_error_indicator cleared.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::FilterPlugin::getOptions()
{
    scrambling_ctrl = present(u"clear") ? 0 : intValue(u"scrambling-control", -1);
    with_payload = present(u"payload");
    with_af = present(u"adaptation-field");
    with_pes = present(u"pes");
    has_pcr = present(u"pcr");
    unit_start = present(u"unit-start");
    nullified = present(u"nullified");
    input_stuffing = present(u"input-stuffing");
    valid = present(u"valid");
    negate = present(u"negate");
    getIntValue(min_payload, u"min-payload-size", -1);
    getIntValue(max_payload, u"max-payload-size", -1);
    getIntValue(min_af, u"min-adaptation-field-size", -1);
    getIntValue(max_af, u"max-adaptation-field-size", -1);
    getIntValue(after_packets, u"after-packets");
    getIntValue(every_packets, u"every");
    getIntValues(pid, u"pid");
    getIntValues(labels, u"label");
    getIntValues(set_labels, u"set-label");
    getIntValues(reset_labels, u"reset-label");

    // Status for unselected packets.
    if (set_labels.any() || reset_labels.any()) {
        // Do not drop unselected packets, simply set/reset labels on selected packets.
        drop_status = TSP_OK;
    }
    else if (present(u"stuffing")) {
        // Replace unselected packets with stuffing.
        drop_status = TSP_NULL;
    }
    else {
        // Drop unselected packets.
        drop_status = TSP_DROP;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::FilterPlugin::start()
{
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::FilterPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Pass initial packets without filtering.
    if (tsp->pluginPackets() < after_packets) {
        return TSP_OK;
    }

    // Check if the packet matches one of the selected criteria.
    bool ok = pid[pkt.getPID()] ||
        (with_payload && pkt.hasPayload()) ||
        (with_af && pkt.hasAF()) ||
        (unit_start && pkt.getPUSI()) ||
        (nullified && pkt_data.getNullified()) ||
        (input_stuffing && pkt_data.getInputStuffing()) ||
        (valid && pkt.hasValidSync() && !pkt.getTEI()) ||
        (scrambling_ctrl == pkt.getScrambling()) ||
        (has_pcr && (pkt.hasPCR() || pkt.hasOPCR())) ||
        (min_payload >= 0 && int (pkt.getPayloadSize()) >= min_payload) ||
        (int (pkt.getPayloadSize()) <= max_payload) ||
        (min_af >= 0 && int (pkt.getAFSize()) >= min_af) ||
        (int (pkt.getAFSize()) <= max_af) ||
        pkt_data.hasAnyLabel(labels) ||
        (every_packets > 0 && (tsp->pluginPackets() - after_packets) % every_packets == 0) ||

        // Check the presence of a PES header.
        // A PES header starts with the 3-byte prefix 0x000001. A packet has a PES
        // header if the 'payload unit start' is set in the TS header and the
        // payload starts with 0x000001.
        //
        // Note that there is no risk to misinterpret the prefix: When 'payload
        // unit start' is set, the payload may also contains PSI/SI tables. In
        // that case, 0x000001 is not a possible value for the beginning of the
        // payload. With PSI/SI, a payload starting with 0x000001 would mean:
        //  0x00 : pointer field -> a section starts at next byte
        //  0x00 : table id -> a PAT
        //  0x01 : section_syntax_indicator field is 0, impossible for a PAT

        (with_pes && pkt.hasValidSync() && !pkt.getTEI() && pkt.getPayloadSize() >= 3 &&
         (GetUInt32 (pkt.b + pkt.getHeaderSize() - 1) & 0x00FFFFFF) == 0x000001);

    // Reverse selection criteria with --negate.
    if (negate) {
        ok = !ok;
    }

    if (ok) {
        pkt_data.setLabels(set_labels);
        pkt_data.clearLabels(reset_labels);
        return TSP_OK;
    }
    else {
        return drop_status;
    }
}
