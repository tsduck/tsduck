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

#include "tsPluginRepository.h"
#include "tsPESDemux.h"
#include "tsAVCAccessUnitDelimiter.h"
#include "tsAVCSequenceParameterSet.h"
#include "tsNames.h"
#include "tsMemory.h"
#include "tsPES.h"
#include "tsAVC.h"
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
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Commmand line options.
        bool     _trace_packets;
        bool     _trace_packet_index;
        bool     _dump_pes_header;
        bool     _dump_pes_payload;
        bool     _dump_start_code;
        bool     _dump_nal_units;
        bool     _dump_avc_sei;
        bool     _video_attributes;
        bool     _audio_attributes;
        bool     _intra_images;
        uint32_t _hexa_flags;
        size_t   _hexa_bpl;
        size_t   _max_dump_size;
        size_t   _max_dump_count;
        int      _min_payload;    // Minimum payload size (<0: no filter)
        int      _max_payload;    // Maximum payload size (<0: no filter)
        UString  _out_filename;
        UString  _pes_filename;
        PIDSet   _pids;
        std::bitset<32>      _nal_unit_filter;
        std::set<uint32_t>   _sei_type_filter;
        std::list<ByteBlock> _sei_uuid_filter;

        // Working data.
        bool          _abort;
        std::ofstream _out_file;
        std::ofstream _pes_file;
        std::ostream* _pes_stream;
        PESDemux      _demux;

        // A string containing the PID and optional TS packet indexes.
        UString prefix(const PESPacket&) const;

        // Process dump count. Return true when terminated. Also process error on output.
        bool lastDump(std::ostream&);

        // Implementation of PESHandlerInterface.
        virtual void handlePESPacket(PESDemux&, const PESPacket&) override;
        virtual void handleIntraImage(PESDemux&, const PESPacket&, size_t) override;
        virtual void handleVideoStartCode(PESDemux&, const PESPacket&, uint8_t, size_t, size_t) override;
        virtual void handleNewMPEG2VideoAttributes(PESDemux&, const PESPacket&, const MPEG2VideoAttributes&) override;
        virtual void handleAVCAccessUnit(PESDemux&, const PESPacket&, uint8_t, size_t, size_t) override;
        virtual void handleSEI(PESDemux& demux, const PESPacket& packet, uint32_t sei_type, size_t offset, size_t size) override;
        virtual void handleNewAVCAttributes(PESDemux&, const PESPacket&, const AVCAttributes&) override;
        virtual void handleNewMPEG2AudioAttributes(PESDemux&, const PESPacket&, const MPEG2AudioAttributes&) override;
        virtual void handleNewAC3Attributes(PESDemux&, const PESPacket&, const AC3Attributes&) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pes", ts::PESPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PESPlugin::PESPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Analyze PES packets", u"[options]"),
    _trace_packets(false),
    _trace_packet_index(false),
    _dump_pes_header(false),
    _dump_pes_payload(false),
    _dump_start_code(false),
    _dump_nal_units(false),
    _dump_avc_sei(false),
    _video_attributes(false),
    _audio_attributes(false),
    _intra_images(false),
    _hexa_flags(0),
    _hexa_bpl(0),
    _max_dump_size(0),
    _max_dump_count(0),
    _min_payload(0),
    _max_payload(0),
    _out_filename(),
    _pes_filename(),
    _pids(),
    _nal_unit_filter(),
    _sei_type_filter(),
    _sei_uuid_filter(),
    _abort(false),
    _out_file(),
    _pes_file(),
    _pes_stream(nullptr),
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

    option(u"intra-image", 'i');
    help(u"intra-image", u"Report intra images.");

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

    option(u"save-pes", 0, STRING);
    help(u"save-pes", u"filename",
         u"Save all PES packets in the specified file. "
         u"All PES packets are saved in a raw binary form without encapsulation. "
         u"When the specified file is '-', the standard output is used.");

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
         u"It must be either an ASCII string of exactly 16 characters or a hexadecimal "
         u"value representing 16 bytes.");

    option(u"video-attributes", 'v');
    help(u"video-attributes", u"Display video attributes.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::PESPlugin::getOptions()
{
    _dump_pes_header = present(u"header");
    _dump_pes_payload = present(u"payload");
    _trace_packets = present(u"trace-packets") || _dump_pes_header || _dump_pes_payload;
    _trace_packet_index = present(u"packet-index");
    _dump_start_code = present(u"start-code");
    _dump_nal_units = present(u"avc-access-unit");
    _dump_avc_sei = present(u"sei-avc");
    _video_attributes = present(u"video-attributes");
    _audio_attributes = present(u"audio-attributes");
    _intra_images = present(u"intra-image");
    _max_dump_size = intValue<size_t>(u"max-dump-size", 0);
    _max_dump_count = intValue<size_t>(u"max-dump-count", 0);
    _min_payload = intValue<int>(u"min-payload-size", -1);
    _max_payload = intValue<int>(u"max-payload-size", -1);
    getValue(_out_filename, u"output-file");
    getValue(_pes_filename, u"save-pes");

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
        getIntValues(_pids, u"pid");
        if (present(u"negate-pid")) {
            _pids.flip();
        }
    }
    else {
        _pids.set();
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

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PESPlugin::start()
{
    // Create output files.
    if (!_out_filename.empty()) {
        tsp->verbose(u"creating %s", {_out_filename});
        _out_file.open(_out_filename.toUTF8().c_str(), std::ios::out);
        if (!_out_file) {
            error(u"cannot create %s", {_out_filename});
            return false;
        }
    }
    if (_pes_filename == u"-") {
        // Save PES packets on standard output, in binary mode.
        _pes_stream = &std::cout;
        SetBinaryModeStdout(*tsp);
    }
    else if (_pes_filename.empty()) {
        // Don't save PES packets.
        _pes_stream = nullptr;
    }
    else {
        // Save PES packets in a regular binary file.
        tsp->verbose(u"creating %s", {_pes_filename});
        _pes_file.open(_pes_filename.toUTF8().c_str(), std::ios::out | std::ios::binary);
        if (!_pes_file) {
            error(u"cannot create %s", {_pes_filename});
            return false;
        }
        _pes_stream = &_pes_file;
    }

    // Reset state.
    _demux.reset();
    _demux.setPIDFilter(_pids);
    _abort = false;
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PESPlugin::stop()
{
    // Close output files.
    if (_out_file.is_open()) {
        _out_file.close();
    }
    if (_pes_file.is_open()) {
        _pes_file.close();
    }
    _pes_stream = nullptr;
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
// A string containing the PID and optional TS packet indexes.
//----------------------------------------------------------------------------

ts::UString ts::PESPlugin::prefix(const PESPacket& pkt) const
{
    UString line;
    line.format(u"PID 0x%X", {pkt.getSourcePID()});
    if (_trace_packet_index) {
        line.format(u", TS packets %'d-%'d", {pkt.getFirstTSPacketIndex(), pkt.getLastTSPacketIndex()});
    }
    return line;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete PES packet is available.
//----------------------------------------------------------------------------

void ts::PESPlugin::handlePESPacket(PESDemux&, const PESPacket& pkt)
{
    std::ostream& out(_out_file.is_open() ? _out_file : std::cout);

    // Skip PES packets without appropriate payload size
    if (int(pkt.payloadSize()) < _min_payload || (_max_payload >= 0 && int(pkt.payloadSize()) > _max_payload)) {
        return;
    }

    // Report packet description
    if (_trace_packets) {
        out << "* " << prefix(pkt)
            << ", stream_id " << names::StreamId(pkt.getStreamId(), names::FIRST)
            << UString::Format(u", size: %d bytes (header: %d, payload: %d)", {pkt.size(), pkt.headerSize(), pkt.payloadSize()});
        const size_t spurious = pkt.spuriousDataSize();
        if (spurious > 0) {
            out << UString::Format(u", %d spurious trailing bytes", {spurious});
        }
        out << std::endl;
        if (lastDump(out)) {
            return;
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
        if (IsVideoSID(pkt.getStreamId()) && !pkt.isMPEG2Video() && !pkt.isAVC() && !pkt.isHEVC()) {
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

    // Save binary PES packet.
    if (_pes_stream != nullptr) {
        _pes_stream->write(reinterpret_cast<const char*>(pkt.content()), pkt.size());
        if (!(*_pes_stream)) {
            tsp->error(u"error writing PES packet to %s", {_pes_filename == u"-" ? u"standard output" : _pes_filename});
            _abort = true;
        }
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when an intra-code image is found.
//----------------------------------------------------------------------------

void ts::PESPlugin::handleIntraImage(PESDemux& demux, const PESPacket& pkt, size_t offset)
{
    if (_intra_images) {
        std::ostream& out(_out_file.is_open() ? _out_file : std::cout);
        out << "* " << prefix(pkt) << UString::Format(u", intra-image offset in PES payload: %d/%d", {offset, pkt.payloadSize()}) << std::endl;
        lastDump(out);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a PES start code is encountered.
//----------------------------------------------------------------------------

void ts::PESPlugin::handleVideoStartCode(PESDemux&, const PESPacket& pkt, uint8_t start_code, size_t offset, size_t size)
{
    std::ostream& out(_out_file.is_open() ? _out_file : std::cout);

    // Dump video start code.
    if (_dump_start_code) {
        out << "* " << prefix(pkt)
            << ", start code " << names::PESStartCode(start_code, names::FIRST)
            << UString::Format(u", offset in PES payload: %d, size: %d bytes", {offset, size})
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
}


//----------------------------------------------------------------------------
// This hook is invoked when an AVC access unit is found
//----------------------------------------------------------------------------

void ts::PESPlugin::handleAVCAccessUnit(PESDemux&, const PESPacket& pkt, uint8_t nal_unit_type, size_t offset, size_t size)
{
    assert(nal_unit_type < 32);

    std::ostream& out(_out_file.is_open() ? _out_file : std::cout);

    // Dump the NALunit.
    if (_dump_nal_units && _nal_unit_filter.test(nal_unit_type)) {

        // Hexadecimal dump
        out << "* " << prefix(pkt) << ", AVC access unit type " << names::AVCUnitType(nal_unit_type, names::FIRST) << std::endl;
        out << UString::Format(u"  Offset in PES payload: %d, size: %d bytes", {offset, size}) << std::endl;

        size_t dsize = size;
        out << "  AVC access unit";
        if (_max_dump_size > 0 && dsize > _max_dump_size) {
            dsize = _max_dump_size;
            out << " (truncated)";
        }
        out << ":" << std::endl << UString::Dump(pkt.payload() + offset, dsize, _hexa_flags, 4, _hexa_bpl);

        // Structured formatting if possible
        switch (nal_unit_type) {
            case AVC_AUT_SEQPARAMS: {
                const AVCSequenceParameterSet params(pkt.payload() + offset, size);
                params.display(out, u"  ");
                break;
            }
            case AVC_AUT_DELIMITER: {
                const AVCAccessUnitDelimiter aud(pkt.payload() + offset, size);
                aud.display(out, u"  ");
                break;
            }
            default: {
                break;
            }
        }
        lastDump(out);
    }
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
    std::ostream& out(_out_file.is_open() ? _out_file : std::cout);
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
}


//----------------------------------------------------------------------------
// This hook is invoked when new audio attributes are found in an audio PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewMPEG2AudioAttributes(PESDemux&, const PESPacket& pkt, const MPEG2AudioAttributes& aa)
{
    if (!_audio_attributes) {
        return;
    }

    std::ostream& out(_out_file.is_open() ? _out_file : std::cout);

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

    std::ostream& out(_out_file.is_open() ? _out_file : std::cout);

    out << UString::Format(u"* PID 0x%X, stream_id %s, AC-3 audio attributes:", {pkt.getSourcePID(), names::StreamId(pkt.getStreamId(), names::FIRST)})
        << std::endl
        << "  " << aa << std::endl;

    lastDump(out);
}


//----------------------------------------------------------------------------
// This hook is invoked when new video attributes are found in a video PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewMPEG2VideoAttributes(PESDemux&, const PESPacket& pkt, const MPEG2VideoAttributes& va)
{
    if (!_video_attributes) {
        return;
    }

    std::ostream& out(_out_file.is_open() ? _out_file : std::cout);

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

    std::ostream& out(_out_file.is_open() ? _out_file : std::cout);

    out << UString::Format(u"* PID 0x%X, stream_id %s, AVC video attributes:", {pkt.getSourcePID(), names::StreamId(pkt.getStreamId(), names::FIRST)})
        << std::endl
        << "  " << va << std::endl;

    lastDump(out);
}
