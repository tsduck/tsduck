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
//  Filter TS packets.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsMemory.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class FilterPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(FilterPlugin);
    public:
        // Implementation of plugin API
        FilterPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Packet intervals and list of them.
        typedef std::pair<PacketCounter, PacketCounter> PacketRange;
        typedef std::list<PacketRange> PacketRangeList;

        // Command line options:
        Status          _drop_status;        // Return status for unselected packets
        int             _scrambling_ctrl;    // Scrambling control value (<0: no filter)
        bool            _with_payload;       // Packets with payload
        bool            _with_af;            // Packets with adaptation field
        bool            _with_pes;           // Packets with clear PES headers
        bool            _with_pcr;           // Packets with PCR or OPCR
        bool            _with_splice;        // Packets with splice_countdown in adaptation field
        bool            _unit_start;         // Packets with payload unit start
        bool            _nullified;          // Packets which were nullified by a previous plugin
        bool            _input_stuffing;     // Null packets which were artificially inserted
        bool            _valid;              // Packets with valid sync byte and error ind
        bool            _negate;             // Negate filter (exclude selected packets)
        int             _min_payload;        // Minimum payload size (<0: no filter)
        int             _max_payload;        // Maximum payload size (<0: no filter)
        int             _min_af;             // Minimum adaptation field size (<0: no filter)
        int             _max_af;             // Maximum adaptation field size (<0: no filter)
        int             _splice;             // Exact splice_countdown value (<-128: no filter)
        int             _min_splice;         // Minimum splice_countdown value (<-128: no filter)
        int             _max_splice;         // Maximum splice_countdown value (<-128: no filter)
        PacketCounter   _after_packets;      // Number of initial packets to skip
        PacketCounter   _every_packets;      // Filter 1 out of this number of packets
        PIDSet          _explicit_pid;       // Explicit PID values to filter
        ByteBlock       _pattern;            // Byte pattern to search.
        bool            _search_payload;     // Search pattern in payload only.
        bool            _use_search_offset;  // Search at specified offset only.
        size_t          _search_offset;      // Offset where to search.
        PacketRangeList _ranges;             // Ranges of packets to filter.
        std::set<uint8_t>          _stream_ids;        // PES stream ids to filter
        TSPacketMetadata::LabelSet _labels;            // Select packets with any of these labels
        TSPacketMetadata::LabelSet _set_labels;        // Labels to set on filtered packets
        TSPacketMetadata::LabelSet _reset_labels;      // Labels to reset on filtered packets
        TSPacketMetadata::LabelSet _set_perm_labels;   // Labels to set on all packets after getting one packet
        TSPacketMetadata::LabelSet _reset_perm_labels; // Labels to reset on all packets after getting one packet

        // Working data:
        PacketCounter   _filtered_packets;   // Number of filtered packets
        PIDSet          _stream_id_pid;      // PID values selected from stream ids.
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"filter", ts::FilterPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::FilterPlugin::FilterPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Filter TS packets according to various conditions", u"[options]"),
    _drop_status(TSP_DROP),
    _scrambling_ctrl(0),
    _with_payload(false),
    _with_af(false),
    _with_pes(false),
    _with_pcr(false),
    _with_splice(false),
    _unit_start(false),
    _nullified(false),
    _input_stuffing(false),
    _valid(false),
    _negate(false),
    _min_payload(0),
    _max_payload(0),
    _min_af(0),
    _max_af(0),
    _splice(0),
    _min_splice(0),
    _max_splice(0),
    _after_packets(0),
    _every_packets(0),
    _explicit_pid(),
    _pattern(),
    _search_payload(false),
    _use_search_offset(false),
    _search_offset(0),
    _ranges(),
    _stream_ids(),
    _labels(),
    _set_labels(),
    _reset_labels(),
    _set_perm_labels(),
    _reset_perm_labels(),
    _filtered_packets(0),
    _stream_id_pid()
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

    option(u"interval", 'i', STRING, 0, UNLIMITED_COUNT);
    help(u"interval", u"index1[-[index2]]",
         u"Select all packets in the specified interval from the start of the stream. "
         u"The packets in the stream are indexed starting at zero. "
         u"In the form 'index1', only one packet is selected, at the specified index. "
         u"In the form 'index1-index2', all packets in the specified range of indexes, inclusive, are selected. "
         u"In the form 'index1-', all packets starting at the specified index are selected, up to the end of the stream. "
         u"Several options --interval can be specified.");

    option(u"label", 'l', INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"label", u"label1[-label2]",
         u"Select packets with any of the specified labels. "
         u"Labels should have typically been set by a previous plugin in the chain. "
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

    option(u"pattern", 0, STRING);
    help(u"pattern",
         u"Select packets containing the specified pattern bytes. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"By default, the packet is selected when the value is anywhere inside the packet. "
         u"With option --search-payload, only search the pattern in the payload of the packet. "
         u"With option --search-offset, the packet is selected only if the pattern "
         u"is at the specified offset in the packet. "
         u"When --search-payload and --search-offset are both specified, the packet "
         u"is selected only if the pattern is at the specified offset in the payload.");

    option(u"search-payload");
    help(u"search-payload",
         u"With --pattern, only search the set of bytes in the payload of the packet. "
         u"Do not search the pattern in the header or adaptation field.");

    option(u"search-offset", 0, INTEGER, 0, 1, 0, PKT_SIZE - 1);
    help(u"search-offset",
         u"With --pattern, only search the set of bytes at the specified offset in the packet "
         u"(the default) or in the payload (with --search-payload).");

    option(u"scrambling-control", 0, INTEGER, 0, 1, 0, 3);
    help(u"scrambling-control",
         u"Select packets with the specified scrambling control value. Valid "
         u"values are 0 (clear), 1 (reserved), 2 (even key), 3 (odd key).");

    option(u"stream-id", 0, UINT8, 0, UNLIMITED_COUNT);
    help(u"stream-id", u"id1[-id2]",
         u"Select PES PID's with any of the specified stream ids. "
         u"A PID starts to be selected when a specified stream id appears. "
         u"Such a PID is no longer selected when non-specified stream id is found. "
         u"Several --stream-id options may be specified.");

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

    option(u"set-permanent-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"set-permanent-label", u"label1[-label2]",
         u"Set the specified labels on all packets, selected and unselected ones, after at least one was selected. "
         u"Do not drop unselected packets, simply use selected ones as trigger. "
         u"Several --set-permanent-label options may be specified.");

    option(u"has-splice-countdown");
    help(u"has-splice-countdown", u"Select packets which contain a splice_countdown value in adaptation field.");

    option(u"splice-countdown", 0, INT8);
    help(u"splice-countdown", u"Select packets with the specified splice_countdown value in adaptation field.");

    option(u"min-splice-countdown", 0, INT8);
    help(u"min-splice-countdown",
         u"Select packets with a splice_countdown value in adaptation field which is "
         u"greater than or equal to the specified value.");

    option(u"max-splice-countdown", 0, INT8);
    help(u"max-splice-countdown",
         u"Select packets with a splice_countdown value in adaptation field which is "
         u"lower than or equal to the specified value.");

    option(u"reset-permanent-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"reset-permanent-label", u"label1[-label2]",
         u"Clear the specified labels on all packets, selected and unselected ones, after at least one was selected. "
         u"Do not drop unselected packets, simply use selected ones as trigger. "
         u"Several --reset-permanent-label options may be specified.");

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
    _scrambling_ctrl = present(u"clear") ? 0 : intValue(u"scrambling-control", -1);
    _with_payload = present(u"payload");
    _with_af = present(u"adaptation-field");
    _with_pes = present(u"pes");
    _with_pcr = present(u"pcr");
    _with_splice = present(u"has-splice-countdown");
    _unit_start = present(u"unit-start");
    _nullified = present(u"nullified");
    _input_stuffing = present(u"input-stuffing");
    _valid = present(u"valid");
    _negate = present(u"negate");
    getIntValue(_min_payload, u"min-payload-size", -1);
    getIntValue(_max_payload, u"max-payload-size", -1);
    getIntValue(_min_af, u"min-adaptation-field-size", -1);
    getIntValue(_max_af, u"max-adaptation-field-size", -1);
    getIntValue(_splice, u"splice-countdown", INT_MIN);
    getIntValue(_min_splice, u"min-splice-countdown", INT_MIN);
    getIntValue(_max_splice, u"max-splice-countdown", INT_MIN);
    getIntValue(_after_packets, u"after-packets");
    getIntValue(_every_packets, u"every");
    getIntValues(_explicit_pid, u"pid");
    getIntValues(_stream_ids, u"stream-id");
    getIntValues(_labels, u"label");
    getIntValues(_set_labels, u"set-label");
    getIntValues(_reset_labels, u"reset-label");
    getIntValues(_set_perm_labels, u"set-permanent-label");
    getIntValues(_reset_perm_labels, u"reset-permanent-label");
    _search_payload = present(u"search-payload");
    _use_search_offset = present(u"search-offset");
    getIntValue(_search_offset, u"search-offset");

    if (!value(u"pattern").hexaDecode(_pattern)) {
        tsp->error(u"invalid hexadecimal pattern");
        return false;
    }

    // Decode all index ranges.
    _ranges.clear();
    UStringVector intervals;
    getValues(intervals, u"interval");
    for (auto it = intervals.begin(); it != intervals.end(); ++it) {
        PacketCounter first = 0;
        PacketCounter second = 0;
        if (it->scan(u"%d-%d", {&first, &second})) {
            _ranges.push_back(std::make_pair(first, second));
        }
        else if (it->scan(u"%d-", {&first})) {
            _ranges.push_back(std::make_pair(first, std::numeric_limits<PacketCounter>::max()));
        }
        else if (it->scan(u"%d", {&first})) {
            _ranges.push_back(std::make_pair(first, first));
        }
        else {
            tsp->error(u"invalid packet range %s", {*it});
            return false;
        }
    }

    // Check that the pattern to search is not larger than the packet.
    if (_pattern.size() > PKT_SIZE || (_use_search_offset && _search_offset + _pattern.size() > PKT_SIZE)) {
        tsp->error(u"search pattern too large for TS packets");
        return false;
    }

    // Status for unselected packets.
    if (_set_labels.any() || _reset_labels.any() || _set_perm_labels.any() || _reset_perm_labels.any()) {
        // Do not drop unselected packets, simply set/reset labels on selected packets.
        _drop_status = TSP_OK;
    }
    else if (present(u"stuffing")) {
        // Replace unselected packets with stuffing.
        _drop_status = TSP_NULL;
    }
    else {
        // Drop unselected packets.
        _drop_status = TSP_DROP;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method.
//----------------------------------------------------------------------------

bool ts::FilterPlugin::start()
{
    _filtered_packets = 0;
    _stream_id_pid.reset();
    return true;
}


//----------------------------------------------------------------------------
// Stop method.
//----------------------------------------------------------------------------

bool ts::FilterPlugin::stop()
{
    tsp->debug(u"%'d / %'d filtered packets", {_filtered_packets, tsp->pluginPackets()});
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::FilterPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Pass initial packets without filtering.
    const PacketCounter packetIndex = tsp->pluginPackets();
    if (packetIndex < _after_packets) {
        return TSP_OK;
    }

    // Check stream ids of PES packets. The stream id is in the fourth byte of
    // the payload of a TS packet containing the start of a PES packet.
    if (!_stream_ids.empty() && pkt.startPES() && pkt.getPayloadSize() >= 4) {
        const uint8_t id = pkt.getPayload()[3];
        const bool selected = _stream_ids.find(id) != _stream_ids.end();
        _stream_id_pid.set(pid, selected);
    }

    // Check if the packet matches one of the selected criteria.
    bool ok = _explicit_pid[pid] ||
        _stream_id_pid[pid] ||
        (_with_payload && pkt.hasPayload()) ||
        (_with_af && pkt.hasAF()) ||
        (_unit_start && pkt.getPUSI()) ||
        (_nullified && pkt_data.getNullified()) ||
        (_input_stuffing && pkt_data.getInputStuffing()) ||
        (_valid && pkt.hasValidSync() && !pkt.getTEI()) ||
        (_scrambling_ctrl == pkt.getScrambling()) ||
        (_with_pcr && (pkt.hasPCR() || pkt.hasOPCR())) ||
        (_with_splice && pkt.hasSpliceCountdown()) ||
        (_splice >= -128 && pkt.hasSpliceCountdown() && pkt.getSpliceCountdown() == _splice) ||
        (_min_splice >= -128 && pkt.hasSpliceCountdown() && pkt.getSpliceCountdown() >= _min_splice) ||
        (_max_splice >= -128 && pkt.hasSpliceCountdown() && pkt.getSpliceCountdown() <= _max_splice) ||
        (_min_payload >= 0 && int(pkt.getPayloadSize()) >= _min_payload) ||
        (int(pkt.getPayloadSize()) <= _max_payload) ||
        (_min_af >= 0 && int(pkt.getAFSize()) >= _min_af) ||
        (int(pkt.getAFSize()) <= _max_af) ||
        pkt_data.hasAnyLabel(_labels) ||
        (_every_packets > 0 && (tsp->pluginPackets() - _after_packets) % _every_packets == 0) ||
        (_with_pes && pkt.startPES());

    // Search binary patterns in packets.
    if (!ok && !_pattern.empty()) {
        const size_t start = _search_payload ? pkt.getHeaderSize() : 0;
        if (start + _search_offset + _pattern.size() <= PKT_SIZE) {
            if (_use_search_offset) {
                ok = ::memcmp(pkt.b + start + _search_offset, _pattern.data(), _pattern.size()) == 0;
            }
            else {
                ok = LocatePattern(pkt.b + start, PKT_SIZE - start, _pattern.data(), _pattern.size()) != nullptr;
            }
        }
    }

    // Search if packet is in one selected range.
    for (auto it = _ranges.begin(); !ok && it != _ranges.end(); ++it) {
        ok = packetIndex >= it->first && packetIndex <= it->second;
    }

    // Reverse selection criteria with --negate.
    if (_negate) {
        ok = !ok;
    }

    // Set/reset labels on filtered packets.
    if (ok) {
        _filtered_packets++;
        pkt_data.setLabels(_set_labels);
        pkt_data.clearLabels(_reset_labels);
    }

    // Set/reset permanent labels on all packets once at least one was filtered.
    if (_filtered_packets > 0) {
        pkt_data.setLabels(_set_perm_labels);
        pkt_data.clearLabels(_reset_perm_labels);
    }

    return ok ? TSP_OK : _drop_status;
}
