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
//  Analyze PES packets.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPESDemux.h"
#include "tsAVCSequenceParameterSet.h"
#include "tsNames.h"
#include "tsMemoryUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PESPlugin: public ProcessorPlugin, private PESHandlerInterface
    {
    public:
        // Implementation of plugin API
        PESPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

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
        std::bitset<32> _nal_unit_filter;
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
        bool lastDump (std::ostream&);

        // Hooks
        virtual void handlePESPacket (PESDemux&, const PESPacket&) override;
        virtual void handleVideoStartCode (PESDemux&, const PESPacket&, uint8_t, size_t, size_t) override;
        virtual void handleNewVideoAttributes (PESDemux&, const PESPacket&, const VideoAttributes&) override;
        virtual void handleAVCAccessUnit (PESDemux&, const PESPacket&, uint8_t, size_t, size_t) override;
        virtual void handleNewAVCAttributes (PESDemux&, const PESPacket&, const AVCAttributes&) override;
        virtual void handleNewAudioAttributes (PESDemux&, const PESPacket&, const AudioAttributes&) override;
        virtual void handleNewAC3Attributes (PESDemux&, const PESPacket&, const AC3Attributes&) override;

        // Inaccessible operations
        PESPlugin() = delete;
        PESPlugin(const PESPlugin&) = delete;
        PESPlugin& operator=(const PESPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::PESPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PESPlugin::PESPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Analyze PES packets.", u"[options]"),
    _abort(false),
    _outfile(),
    _trace_packets(false),
    _trace_packet_index(false),
    _dump_pes_header(false),
    _dump_pes_payload(false),
    _dump_start_code(false),
    _dump_nal_units(false),
    _nal_unit_filter(),
    _video_attributes(false),
    _audio_attributes(false),
    _max_dump_size(0),
    _max_dump_count(0),
    _hexa_flags(0),
    _hexa_bpl(0),
    _min_payload(0),
    _max_payload(0),
    _demux(this)
{
    option(u"audio-attributes",    'a');
    option(u"avc-access-unit",      0);
    option(u"binary",              'b');
    option(u"header",              'h');
    option(u"max-dump-count",      'x', UNSIGNED);
    option(u"max-dump-size",       'm', UNSIGNED);
    option(u"max-payload-size",     0,  UNSIGNED);
    option(u"min-payload-size",     0,  UNSIGNED);
    option(u"nal-unit-type",        0,  INTEGER, 0, UNLIMITED_COUNT, 0, 31);
    option(u"negate-nal-unit-type", 0);
    option(u"negate-pid",          'n');
    option(u"nibble",               0);
    option(u"output-file",         'o', STRING);
    option(u"packet-index",         0);
    option(u"payload",              0);
    option(u"pid",                 'p', PIDVAL, 0, UNLIMITED_COUNT);
    option(u"start-code",          's');
    option(u"trace-packets",       't');
    option(u"video-attributes",    'v');

    setHelp(u"Options:\n"
            u"\n"
            u"  -a\n"
            u"  --audio-attributes\n"
            u"      Display audio attributes.\n"
            u"\n"
            u"  --avc-access-unit\n"
            u"      Dump all AVC (ISO 14496-10, ITU H.264) access units (aka \"NALunits\").\n"
            u"\n"
            u"  -b\n"
            u"  --binary\n"
            u"      Include binary dump in addition to hexadecimal.\n"
            u"\n"
            u"  -h\n"
            u"  --header\n"
            u"      Dump PES packet header.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -x value\n"
            u"  --max-dump-count value\n"
            u"      Specify the maximum number of times data dump occurs with options\n"
            u"      --trace-packets, --header, --payload, --start-code, --avc-access-unit.\n"
            u"      Default: unlimited.\n"
            u"\n"
            u"  -m value\n"
            u"  --max-dump-size value\n"
            u"      Specify the maximum dump size for options --header, --payload,\n"
            u"      --start-code, --avc-access-unit.\n"
            u"\n"
            u"  --max-payload-size value\n"
            u"      Display PES packets with no payload or with a payload the size (in bytes)\n"
            u"      of which is not greater than the specified value.\n"
            u"\n"
            u"  --min-payload-size value\n"
            u"      Display PES packets with a payload the size (in bytes) of which is equal\n"
            u"      to or greater than the specified value.\n"
            u"\n"
            u"  --nal-unit-type value\n"
            u"      AVC NALunit filter: with --avc-access-unit, select access units with\n"
            u"      this type (default: all access units). Several --nal-unit-type options\n"
            u"      may be specified.\n"
            u"\n"
            u"  --negate-nal-unit-type\n"
            u"      Negate the AVC NALunit filter: specified access units are excluded.\n"
            u"\n"
            u"  -n\n"
            u"  --negate-pid\n"
            u"      Negate the PID filter: specified PID's are excluded.\n"
            u"\n"
            u"  --nibble\n"
            u"      Same as --binary but add separator between 4-bit nibbles.\n"
            u"\n"
            u"  -o filename\n"
            u"  --output-file filename\n"
            u"      Specify the output file for the report (default: standard output).\n"
            u"\n"
            u"  --packet-index\n"
            u"      Display the index of the first and last TS packet of each displayed\n"
            u"      PES packet.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      PID filter: select packets with this PID value (default: all PID's\n"
            u"      containing PES packets). Several -p or --pid options may be specified.\n"
            u"\n"
            u"  --payload\n"
            u"      Dump PES packet payload.\n"
            u"\n"
            u"  -s\n"
            u"  --start-code\n"
            u"      Dump all start codes in PES packet payload.\n"
            u"\n"
            u"  -t\n"
            u"  --trace-packets\n"
            u"      Trace all PES packets.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n"
            u"\n"
            u"  -v\n"
            u"  --video-attributes\n"
            u"      Display video attributes.\n");
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
        getPIDSet(pids, u"pid");
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

ts::ProcessorPlugin::Status ts::PESPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
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
        out << UString::Format(u"* PID 0x%X, stream_id %s, size: %d bytes (header: %d, payload: %d)",
                               {pkt.getSourcePID(), names::StreamId(pkt.getStreamId(), names::FIRST), pkt.size(), pkt.headerSize(), pkt.payloadSize()})
            << std::endl;
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
