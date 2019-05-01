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
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        bool          only_with_mark;   // Check criteria only if it's marked
        int           scrambling_ctrl;  // Scrambling control value (<0: no filter)
        bool          with_payload;     // Packets with payload
        bool          with_af;          // Packets with adaptation field
        bool          with_pes;         // Packets with clear PES headers
        bool          has_pcr;          // Packets with PCR or OPCR
        bool          unit_start;       // Packets with payload unit start
        bool          valid;            // Packets with valid sync byte and error ind
        bool          negate;           // Negate filter (exclude selected packets)
        bool          stuffing;         // Replace excluded packet with stuffing
        int           every_packets;    // Number of times to trigger the condition
        int           inplace_remap;    // Target pid when doing in-place remap
        int           min_payload;      // Minimum payload size (<0: no filter)
        int           max_payload;      // Maximum payload size (<0: no filter)
        int           min_af;           // Minimum adaptation field size (<0: no filter)
        int           max_af;           // Maximum adaptation field size (<0: no filter)
        bool          marking_only;     // Instead of filter/process mark it only
        PacketCounter after_packets;
        PacketCounter ok_packets;
        PIDSet        pid;              // PID values to filter

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
    only_with_mark(false),
    scrambling_ctrl(0),
    with_payload(false),
    with_af(false),
    with_pes(false),
    has_pcr(false),
    unit_start(false),
    valid(false),
    negate(false),
    stuffing(false),
    every_packets(0),
    inplace_remap(-1),
    min_payload(0),
    max_payload(0),
    min_af(0),
    max_af(0),
    marking_only(false),
    after_packets(0),
    ok_packets(0),
    pid()
{
    option(u"only-with-mark", 'o');
    help(u"only-with-mark", u"Requires a previous mark to check the criteria. "
         u"If no mark, then pass the packet directly.");

    option(u"mark-only", 'm');
    help(u"mark-only", u"Instead of filter/process the packet, mark it only.");

    option(u"every", 'e', INTEGER, 0, 1, 1, UNLIMITED_VALUE);
    help(u"every",
         u"Triggers the condition only after the number of times indicated. "
         u"Default is 1, so each time the condition is true.");

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

    option(u"in-place-process-remap", 0, INTEGER, 0, 1, 0, PID_NULL - 1);
    help(u"in-place-process-remap",
         u"Instead of filter out, process the packet local and pass it. "
         u"Process to do: Remap the pid to the indicated value.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::FilterPlugin::start()
{
    only_with_mark = present(u"only-with-mark");
    marking_only = present(u"mark-only");
    scrambling_ctrl = present(u"clear") ? 0 : intValue(u"scrambling-control", -1);
    with_payload = present(u"payload");
    with_af = present(u"adaptation-field");
    with_pes = present(u"pes");
    has_pcr = present(u"pcr");
    unit_start = present(u"unit-start");
    valid = present(u"valid");
    negate = present(u"negate");
    stuffing = present(u"stuffing");
    inplace_remap = intValue<int>(u"in-place-process-remap", -1);
    every_packets = intValue<int>(u"every", 1);
    min_payload = intValue<int>(u"min-payload-size", -1);
    max_payload = intValue<int>(u"max-payload-size", -1);
    min_af = intValue<int>(u"min-adaptation-field-size", -1);
    max_af = intValue<int>(u"max-adaptation-field-size", -1);
    after_packets = intValue<PacketCounter>(u"after-packets", 0);

    getIntValues(pid, u"pid");

    ok_packets = every_packets;

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::FilterPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Pass initial packets without filtering.
    if (after_packets > 0) {
        after_packets--;
        return TSP_OK;
    }

    if (only_with_mark) {
        // If no Mark then pass it directly!
        if (pkt.b[0] != 0xff) {
            return TSP_OK;
        }
    }

    // Check if the packet matches one of the selected criteria.

    bool ok = pid[pkt.getPID()] ||
        (with_payload && pkt.hasPayload()) ||
        (with_af && pkt.hasAF()) ||
        (unit_start && pkt.getPUSI()) ||
        (valid && pkt.hasValidSync() && !pkt.getTEI()) ||
        (scrambling_ctrl == pkt.getScrambling()) ||
        (has_pcr && (pkt.hasPCR() || pkt.hasOPCR())) ||
        (min_payload >= 0 && int (pkt.getPayloadSize()) >= min_payload) ||
        (int (pkt.getPayloadSize()) <= max_payload) ||
        (min_af >= 0 && int (pkt.getAFSize()) >= min_af) ||
        (int (pkt.getAFSize()) <= max_af) ||

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

    if (negate) {
        ok = !ok;
    }

    if (ok) {
        ok_packets--;
        if (ok_packets == 0) {
            ok_packets = every_packets;
        }
        else {
            ok = !ok;
        }
    }

    if (marking_only) {
        if (ok) {
            pkt.b[0] = 0xff; //TSP_MARK;
            return TSP_OK;
        }
        else {
            pkt.b[0] = 0x47;
            return TSP_OK;
        }
    }
    else {
        pkt.b[0] = 0x47;
    }

    if (inplace_remap >= 0) {
        if (ok) {
            pkt.setPID(inplace_remap);
        }
        return TSP_OK;
    }

    if (ok) {
        return TSP_OK;
    }
    else if (stuffing) {
        return TSP_NULL;
    }
    else {
        return TSP_DROP;
    }
}
