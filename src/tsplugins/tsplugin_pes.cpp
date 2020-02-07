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
//  Analyze PES packets.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsPESDemux.h"
#include "tsAVCSequenceParameterSet.h"
#include "tsNames.h"
#include "tsMemory.h"
#include "tsBitStream.h"
#include <sstream>
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PESPlugin: public ProcessorPlugin, private PESHandlerInterface
    {
        TS_NOBUILD_NOCOPY(PESPlugin);
    public:
        // Implementation of plugin API
        PESPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // PESPlugin private members
        bool            _abort;
        std::ofstream   _outfile;
        bool            _trace_packets;
        bool            _trace_packet_index;
        bool            _dump_pes_header;
        bool            _dump_pes_payload;
        bool            _dump_start_code;
        bool            _dump_nal_units;
        bool            _dump_avc_sei;
        std::bitset<32> _nal_unit_filter;
        std::set<uint32_t>   _sei_type_filter;
        std::list<ByteBlock> _sei_uuid_filter;
        bool            _video_attributes;
        bool            _audio_attributes;
        size_t          _max_dump_size;
        size_t          _max_dump_count;
        uint32_t        _hexa_flags;
        size_t          _hexa_bpl;
        int             _min_payload;    // Minimum payload size (<0: no filter)
        int             _max_payload;    // Maximum payload size (<0: no filter)
        PESDemux        _demux;

        // Process dump count. Return true when terminated. Also process error on output.
        bool lastDump(std::ostream&);

        // Hooks
        virtual void handlePESPacket (PESDemux&, const PESPacket&) override;
        virtual void handleVideoStartCode (PESDemux&, const PESPacket&, uint8_t, size_t, size_t) override;
        virtual void handleNewVideoAttributes (PESDemux&, const PESPacket&, const VideoAttributes&) override;
        virtual void handleAVCAccessUnit (PESDemux&, const PESPacket&, uint8_t, size_t, size_t) override;
        virtual void handleSEI(PESDemux& demux, const PESPacket& packet, uint32_t sei_type, size_t offset, size_t size) override;
        virtual void handleNewAVCAttributes (PESDemux&, const PESPacket&, const AVCAttributes&) override;
        virtual void handleNewAudioAttributes (PESDemux&, const PESPacket&, const AudioAttributes&) override;
        virtual void handleNewAC3Attributes (PESDemux&, const PESPacket&, const AC3Attributes&) override;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(pes, ts::PESPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PESPlugin::PESPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Analyze PES packets", u"[options]"),
    _abort(false),
    _outfile(),
    _trace_packets(false),
    _trace_packet_index(false),
    _dump_pes_header(false),
    _dump_pes_payload(false),
    _dump_start_code(false),
    _dump_nal_units(false),
    _dump_avc_sei(false),
    _nal_unit_filter(),
    _sei_type_filter(),
    _sei_uuid_filter(),
    _video_attributes(false),
    _audio_attributes(false),
    _max_dump_size(0),
    _max_dump_count(0),
    _hexa_flags(0),
    _hexa_bpl(0),
    _min_payload(0),
    _max_payload(0),
    _demux(duck, this)
{
    option(u"audio-attributes", 'a');
    help(u"audio-attributes", u"Display audio attributes.");

    option(u"avc-access-unit");
    help(u"avc-access-unit", u"Dump all AVC (ISO 14496-10, ITU H.264) access units (aka \"NALunits\").");

    option(u"binary", 'b');
    help(u"binary", u"Include binary dump in addition to hexadecimal.");

    option(u"header", 'h');
    help(u"header", u"Dump PES packet header.");

    option(u"max-dump-count", 'x', UNSIGNED);
    help(u"max-dump-count",
         u"Specify the maximum number of times data dump occurs with options "
         u"--trace-packets, --header, --payload, --start-code, --avc-access-unit. "
         u"Default: unlimited.");

    option(u"max-dump-size", 'm', UNSIGNED);
    help(u"max-dump-size",
         u"Specify the maximum dump size for options --header, --payload, "
         u"--start-code, --avc-access-unit.");

    option(u"max-payload-size", 0, UNSIGNED);
    help(u"max-payload-size",
         u"Display PES packets with no payload or with a payload the size (in bytes) "
         u"of which is not greater than the specified value.");

    option(u"min-payload-size", 0, UNSIGNED);
    help(u"min-payload-size",
         u"Display PES packets with a payload the size (in bytes) of which is equal "
         u"to or greater than the specified value.");

    option(u"nal-unit-type", 0, INTEGER, 0, UNLIMITED_COUNT, 0, 31);
    help(u"nal-unit-type",
         u"AVC NALunit filter: with --avc-access-unit, select access units with "
         u"this type (default: all access units). Several --nal-unit-type options "
         u"may be specified.");

    option(u"negate-nal-unit-type");
    help(u"negate-nal-unit-type",
         u"Negate the AVC NALunit filter: specified access units are excluded.");

    option(u"negate-pid", 'n');
    help(u"negate-pid",
         u"Negate the PID filter: specified PID's are excluded.");

    option(u"nibble");
    help(u"nibble",
         u"Same as --binary but add separator between 4-bit nibbles.");

    option(u"output-file", 'o', STRING);
    help(u"output-file", u"filename",
         u"Specify the output file for the report (default: standard output).");

    option(u"packet-index");
    help(u"packet-index",
         u"Display the index of the first and last TS packet of each displayed PES packet.");

    option(u"payload", 0);
    help(u"payload", u"Dump PES packet payload.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"PID filter: select packets with these PID values (default: all PID's "
         u"containing PES packets). Several -p or --pid options may be specified.");

    option(u"sei-avc");
    help(u"sei-avc", u"Dump all SEI (Supplemental Enhancement Information) in AVC/H.264 access units.");

    option(u"start-code", 's');
    help(u"start-code", u"Dump all start codes in PES packet payload.");

    option(u"trace-packets", 't');
    help(u"trace-packets", u"Trace all PES packets.");

    option(u"sei-type", 0, UINT32);
    help(u"sei-type",
         u"SEI type filter: with --sei-avc, select SEI access units with this "
         u"type (default: all SEI access units). Several --sei-type options "
         u"may be specified.");

    option(u"uuid-sei", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"uuid-sei",
         u"AVC SEI filter: with --sei-avc, select \"user data unregistered\" SEI "
         u"access units with the specified UUID value (default: all SEI). Several "
         u"--uuid-sei options may be specified. The UUID value must be 16 bytes long. "
         u"It must be either an ASCII string of exactly 16 characters or a hexa- "
         u"decimal value representing 16 bytes.");

    option(u"video-attributes", 'v');
    help(u"video-attributes", u"Display video attributes.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PESPlugin::start()
{
    // Options
    _dump_pes_header = present(u"header");
    _dump_pes_payload = present(u"payload");
    _trace_packets = present(u"trace-packets") || _dump_pes_header || _dump_pes_payload;
    _trace_packet_index = present(u"packet-index");
    _dump_start_code = present(u"start-code");
    _dump_nal_units = present(u"avc-access-unit");
    _dump_avc_sei = present(u"sei-avc");
    _video_attributes = present(u"video-attributes");
    _audio_attributes = present(u"audio-attributes");
    _max_dump_size = intValue<size_t>(u"max-dump-size", 0);
    _max_dump_count = intValue<size_t>(u"max-dump-count", 0);
    _min_payload = intValue<int>(u"min-payload-size", -1);
    _max_payload = intValue<int>(u"max-payload-size", -1);

    // Hexa dump flags and bytes-per-line
    _hexa_flags = UString::HEXA | UString::OFFSET | UString::BPL;
    _hexa_bpl = 16;
    if (present(u"binary")) {
        _hexa_flags |= UString::BINARY;
        _hexa_bpl = 8;
    }
    if (present(u"nibble")) {
        _hexa_flags |= UString::BIN_NIBBLE;
        _hexa_bpl = 8;
    }

    // PID values to filter
    if (present(u"pid")) {
        PIDSet pids;
        getIntValues(pids, u"pid");
        if (present(u"negate-pid")) {
            pids.flip();
        }
        _demux.setPIDFilter(pids);
    }
    else {
        _demux.setPIDFilter(AllPIDs);
    }

    // AVC NALunits to filter
    const size_t nal_count = count(u"nal-unit-type");
    if (nal_count == 0) {
        // Default: all NALunits
        _nal_unit_filter.set();
    }
    else {
        _nal_unit_filter.reset();
        for (size_t n = 0; n < nal_count; n++) {
            _nal_unit_filter.set(intValue<size_t>(u"nal-unit-type", 0, n));
        }
        if (present(u"negate-nal-unit-type")) {
            _nal_unit_filter.flip();
        }
    }

    // SEI types to filter
    getIntValues(_sei_type_filter, u"sei-type");

    // SEI UUID's to filter.
    const size_t uuid_count = count(u"uuid-sei");
    _sei_uuid_filter.clear();
    for (size_t n = 0; n < uuid_count; n++) {
        const UString uuid(value(u"uuid-sei"));
        ByteBlock bytes;
        // Try to use parameter value as 16-char string or 16-byte hexa string.
        bytes.appendUTF8(uuid);
        if (bytes.size() == AVC_SEI_UUID_SIZE || (uuid.hexaDecode(bytes) && bytes.size() == AVC_SEI_UUID_SIZE)) {
            _sei_uuid_filter.push_back(bytes);
        }
        else {
            error(u"invalid UUID \"%s\"", {uuid});
            return false;
        }
    }

    // Create output file
    if (present(u"output-file")) {
        const UString name(value(u"output-file"));
        tsp->verbose(u"creating %s", {name});
        _outfile.open(name.toUTF8().c_str(), std::ios::out);
        if (!_outfile) {
            error(u"cannot create %s", {name});
            return false;
        }
    }

    _abort = false;
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PESPlugin::stop()
{
    // Close output file
    if (_outfile.is_open()) {
        _outfile.close();
    }

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PESPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _demux.feedPacket(pkt);
    return _abort ? TSP_END : TSP_OK;
}


//----------------------------------------------------------------------------
// Process dump count. Return true when terminated.
//----------------------------------------------------------------------------

bool ts::PESPlugin::lastDump(std::ostream& out)
{
    if (!out || (_max_dump_count != 0 && _max_dump_count-- == 1)) {
        return _abort = true;
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete PES packet is available.
//----------------------------------------------------------------------------

void ts::PESPlugin::handlePESPacket(PESDemux&, const PESPacket& pkt)
{
    std::ostream& out(_outfile.is_open() ? _outfile : std::cout);

    // Skip PES packets without appropriate payload size
    if (int(pkt.payloadSize()) < _min_payload || (_max_payload >= 0 && int(pkt.payloadSize()) > _max_payload)) {
        return;
    }

    // Report packet description
    if (_trace_packets) {
        UString line(UString::Format(u"PID 0x%X, stream_id %s, size: %d bytes (header: %d, payload: %d)",
                                     {pkt.getSourcePID(),
                                      names::StreamId(pkt.getStreamId(), names::FIRST),
                                      pkt.size(), pkt.headerSize(), pkt.payloadSize()}));
        const size_t spurious = pkt.spuriousDataSize();
        if (spurious > 0) {
            line.append(UString::Format(u", %d spurious trailing bytes", {spurious}));
        }
        out << "* " << line << std::endl;
        if (lastDump(out)) {
            return;
        }
    }

    // Report TS packet index
    if (_trace_packet_index) {
        out << UString::Format(u"  First TS packet: %'d, last: %'d", {pkt.getFirstTSPacketIndex(), pkt.getLastTSPacketIndex()}) << std::endl;
    }

    // Report PES header
    if (_dump_pes_header) {
        size_t size = pkt.headerSize();
        out << "  PES header";
        if (_max_dump_size > 0 && size > _max_dump_size) {
            size = _max_dump_size;
            out << " (truncated)";
        }
        out << ":" << std::endl << UString::Dump(pkt.header(), size, _hexa_flags, 4, _hexa_bpl);
        if (lastDump (out)) {
            return;
        }
    }

    // Check that video packets start with either 00 00 01 (ISO 11172-2, MPEG-1, or ISO 13818-2, MPEG-2)
    // or 00 00 00 .. 01 (ISO 14496-10, MPEG-4 AVC). Don't know how ISO 14496-2 (MPEG-4 video) should start.
    if (IsVideoSID(pkt.getStreamId()) && !pkt.isMPEG2Video() && !pkt.isAVC()) {
        out << UString::Format(u"WARNING: PID 0x%X, invalid start of video PES payload: ", {pkt.getSourcePID()})
            << UString::Dump(pkt.payload(), std::min<size_t> (8, pkt.payloadSize()), UString::SINGLE_LINE)
            << std::endl;
    }

    // Report PES payload
    if (_dump_pes_payload) {
        size_t size = pkt.payloadSize();
        out << "  PES payload";
        if (_max_dump_size > 0 && size > _max_dump_size) {
            size = _max_dump_size;
            out << " (truncated)";
        }
        out << ":" << std::endl << UString::Dump(pkt.payload(), size, _hexa_flags | UString::ASCII, 4, _hexa_bpl);
        if (lastDump(out)) {
            return;
        }
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a PES start code is encountered.
//----------------------------------------------------------------------------

void ts::PESPlugin::handleVideoStartCode(PESDemux&, const PESPacket& pkt, uint8_t start_code, size_t offset, size_t size)
{
    if (!_dump_start_code) {
        return;
    }

    std::ostream& out(_outfile.is_open() ? _outfile : std::cout);

    out << UString::Format(u"* PID 0x%X, start code %s, offset in PES payload: %d, size: %d bytes", {pkt.getSourcePID(), names::PESStartCode(start_code, names::FIRST), offset, size})
        << std::endl;

    size_t dsize = size;
    out << "  MPEG-1/2 video unit";
    if (_max_dump_size > 0 && dsize > _max_dump_size) {
        dsize = _max_dump_size;
        out << " (truncated)";
    }
    out << ":" << std::endl << UString::Dump(pkt.payload() + offset, dsize, _hexa_flags, 4, _hexa_bpl);

    lastDump(out);
}


//----------------------------------------------------------------------------
// This hook is invoked when an AVC access unit is found
//----------------------------------------------------------------------------

void ts::PESPlugin::handleAVCAccessUnit(PESDemux&, const PESPacket& pkt, uint8_t nal_unit_type, size_t offset, size_t size)
{
    assert(nal_unit_type < 32);

    if (!_dump_nal_units || !_nal_unit_filter.test(nal_unit_type)) {
        return;
    }

    std::ostream& out(_outfile.is_open() ? _outfile : std::cout);

    // Hexadecimal dump
    out << UString::Format(u"* PID 0x%X, AVC access unit type %s", {pkt.getSourcePID(), names::AVCUnitType(nal_unit_type, names::FIRST)})
        << std::endl
        << UString::Format(u"  Offset in PES payload: %d, size: %d bytes", {offset, size})
        << std::endl;
    size_t dsize = size;
    out << "  AVC access unit";
    if (_max_dump_size > 0 && dsize > _max_dump_size) {
        dsize = _max_dump_size;
        out << " (truncated)";
    }
    out << ":" << std::endl << UString::Dump(pkt.payload() + offset, dsize, _hexa_flags, 4, _hexa_bpl);

    // Structured formatting if possible
    switch (nal_unit_type) {
        case AVC_AUT_SEQPARAMS:
        {
            AVCSequenceParameterSet params(pkt.payload() + offset, size);
            params.display(out, u"  ");
            break;
        }
        default: {
            break;
        }
    }

    lastDump(out);
}


//----------------------------------------------------------------------------
// This hook is invoked when an AVC SEI is found.
//----------------------------------------------------------------------------

void ts::PESPlugin::handleSEI(PESDemux& demux, const PESPacket& pkt, uint32_t sei_type, size_t offset, size_t size)
{
    if (!_dump_avc_sei || (!_sei_type_filter.empty() && _sei_type_filter.find(sei_type) == _sei_type_filter.end())) {
        return;
    }

    // Check if we must filter UUID on SEI's.
    if (!_sei_uuid_filter.empty()) {
        // Filter out SEI's other than user_data_unregistered (or SEI too short).
        if (sei_type != AVC_SEI_USER_DATA_UNREG || size < AVC_SEI_UUID_SIZE) {
            return;
        }
        // Check if the UUID (in the 16 first bytes of the SEI payload) must be filtered.
        bool found = false;
        for (std::list<ByteBlock>::const_iterator it = _sei_uuid_filter.begin(); !found && it != _sei_uuid_filter.end(); ++it) {
            assert(it->size() == AVC_SEI_UUID_SIZE);
            found = ::memcmp(it->data(), pkt.payload() + offset, AVC_SEI_UUID_SIZE) == 0;
        }
        if (!found) {
            // We don't want to dump this one.
            return;
        }
    }

    // Now display the SEI.
    std::ostream& out(_outfile.is_open() ? _outfile : std::cout);
    out << UString::Format(u"* PID 0x%X, SEI type %s", {pkt.getSourcePID(), NameFromSection(u"AVCSEIType", sei_type, names::FIRST)})
        << std::endl
        << UString::Format(u"  Offset in PES payload: %d, size: %d bytes", {offset, size})
        << std::endl;

    size_t dsize = size;
    out << "  AVC SEI";
    if (_max_dump_size > 0 && dsize > _max_dump_size) {
        dsize = _max_dump_size;
        out << " (truncated)";
    }
    out << ":" << std::endl << UString::Dump(pkt.payload() + offset, dsize, _hexa_flags | UString::ASCII, 4, _hexa_bpl);

    if (sei_type == AVC_SEI_PIC_TIMING)
    {
       // Maybe move that code to a new "tsAVCSEI.cpp" file
#define DISS(n, v) out << "  " << u ## #n << " = " << v << std::endl
#define DISP(n) DISS(n, static_cast<int>(n))
       ts::BitStream bs(pkt.payload() + offset, size*8);
       bs.skip(15);
       uint8_t pic_struct = bs.read<uint8_t>(4);
       DISP(pic_struct);
       bool clock_timestamp_flag = bs.readBit();
       if (clock_timestamp_flag)
       {
          uint8_t ct_type = bs.read<uint8_t>(2);
          DISP(ct_type);
          bool nuit_field_based_flag = bs.readBit();
          DISP(nuit_field_based_flag);
          uint8_t counting_type = bs.read<uint8_t>(5);
          DISP(counting_type);
          bool full_timestamp_flag = bs.readBit();
          bool discontinuity_flag = bs.readBit();
          DISP(discontinuity_flag);
          bool cnt_dropped_flag = bs.readBit();
          DISP(cnt_dropped_flag);
          uint8_t n_frames = bs.read<uint8_t>(8);
          std::stringstream timecode;
          uint8_t s = 0, m = 0, h = 0;
          if (full_timestamp_flag)
          {
             s = bs.read<uint8_t>(6);
             m = bs.read<uint8_t>(6);
             h = bs.read<uint8_t>(5);
          }
          else
          {
             if (bs.readBit())
             {
                s = bs.read<uint8_t>(6);
                if (bs.readBit())
                {
                   m = bs.read<uint8_t>(6);
                   if (bs.readBit())
                   {
                      h = bs.read<uint8_t>(5);
                   }
                }
             }
          }
          timecode << int(h) << ":" << int(m) << ":" << int(s) << "." << int(n_frames);
          DISS(timecode, timecode.str());
       }
#undef DISS
#undef DISP
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when new audio attributes are found in an audio PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewAudioAttributes(PESDemux&, const PESPacket& pkt, const AudioAttributes& aa)
{
    if (!_audio_attributes) {
        return;
    }

    std::ostream& out(_outfile.is_open() ? _outfile : std::cout);

    out << UString::Format(u"* PID 0x%04X, stream_id %s, audio attributes:", {pkt.getSourcePID(), names::StreamId(pkt.getStreamId(), names::FIRST)})
        << std::endl
        << "  " << aa << std::endl;

    lastDump(out);
}


//----------------------------------------------------------------------------
// This hook is invoked when new AC-3 attributes are found in an audio PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewAC3Attributes(PESDemux&, const PESPacket& pkt, const AC3Attributes& aa)
{
    if (!_audio_attributes) {
        return;
    }

    std::ostream& out(_outfile.is_open() ? _outfile : std::cout);

    out << UString::Format(u"* PID 0x%X, stream_id %s, AC-3 audio attributes:", {pkt.getSourcePID(), names::StreamId(pkt.getStreamId(), names::FIRST)})
        << std::endl
        << "  " << aa << std::endl;

    lastDump(out);
}


//----------------------------------------------------------------------------
// This hook is invoked when new video attributes are found in a video PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewVideoAttributes(PESDemux&, const PESPacket& pkt, const VideoAttributes& va)
{
    if (!_video_attributes) {
        return;
    }

    std::ostream& out(_outfile.is_open() ? _outfile : std::cout);

    out << UString::Format(u"* PID 0x%X, stream_id %s, video attributes:", {pkt.getSourcePID(), names::StreamId(pkt.getStreamId(), names::FIRST)})
        << std::endl
        << "  " << va << std::endl
        << UString::Format(u"  Maximum bitrate: %'d b/s, VBV buffer size: %'d bits", {va.maximumBitRate(), va.vbvSize()})
        << std::endl;

    lastDump(out);
}


//----------------------------------------------------------------------------
// This hook is invoked when new AVC attributes are found in a video PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewAVCAttributes(PESDemux&, const PESPacket& pkt, const AVCAttributes& va)
{
    if (!_video_attributes) {
        return;
    }

    std::ostream& out(_outfile.is_open() ? _outfile : std::cout);

    out << UString::Format(u"* PID 0x%X, stream_id %s, AVC video attributes:", {pkt.getSourcePID(), names::StreamId(pkt.getStreamId(), names::FIRST)})
        << std::endl
        << "  " << va << std::endl;

    lastDump(out);
}
