//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Analyze PES packets.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsFileNameGenerator.h"
#include "tsPESDemux.h"
#include "tsAVCAccessUnitDelimiter.h"
#include "tsAVCSequenceParameterSet.h"
#include "tsHEVCAccessUnitDelimiter.h"
#include "tsHEVCSequenceParameterSet.h"
#include "tsVVCAccessUnitDelimiter.h"
#include "tsPES.h"
#include "tsAVC.h"
#include "tsHEVC.h"
#include "tsVVC.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PESPlugin: public ProcessorPlugin, private PESHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(PESPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Commmand line options.
        bool      _trace_packets = false;
        bool      _trace_packet_index = false;
        bool      _dump_pes_header = false;
        bool      _dump_pes_payload = false;
        bool      _dump_start_code = false;
        bool      _dump_nal_units = false;
        bool      _dump_avc_sei = false;
        bool      _video_attributes = false;
        bool      _audio_attributes = false;
        bool      _intra_images = false;
        bool      _negate_nal_unit_filter = false;
        bool      _multiple_files = false;
        bool      _flush_last = false;
        uint32_t  _hexa_flags = 0;
        size_t    _hexa_bpl = 0;
        size_t    _max_dump_size = 0;
        size_t    _max_dump_count = 0;
        int       _min_payload = 0;    // Minimum payload size (<0: no filter)
        int       _max_payload = 0;    // Maximum payload size (<0: no filter)
        fs::path  _out_filename {};
        fs::path  _pes_filename {};
        fs::path  _es_filename {};
        PIDSet    _pids {};
        CodecType _default_h26x = CodecType::UNDEFINED;
        std::set<uint8_t>    _nal_unit_filter {};
        std::set<uint32_t>   _sei_type_filter {};
        std::list<ByteBlock> _sei_uuid_filter {};

        // Working data.
        bool              _abort = false;
        std::ofstream     _out_file {};
        std::ostream*     _out = nullptr;
        std::ofstream     _pes_file {};
        std::ostream*     _pes_stream = nullptr;
        std::ofstream     _es_file {};
        std::ostream*     _es_stream = nullptr;
        PESDemux          _demux;
        FileNameGenerator _pes_name_gen {};
        FileNameGenerator _es_name_gen {};

        // Open output file.
        bool openOutput(const fs::path&, std::ofstream*, std::ostream**, bool binary);

        // A string containing the PID and optional TS packet indexes.
        UString prefix(const DemuxedData&) const;

        // Do we need to display this acces unit type?
        bool useAccesUnitType(uint8_t) const;

        // Process dump count. Return true when terminated. Also process error on output.
        bool lastDump(std::ostream&);

        // Save one file using --multiple-file. Set _abort on error.
        void saveOnePES(FileNameGenerator& namegen, const uint8_t* data, size_t size);

        // Implementation of PESHandlerInterface.
        virtual void handlePESPacket(PESDemux&, const PESPacket&) override;
        virtual void handleInvalidPESPacket(PESDemux&, const DemuxedData&) override;
        virtual void handleIntraImage(PESDemux&, const PESPacket&, size_t) override;
        virtual void handleVideoStartCode(PESDemux&, const PESPacket&, uint8_t, size_t, size_t) override;
        virtual void handleNewMPEG2VideoAttributes(PESDemux&, const PESPacket&, const MPEG2VideoAttributes&) override;
        virtual void handleAccessUnit(PESDemux&, const PESPacket&, uint8_t, size_t, size_t) override;
        virtual void handleSEI(PESDemux& demux, const PESPacket& packet, uint32_t sei_type, size_t offset, size_t size) override;
        virtual void handleNewAVCAttributes(PESDemux&, const PESPacket&, const AVCAttributes&) override;
        virtual void handleNewHEVCAttributes(PESDemux&, const PESPacket&, const HEVCAttributes&) override;
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
    _demux(duck, this)
{
    option(u"audio-attributes", 'a');
    help(u"audio-attributes", u"Display audio attributes.");

    option(u"avc-access-unit");
    help(u"avc-access-unit",
         u"Dump all AVC (H.264), HEVC (H.265) or VVC (H.266) access units (aka \"NALunits\").");

    option(u"binary", 'b');
    help(u"binary", u"Include binary dump in addition to hexadecimal.");

    option(u"flush-last-unbounded-pes", 'f');
    help(u"flush-last-unbounded-pes",
         u"At end of stream, consider the last unbounded PES packet as complete in each PID. "
         u"By default, there is no guarantee that the unbounded buffered data at end of stream "
         u"form a valid PES packet and they are discarded.");

    option(u"h26x-default-format", 0, Names({
        {u"AVC",   CodecType::AVC},
        {u"H.264", CodecType::AVC},
        {u"HEVC",  CodecType::HEVC},
        {u"H.265", CodecType::HEVC},
        {u"VVC",   CodecType::VVC},
        {u"H.266", CodecType::VVC},
    }));
    help(u"h26x-default-format", u"name",
         u"The video formats AVC (H.264), HEVC (H.265) and VVC (H.266) use the same binary bitstream format. "
         u"But the formats of their NALunits are different. "
         u"When analyzing PES packets of one of these formats, the plugin must know which is the actual one. "
         u"This is usually automatically done from the stream type in the PMT of the service. "
         u"However, if the PID is unreferenced or if the PMT was previously filtered out, "
         u"this option indicates which format to use. "
         u"The default is AVC (H.264).");

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

    option(u"multiple-files");
    help(u"multiple-files",
         u"With options --save-pes and --save-es, save each PES packet in a distinct file. "
         u"The specified file name in --save-pes or --save-es is considered as a template and a unique "
         u"number is automatically added to the name part so that successive files receive distinct names. "
         u"Example: if the specified file name is base.pes, the various files are named base-000000.pes, base-000001.pes, etc. "
         u"If the specified template already contains trailing digits, this unmodified name is used for the first file. "
         u"Then, the integer part is incremented. "
         u"Example: if the specified file name is base-027.pes, the various files are named base-027.pes, base-028.pes, etc.");

    option(u"nal-unit-type", 0, UINT8, 0, UNLIMITED_COUNT);
    help(u"nal-unit-type",
         u"AVC (H.264), HEVC (H.265) or VVC (H.266) NALunit filter: "
         u"with --avc-access-unit, select access units with this type "
         u"(default: all access units). "
         u"Several --nal-unit-type options may be specified.");

    option(u"negate-nal-unit-type");
    help(u"negate-nal-unit-type",
         u"Negate the AVC/HEVC/VVC NALunit filter: specified access units are excluded.");

    option(u"negate-pid", 'n');
    help(u"negate-pid",
         u"Negate the PID filter: specified PID's are excluded.");

    option(u"nibble");
    help(u"nibble",
         u"Same as --binary but add separator between 4-bit nibbles.");

    option(u"output-file", 'o', FILENAME);
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

    option(u"save-es", 0, FILENAME);
    help(u"save-es", u"filename",
         u"Save the elementary stream in the specified file. "
         u"The payloads of all PES packets are saved in a raw binary form without encapsulation. "
         u"The PES headers are dropped. "
         u"When the specified file is '-', the standard output is used.");

    option(u"save-pes", 0, FILENAME);
    help(u"save-pes", u"filename",
        u"Save all PES packets, header and payload, in the specified file. "
        u"All PES packets are saved in a raw binary form without encapsulation. "
        u"When the specified file is '-', the standard output is used.");

    option(u"sei-avc");
    help(u"sei-avc",
         u"Dump all SEI (Supplemental Enhancement Information) "
         u"in AVC (H.264), HEVC (H.265) or VVC (H.266) access units.");

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
         u"SEI filter: with --sei-avc, select \"user data unregistered\" SEI "
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
    _multiple_files = present(u"multiple-files");
    _flush_last = present(u"flush-last-unbounded-pes");
    getIntValue(_max_dump_size, u"max-dump-size", 0);
    getIntValue(_max_dump_count, u"max-dump-count", 0);
    getIntValue(_min_payload, u"min-payload-size", -1);
    getIntValue(_max_payload, u"max-payload-size", -1);
    getIntValue(_default_h26x, u"h26x-default-format", CodecType::AVC);
    getPathValue(_out_filename, u"output-file");
    getPathValue(_pes_filename, u"save-pes");
    getPathValue(_es_filename, u"save-es");
    _negate_nal_unit_filter = present(u"negate-nal-unit-type");
    getIntValues(_nal_unit_filter, u"nal-unit-type");
    getIntValues(_sei_type_filter, u"sei-type");

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
            error(u"invalid UUID \"%s\"", uuid);
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
    // Reset PES demux.
    _demux.reset();
    _demux.setPIDFilter(_pids);
    _demux.setDefaultCodec(_default_h26x);

    // Create output files.
    bool ok = openOutput(_out_filename, &_out_file, &_out, false);
    if (_multiple_files) {
        _pes_name_gen.initCounter(_pes_filename);
        _es_name_gen.initCounter(_es_filename);
    }
    else {
        ok = ok && openOutput(_pes_filename, &_pes_file, &_pes_stream, true) && openOutput(_es_filename, &_es_file, &_es_stream, true);
    }

    if (!ok) {
        // Close files which were open before failure
        stop();
    }

    _abort = false;
    return ok;
}


//----------------------------------------------------------------------------
// Open output binary file (--save-pes or --save-es).
//----------------------------------------------------------------------------

bool ts::PESPlugin::openOutput(const fs::path& filename, std::ofstream* file, std::ostream** stream, bool binary)
{
    if (filename == u"-") {
        // Save binary data on standard output, in binary mode.
        *stream = &std::cout;
        if (binary) {
            SetBinaryModeStdout(*this);
        }
    }
    else if (filename.empty()) {
        // Don't save binary data, log text data.
        *stream = binary ? nullptr : &std::cout;
    }
    else {
        // Save binary data in a regular binary file.
        verbose(u"creating %s", filename);
        file->open(filename, binary ? (std::ios::out | std::ios::binary) : std::ios::out);
        if (!(*file)) {
            error(u"cannot create %s", filename);
            return false;
        }
        *stream = file;
    }
    return true;
}


//----------------------------------------------------------------------------
// Save one file using --multiple-file. Set _abort on error.
//----------------------------------------------------------------------------

void ts::PESPlugin::saveOnePES(FileNameGenerator& namegen, const uint8_t* data, size_t size)
{
    const fs::path filename(namegen.newFileName());
    debug(u"creating %s", filename);
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file) {
        error(u"cannot create %s", filename);
        _abort = false;
    }
    else {
        file.write(reinterpret_cast<const char*>(data), size);
        file.close();
    }
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PESPlugin::stop()
{
    if (_flush_last && !_abort) {
        _demux.flushUnboundedPES();
    }
    if (_out_file.is_open()) {
        _out_file.close();
    }
    if (_pes_file.is_open()) {
        _pes_file.close();
    }
    if (_es_file.is_open()) {
        _es_file.close();
    }
    _out = &std::cout;
    _pes_stream = nullptr;
    _es_stream = nullptr;
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

ts::UString ts::PESPlugin::prefix(const DemuxedData& pkt) const
{
    UString line;
    line.format(u"PID 0x%X", pkt.sourcePID());
    if (_trace_packet_index) {
        line.format(u", TS packets %'d-%'d", pkt.firstTSPacketIndex(), pkt.lastTSPacketIndex());
    }
    return line;
}


//----------------------------------------------------------------------------
// Invoked by the demux when an invalid PES packet is encountered.
//----------------------------------------------------------------------------

void ts::PESPlugin::handleInvalidPESPacket(PESDemux&, const DemuxedData& data)
{
    // Report invalid packets with --trace-packets
    if (_trace_packets) {
        *_out << UString::Format(u"* %s, invalid PES packet, data size: %d bytes", prefix(data), data.size());
        const size_t hsize = PESPacket::HeaderSize(data.content(), data.size());
        if (hsize == 0) {
            *_out << ", no PES header found";
        }
        else if (data.size() < hsize) {
            *_out << UString::Format(u", expected header size: %d bytes", hsize);
        }
        else {
            // The embedded PES payload size is either zero (unbounded) or indicates the packet length _after_ that field (ie. after offset 6).
            const size_t psize = 6 + size_t(GetUInt16(data.content() + 4));
            if (psize != 6) {
                *_out << UString::Format(u", PES packet size: %d bytes", psize);
                if (psize < hsize) {
                    *_out << UString::Format(u", expected header size: %d bytes", hsize);
                }
                if (data.size() < psize) {
                    *_out << UString::Format(u", truncated, missing %d bytes", psize - data.size());
                }
            }
        }
        *_out << std::endl;
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete PES packet is available.
//----------------------------------------------------------------------------

void ts::PESPlugin::handlePESPacket(PESDemux&, const PESPacket& pkt)
{
    // Skip PES packets without appropriate payload size
    if (int(pkt.payloadSize()) < _min_payload || (_max_payload >= 0 && int(pkt.payloadSize()) > _max_payload)) {
        return;
    }

    // Report packet description
    if (_trace_packets) {
        *_out << "* " << prefix(pkt)
              << ", stream_id " << NameFromSection(u"dtv", u"pes.stream_id", pkt.getStreamId(), NamesFlags::VALUE_NAME)
              << UString::Format(u", size: %d bytes (header: %d, payload: %d)", pkt.size(), pkt.headerSize(), pkt.payloadSize());
        const size_t spurious = pkt.spuriousDataSize();
        if (spurious > 0) {
            *_out << UString::Format(u", raw data: %d bytes, %d spurious trailing bytes", pkt.rawDataSize(), spurious);
        }
        *_out << std::endl;
        if (lastDump(*_out)) {
            return;
        }

        // Report PES header
        if (_dump_pes_header) {
            size_t size = pkt.headerSize();
            *_out << "  PES header";
            if (_max_dump_size > 0 && size > _max_dump_size) {
                size = _max_dump_size;
                *_out << " (truncated)";
            }
            *_out << ":" << std::endl << UString::Dump(pkt.header(), size, _hexa_flags, 4, _hexa_bpl);
            if (lastDump (*_out)) {
                return;
            }
        }

        // Check that video packets start with either 00 00 01 (ISO 11172-2, MPEG-1, or ISO 13818-2, MPEG-2)
        // or 00 00 00 .. 01 (ISO 14496-10, MPEG-4 AVC). Don't know how ISO 14496-2 (MPEG-4 video) should start.
        if (IsVideoSID(pkt.getStreamId()) &&
            !pkt.isMPEG2Video() &&
            !pkt.isAVC() &&
            !pkt.isHEVC() &&
            !pkt.isVVC() &&
            !PESPacket::HasCommonVideoHeader(pkt.payload(), pkt.payloadSize()))
        {
            *_out << UString::Format(u"WARNING: PID 0x%X, invalid start of video PES payload: ", pkt.sourcePID())
                  << UString::Dump(pkt.payload(), std::min<size_t>(8, pkt.payloadSize()), UString::SINGLE_LINE)
                  << std::endl;
        }

        // Report PES payload
        if (_dump_pes_payload) {
            size_t size = pkt.payloadSize();
            *_out << "  PES payload";
            if (_max_dump_size > 0 && size > _max_dump_size) {
                size = _max_dump_size;
                *_out << " (truncated)";
            }
            *_out << ":" << std::endl << UString::Dump(pkt.payload(), size, _hexa_flags | UString::ASCII, 4, _hexa_bpl);
            if (lastDump(*_out)) {
                return;
            }
        }
    }

    // Save binary PES packet and payload.
    if (_multiple_files) {
        if (!_pes_filename.empty()) {
            saveOnePES(_pes_name_gen, pkt.content(), pkt.size());
        }
        if (!_es_filename.empty()) {
            saveOnePES(_es_name_gen, pkt.payload(), pkt.payloadSize());
        }
    }
    else {
        if (_pes_stream != nullptr) {
            _pes_stream->write(reinterpret_cast<const char*>(pkt.content()), pkt.size());
            if (!(*_pes_stream)) {
                error(u"error writing PES packet to %s", _pes_filename == u"-" ? u"standard output" : _pes_filename);
                _abort = true;
            }
        }
        if (_es_stream != nullptr) {
            _es_stream->write(reinterpret_cast<const char*>(pkt.payload()), pkt.payloadSize());
            if (!(*_es_stream)) {
                error(u"error writing ES data to %s", _es_filename == u"-" ? u"standard output" : _es_filename);
                _abort = true;
            }
        }
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when an intra-code image is found.
//----------------------------------------------------------------------------

void ts::PESPlugin::handleIntraImage(PESDemux& demux, const PESPacket& pkt, size_t offset)
{
    if (_intra_images) {
        *_out << "* " << prefix(pkt) << UString::Format(u", intra-image offset in PES payload: %d/%d",  offset, pkt.payloadSize() ) << std::endl;
        lastDump(*_out);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a PES start code is encountered.
//----------------------------------------------------------------------------

void ts::PESPlugin::handleVideoStartCode(PESDemux&, const PESPacket& pkt, uint8_t start_code, size_t offset, size_t size)
{
    // Dump video start code.
    if (_dump_start_code) {
        *_out << "* " << prefix(pkt)
              << ", start code " << NameFromSection(u"dtv", u"pes.stream_id", start_code, NamesFlags::VALUE_NAME)
              << UString::Format(u", offset in PES payload: %d, size: %d bytes", offset, size)
              << std::endl;

        size_t dsize = size;
        *_out << "  MPEG-1/2 video unit";
        if (_max_dump_size > 0 && dsize > _max_dump_size) {
            dsize = _max_dump_size;
            *_out << " (truncated)";
        }
        *_out << ":" << std::endl << UString::Dump(pkt.payload() + offset, dsize, _hexa_flags, 4, _hexa_bpl);
        lastDump(*_out);
    }
}


//----------------------------------------------------------------------------
// Do we need to display this acces unit type?
//----------------------------------------------------------------------------

bool ts::PESPlugin::useAccesUnitType(uint8_t type) const
{
    if (_nal_unit_filter.empty()) {
        // No filter, use them all.
        return true;
    }
    else {
        const bool found = _nal_unit_filter.contains(type);
        return (!_negate_nal_unit_filter && found) || (_negate_nal_unit_filter && !found);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when an AVC/HEVC/VVC access unit is found
//----------------------------------------------------------------------------

void ts::PESPlugin::handleAccessUnit(PESDemux&, const PESPacket& pes, uint8_t au_type, size_t offset, size_t size)
{
    // Dump the NALunit.
    if (_dump_nal_units && useAccesUnitType(au_type)) {

        const CodecType codec = pes.getCodec();

        // Hexadecimal dump
        *_out << "* " << prefix(pes) << ", " << CodecTypeEnum().name(codec) << " access unit type " << AccessUnitTypeName(codec, au_type, NamesFlags::VALUE_NAME) << std::endl;
        *_out << UString::Format(u"  Offset in PES payload: %d, size: %d bytes", offset, size) << std::endl;

        size_t dsize = size;
        *_out << "  " << CodecTypeEnum().name(codec) << " access unit";
        if (_max_dump_size > 0 && dsize > _max_dump_size) {
            dsize = _max_dump_size;
            *_out << " (truncated)";
        }
        *_out << ":" << std::endl << UString::Dump(pes.payload() + offset, dsize, _hexa_flags, 4, _hexa_bpl);

        // Structured formatting if possible
        if (codec == CodecType::AVC && au_type == AVC_AUT_SEQPARAMS) {
            const AVCSequenceParameterSet params(pes.payload() + offset, size);
            params.display(*_out, u"  ");
        }
        else if (codec == CodecType::AVC && au_type == AVC_AUT_DELIMITER) {
            const AVCAccessUnitDelimiter aud(pes.payload() + offset, size);
            aud.display(*_out, u"  ");
        }
        else if (codec == CodecType::HEVC && au_type == HEVC_AUT_AUD_NUT) {
            const HEVCAccessUnitDelimiter aud(pes.payload() + offset, size);
            aud.display(*_out, u"  ");
        }
        else if (codec == CodecType::HEVC && au_type == HEVC_AUT_SPS_NUT) {
            const HEVCSequenceParameterSet sps(pes.payload() + offset, size);
            sps.display(*_out, u"  ");
        }
        else if (codec == CodecType::VVC && au_type == VVC_AUT_AUD_NUT) {
            const VVCAccessUnitDelimiter aud(pes.payload() + offset, size);
            aud.display(*_out, u"  ");
        }
        lastDump(*_out);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when an AVC SEI is found.
//----------------------------------------------------------------------------

void ts::PESPlugin::handleSEI(PESDemux& demux, const PESPacket& pkt, uint32_t sei_type, size_t offset, size_t size)
{
    if (!_dump_avc_sei || (!_sei_type_filter.empty() && !_sei_type_filter.contains(sei_type))) {
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
        for (auto it = _sei_uuid_filter.begin(); !found && it != _sei_uuid_filter.end(); ++it) {
            assert(it->size() == AVC_SEI_UUID_SIZE);
            found = MemEqual(it->data(), pkt.payload() + offset, AVC_SEI_UUID_SIZE);
        }
        if (!found) {
            // We don't want to dump this one.
            return;
        }
    }

    // Now display the SEI.
    *_out << "* " << prefix(pkt) << ", SEI type " << NameFromSection(u"dtv", u"avc.sei_type", sei_type, NamesFlags::VALUE_NAME) << std::endl;
    *_out << UString::Format(u"  Offset in PES payload: %d, size: %d bytes", offset, size) << std::endl;

    size_t dsize = size;
    *_out << "  AVC SEI";
    if (_max_dump_size > 0 && dsize > _max_dump_size) {
        dsize = _max_dump_size;
        *_out << " (truncated)";
    }
    *_out << ":" << std::endl << UString::Dump(pkt.payload() + offset, dsize, _hexa_flags | UString::ASCII, 4, _hexa_bpl);
}


//----------------------------------------------------------------------------
// This hook is invoked when new audio attributes are found in an audio PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewMPEG2AudioAttributes(PESDemux&, const PESPacket& pkt, const MPEG2AudioAttributes& aa)
{
    if (_audio_attributes) {
        *_out << "* " << prefix(pkt) << ", stream_id " << NameFromSection(u"dtv", u"pes.stream_id", pkt.getStreamId(), NamesFlags::VALUE_NAME) << ", audio attributes:" << std::endl;
        *_out << "  " << aa << std::endl;
        lastDump(*_out);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when new AC-3 attributes are found in an audio PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewAC3Attributes(PESDemux&, const PESPacket& pkt, const AC3Attributes& aa)
{
    if (_audio_attributes) {
        *_out << "* " << prefix(pkt) << ", stream_id " << NameFromSection(u"dtv", u"pes.stream_id", pkt.getStreamId(), NamesFlags::VALUE_NAME) << ", AC-3 audio attributes:" << std::endl;
        *_out << "  " << aa << std::endl;
        lastDump(*_out);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when new video attributes are found in a video PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewMPEG2VideoAttributes(PESDemux&, const PESPacket& pkt, const MPEG2VideoAttributes& va)
{
    if (_video_attributes) {
        *_out << "* " << prefix(pkt) << ", stream_id " << NameFromSection(u"dtv", u"pes.stream_id", pkt.getStreamId(), NamesFlags::VALUE_NAME) << ", video attributes:" << std::endl;
        *_out << "  " << va << std::endl;
        *_out << UString::Format(u"  Maximum bitrate: %'d b/s, VBV buffer size: %'d bits", va.maximumBitRate(), va.vbvSize()) << std::endl;
        lastDump(*_out);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when new AVC attributes are found in a video PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewAVCAttributes(PESDemux&, const PESPacket& pkt, const AVCAttributes& va)
{
    if (_video_attributes) {
        *_out << "* " << prefix(pkt) << ", stream_id " << NameFromSection(u"dtv", u"pes.stream_id", pkt.getStreamId(), NamesFlags::VALUE_NAME) << ", AVC video attributes:" << std::endl;
        *_out << "  " << va << std::endl;
        lastDump(*_out);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when new HEVC attributes are found in a video PID
//----------------------------------------------------------------------------

void ts::PESPlugin::handleNewHEVCAttributes(PESDemux&, const PESPacket& pkt, const HEVCAttributes& va)
{
    if (_video_attributes) {
        *_out << "* " << prefix(pkt) << ", stream_id " << NameFromSection(u"dtv", u"pes.stream_id", pkt.getStreamId(), NamesFlags::VALUE_NAME) << ", HEVC video attributes:" << std::endl;
        *_out << "  " << va << std::endl;
        lastDump(*_out);
    }
}
