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
        int           scrambling_ctrl;  // Scrambling control value (<0: no filter)
        bool          with_payload;     // Packets with payload
        bool          with_af;          // Packets with adaptation field
        bool          with_pes;         // Packets with clear PES headers
        bool          has_pcr;          // Packets with PCR or OPCR
        bool          unit_start;       // Packets with payload unit start
        bool          valid;            // Packets with valid sync byte and error ind
        bool          negate;           // Negate filter (exclude selected packets)
        bool          stuffing;         // Replace excluded packet with stuffing
        int           min_payload;      // Minimum payload size (<0: no filter)
        int           max_payload;      // Maximum payload size (<0: no filter)
        int           min_af;           // Minimum adaptation field size (<0: no filter)
        int           max_af;           // Maximum adaptation field size (<0: no filter)
        PacketCounter after_packets;
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
    scrambling_ctrl(0),
    with_payload(false),
    with_af(false),
    with_pes(false),
    has_pcr(false),
    unit_start(false),
    valid(false),
    negate(false),
    stuffing(false),
    min_payload(0),
    max_payload(0),
    min_af(0),
    max_af(0),
    after_packets(0),
    pid()
{
    option(u"adaptation-field",          0);
    option(u"after-packets",             0,  UNSIGNED);
    option(u"clear",                    'c');
    option(u"max-adaptation-field-size", 0,  INTEGER, 0, 1, 0, 184);
    option(u"max-payload-size",          0,  INTEGER, 0, 1, 0, 184);
    option(u"min-adaptation-field-size", 0,  INTEGER, 0, 1, 0, 184);
    option(u"min-payload-size",          0,  INTEGER, 0, 1, 0, 184);
    option(u"negate",                   'n');
    option(u"payload",                   0);
    option(u"pcr",                       0);
    option(u"pes",                       0);
    option(u"pid",                      'p', PIDVAL, 0, UNLIMITED_COUNT);
    option(u"scrambling-control",        0,  INTEGER, 0, 1, 0, 3);
    option(u"stuffing",                 's');
    option(u"unit-start",                0);
    option(u"valid",                    'v');

    setHelp(u"Options:\n"
            u"\n"
            u"  --adaptation-field\n"
            u"      Select packets with an adaptation field.\n"
            u"\n"
            u"  --after-packets count\n"
            u"      Let the first 'count' packets pass transparently without filtering. Start\n"
            u"      to apply the filtering criteria after that number of packets.\n"
            u"\n"
            u"  -c\n"
            u"  --clear\n"
            u"      Select clear (unscrambled) packets.\n"
            u"      Equivalent to --scrambling-control 0.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --max-adaptation-field-size value\n"
            u"      Select packets with no adaptation field or with an adaptation field the\n"
            u"      size (in bytes) of which is not greater than the specified value.\n"
            u"\n"
            u"  --max-payload-size value\n"
            u"      Select packets with no payload or with a payload the size (in bytes) of\n"
            u"      which is not greater than the specified value.\n"
            u"\n"
            u"  --min-adaptation-field-size value\n"
            u"      Select packets with an adaptation field the size (in bytes) of which\n"
            u"      is equal to or greater than the specified value.\n"
            u"\n"
            u"  --min-payload-size value\n"
            u"      Select packets with a payload the size (in bytes) of which is equal\n"
            u"      to or greater than the specified value.\n"
            u"\n"
            u"  -n\n"
            u"  --negate\n"
            u"      Negate the filter: specified packets are excluded.\n"
            u"\n"
            u"  --payload\n"
            u"      Select packets with a payload.\n"
            u"\n"
            u"  --pcr\n"
            u"      Select packets with PCR or OPCR.\n"
            u"\n"
            u"  --pes\n"
            u"      Select packets with clear PES headers.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      PID filter: select packets with this PID value.\n"
            u"      Several -p or --pid options may be specified.\n"
            u"\n"
            u"  --scrambling-control value\n"
            u"      Select packets with the specified scrambling control value. Valid\n"
            u"      values are 0 (clear), 1 (reserved), 2 (even key), 3 (odd key).\n"
            u"\n"
            u"  -s\n"
            u"  --stuffing\n"
            u"      Replace excluded packets with stuffing (null packets) instead\n"
            u"      of removing them. Useful to preserve bitrate.\n"
            u"\n"
            u"  --unit-start\n"
            u"      Select packets with payload unit start indicator.\n"
            u"\n"
            u"  -v\n"
            u"  --valid\n"
            u"      Select valid packets. A valid packet starts with 0x47 and has\n"
            u"      its transport_error_indicator cleared.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::FilterPlugin::start()
{
    scrambling_ctrl = present(u"clear") ? 0 : intValue(u"scrambling-control", -1);
    with_payload = present(u"payload");
    with_af = present(u"adaptation-field");
    with_pes = present(u"pes");
    has_pcr = present(u"pcr");
    unit_start = present(u"unit-start");
    valid = present(u"valid");
    negate = present(u"negate");
    stuffing = present(u"stuffing");
    min_payload = intValue<int>(u"min-payload-size", -1);
    max_payload = intValue<int>(u"max-payload-size", -1);
    min_af = intValue<int>(u"min-adaptation-field-size", -1);
    max_af = intValue<int>(u"max-adaptation-field-size", -1);
    after_packets = intValue<PacketCounter>(u"after-packets", 0);

    getPIDSet(pid, u"pid");

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
        return TSP_OK;
    }
    else if (stuffing) {
        return TSP_NULL;
    }
    else {
        return TSP_DROP;
    }
}
