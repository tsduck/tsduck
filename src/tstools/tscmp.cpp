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
//
// A command to compare two TS files.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsjsonOutputArgs.h"
#include "tsTSFile.h"
#include "tsFileUtils.h"
#include "tsjsonObject.h"
TS_MAIN(MainCode);

#define DEFAULT_BUFFERED_PACKETS 10000
#define DEFAULT_MIN_REORDER          7


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace ts {
    class TSCompareOptions: public Args
    {
        TS_NOBUILD_NOCOPY(TSCompareOptions);
    public:
        TSCompareOptions(int argc, char *argv[]);
        virtual ~TSCompareOptions() override;

        DuckContext      duck;
        TSPacketFormat   format;
        UString          filename0;
        UString          filename1;
        uint64_t         byte_offset;
        size_t           buffered_packets;
        size_t           threshold_diff;
        size_t           min_reorder;
        bool             search_reorder;
        bool             dump;
        uint32_t         dump_flags;
        bool             normalized;
        bool             quiet;
        bool             payload_only;
        bool             pcr_ignore;
        bool             pid_ignore;
        bool             cc_ignore;
        bool             continue_all;
        json::OutputArgs json;
    };
}


// Command line options constructor.
ts::TSCompareOptions::TSCompareOptions(int argc, char *argv[]) :
    Args(u"Compare two transport stream files", u"[options] filename-1 filename-2"),
    duck(this),
    format(TSPacketFormat::AUTODETECT),
    filename0(),
    filename1(),
    byte_offset(0),
    buffered_packets(0),
    threshold_diff(0),
    min_reorder(0),
    search_reorder(false),
    dump(false),
    dump_flags(0),
    normalized(false),
    quiet(false),
    payload_only(false),
    pcr_ignore(false),
    pid_ignore(false),
    cc_ignore(false),
    continue_all(false),
    json()
{
    ts::DefineTSPacketFormatInputOption(*this, 'f');

    option(u"", 0, FILENAME, 2, 2);
    help(u"", u"MPEG capture files to be compared.");

    option(u"buffered-packets", 0, UNSIGNED);
    help(u"buffered-packets", u"count",
         u"Specifies the files input buffer size in TS packets. "
         u"This is used with --search-reorder to look for reordered packets. "
         u"Packets which are not found within that range in the other file are considered missing. "
         u"The default is " + UString::Decimal(DEFAULT_BUFFERED_PACKETS) + u" TS packets.");

    option(u"byte-offset", 'b', UNSIGNED);
    help(u"byte-offset", u"Start reading the files at the specified byte offset. The default is 0.");

    option(u"cc-ignore", 0);
    help(u"cc-ignore", u"Ignore continuity counters when comparing packets. Useful if one file has been resynchronized.");

    option(u"continue", 'c');
    help(u"continue", u"Continue the comparison up to the end of files. By default, stop after the first differing packet.");

    option(u"dump", 'd');
    help(u"dump", u"Dump the content of all differing packets.");

    option(u"min-reorder", 'm', POSITIVE);
    help(u"min-reorder", u"count",
         u"With --search-reorder, this is the minimum number of consecutive packets to consider in reordered sequences of packets. "
         u"The default is " + UString::Decimal(DEFAULT_MIN_REORDER) + u" TS packets.");

    option(u"normalized", 'n');
    help(u"normalized", u"Report in a normalized output format (useful for automatic analysis).");

    option(u"packet-offset", 'p', UNSIGNED);
    help(u"packet-offset", u"count", u"Start reading the files at the specified TS packet. The default is 0.");

    option(u"payload-only", 0);
    help(u"payload-only", u"Compare only the payload of the packets, ignore header and adaptation field.");

    option(u"pcr-ignore", 0);
    help(u"pcr-ignore", u"Ignore PCR and OPCR when comparing packets. Useful if one file has been resynchronized.");

    option(u"pid-ignore", 0);
    help(u"pid-ignore", u"Ignore PID value when comparing packets. Useful if one file has gone through a remapping process.");

    option(u"quiet", 'q');
    help(u"quiet",
         u"Do not output any message. The process simply terminates with a success "
         u"status if the files are identical and a failure status if they differ.");

    option(u"search-reorder", 's');
    help(u"search-reorder",
         u"Search missing or reordered packets. "
         u"By default, packets are compared one by one. "
         u"See also --threshold-diff and --buffered-packets.");

    option(u"subset");
    help(u"subset", u"Legacy option, same as --search-reorder");

    option(u"threshold-diff", 't', INTEGER, 0, 1, 0, PKT_SIZE);
    help(u"threshold-diff", u"count",
         u"When used with --search-reorder, this value specifies the maximum number of "
         u"differing bytes in packets to declare them equal. When two packets have "
         u"more differing bytes than this threshold, the packets are reported as "
         u"different and the first file is read ahead. The default is zero, which "
         u"means that two packets must be strictly identical to declare them equal.");

    json.defineArgs(*this, true);

    analyze(argc, argv);

    getValue(filename0, u"", u"", 0);
    getValue(filename1, u"", u"", 1);

    getIntValue(buffered_packets, u"buffered-packets", DEFAULT_BUFFERED_PACKETS);
    byte_offset = intValue<uint64_t>(u"byte-offset", intValue<uint64_t>(u"packet-offset", 0) * PKT_SIZE);
    getIntValue(threshold_diff, u"threshold-diff", 0);
    getIntValue(min_reorder, u"min-reorder", std::min<size_t>(DEFAULT_MIN_REORDER, buffered_packets));
    search_reorder = present(u"subset") || present(u"search-reorder");
    payload_only = present(u"payload-only");
    pcr_ignore = present(u"pcr-ignore");
    pid_ignore = present(u"pid-ignore");
    cc_ignore = present(u"cc-ignore");
    continue_all = present(u"continue");
    quiet = present(u"quiet");
    normalized = !quiet && present(u"normalized");
    dump = !quiet && present(u"dump");
    format = ts::LoadTSPacketFormatInputOption(*this);

    if (!quiet) {
        json.loadArgs(duck, *this);
    }
    if (json.useFile() && normalized) {
        error(u"options --json and --normalized are mutually exclusive");
    }
    if (quiet) {
        setMaxSeverity(Severity::Info);
    }

    dump_flags =
        TSPacket::DUMP_TS_HEADER |    // Format TS headers
        TSPacket::DUMP_PES_HEADER |   // Format PES headers
        TSPacket::DUMP_RAW |          // Full dump of packet
        UString::HEXA |               // Hexadecimal dump (for TSPacket::DUMP_RAW)
        UString::ASCII;               // ASCII dump (for TSPacket::DUMP_RAW)

    exitOnError();
}


// Command line options destructor.
ts::TSCompareOptions::~TSCompareOptions()
{
}


//----------------------------------------------------------------------------
// Packet comparator class
//----------------------------------------------------------------------------

namespace ts {
    class PacketComparator
    {
        TS_NOBUILD_NOCOPY(PacketComparator);
    private:
        TSCompareOptions& _opt;
    public:
        bool   equal;          // Compared packets are identical
        size_t compared_size;  // Size of compared data
        size_t first_diff;     // Offset of first difference
        size_t end_diff;       // Offset of last difference + 1
        size_t diff_count;     // Number of different bytes (can be lower than end_diff-first_diff)

        // Constructor, compare the packets.
        PacketComparator(const TSPacket& pkt1, const TSPacket& pkt2, TSCompareOptions& opt);

    private:
        // Compare two TS memory regions, fill all fields with comparison result.
        void compare(const uint8_t* mem1, size_t size1, const uint8_t* mem2, size_t size2);
    };
}


// Packet comparator constructor.
ts::PacketComparator::PacketComparator(const TSPacket& pkt1, const TSPacket& pkt2, TSCompareOptions& opt) :
    _opt(opt),
    equal(false),
    compared_size(0),
    first_diff(0),
    end_diff(0),
    diff_count(0)
{
    if (pkt1.getPID() == PID_NULL || pkt2.getPID() == PID_NULL) {
        // At least one packet is a null packet.
        compare(pkt1.b, PKT_SIZE, pkt2.b, PKT_SIZE);
        // Null packets are always considered as identical and non-null packets are always considered as different from null packets.
        equal = pkt1.getPID() == PID_NULL && pkt2.getPID() == PID_NULL;
    }
    else if (opt.payload_only) {
        // Compare payload only
        compare(pkt1.getPayload(), pkt1.getPayloadSize(), pkt2.getPayload(), pkt2.getPayloadSize());
    }
    else if (!opt.pcr_ignore && !opt.pid_ignore && !opt.cc_ignore) {
        // Compare full original packets
        compare(pkt1.b, PKT_SIZE, pkt2.b, PKT_SIZE);
    }
    else {
        // Some fields should be ignored, reset them in local copies
        TSPacket p1(pkt1);
        TSPacket p2(pkt2);
        if (opt.pcr_ignore) {
            if (p1.hasPCR()) {
                p1.setPCR(0);
            }
            if (p1.hasOPCR()) {
                p1.setOPCR(0);
            }
            if (p2.hasPCR()) {
                p2.setPCR(0);
            }
            if (p2.hasOPCR()) {
                p2.setOPCR(0);
            }
        }
        if (opt.pid_ignore) {
            p1.setPID(PID_NULL);
            p2.setPID(PID_NULL);
        }
        if (opt.cc_ignore) {
            p1.setCC(0);
            p2.setCC(0);
        }
        compare(p1.b, PKT_SIZE, p2.b, PKT_SIZE);
    }
}


// Compare two memory regions.
void ts::PacketComparator::compare(const uint8_t* mem1, size_t size1, const uint8_t* mem2, size_t size2)
{
    diff_count = 0;
    first_diff = end_diff = compared_size = std::min(size1, size2);
    for (size_t i = 0; i < compared_size; i++) {
        if (mem1[i] != mem2[i]) {
            diff_count++;
            end_diff = i + 1;
            if (first_diff == compared_size) {
                first_diff = i;
            }
        }
    }
    equal = (_opt.search_reorder ? (diff_count <= _opt.threshold_diff) : (diff_count == 0)) && size1 == size2;
}


//----------------------------------------------------------------------------
// Context of one file to compare.
//----------------------------------------------------------------------------

namespace ts {
    class FileToCompare
    {
        TS_NOBUILD_NOCOPY(FileToCompare);
    public:
        // Constructor, open the file.
        FileToCompare(TSCompareOptions& opt, const UString& filename);

        // Get the file name and total read packet count.
        UString fileName() const { return _file.getDisplayFileName(); }
        PacketCounter readPacketsCount() const { return _file.readPacketsCount(); }

        // Check if current packet is after end of file.
        bool eof() const { return _end_of_file && _packet_count == 0; }

        // Access to packet at current or given index.
        const TSPacket& packet() const { return packet(_packet_index); }
        const TSPacket& packet(PacketCounter index) const { return _packets_buffer[size_t(index % _packets_buffer.size())]; }

        // First packet in buffer (index in TS file), number of packets in buffer.
        PacketCounter packetIndex() const { return _packet_index; }
        PacketCounter packetCount() const { return _packet_count; }

        // Access count in PID of a packet at a given index inside the buffer.
        PacketCounter countInPID(PacketCounter index) const { return packetData(index).count_in_pid; }

        // Number of missing packets and chunks.
        PacketCounter missingPackets() const { return _missing_packets; }
        PacketCounter missingChunks() const { return _missing_chunks; }

        // Fill the buffer.
        void fillBuffer();

        // Update first index to next packet, forget previous packets, refill the buffer if necessary.
        void moveNext();

        // Find a sequence of packets (beginning of this buffer's file) in another file.
        bool findPackets(FileToCompare& other, PacketCounter& other_index, PacketCounter& count) const;

        // Mark the corresponding packets as already processed (typically when found in a re-ordered set).
        void ignore(PacketCounter index, PacketCounter count);

        // Declare that the current packet is a missing area.
        void startMissingArea();

        // Check if we are in a missing area. Return either 0 or the number of missing packets. Reset the missing area.
        PacketCounter wasInMissingArea();

    private:
        // Metadata for one packet in the buffer.
        struct PacketData {
            PacketCounter count_in_pid;  // Index of this packet in its PID.
            bool          ignore;        // Ignore this packet, already matched to a packet in other file.
        };

        TSCompareOptions&           _opt;
        std::map<PID,PacketCounter> _by_pid;           // Packet counter per PID.
        TSFile                      _file;
        TSPacketVector              _packets_buffer;
        std::vector<PacketData>     _packets_data;     // One entry per packet at same index in _packets_buffer.
        PacketCounter               _packet_index;     // Index in file of first packet in buffer.
        PacketCounter               _packet_count;     // Number of packets in _packets_buffer (wrap up at end of buffer).
        PacketCounter               _missing_start;    // If not NONE, we are inside a zone of missing packets (missing in the other file).
        PacketCounter               _missing_packets;  // Total numner of missing packets.
        PacketCounter               _missing_chunks;   // Number of holes, missing chunks.
        bool                        _end_of_file;      // End of file or error encountered.

        // Dummy value for no packet index.
        static constexpr PacketCounter NONE = std::numeric_limits<PacketCounter>::max();

        // Access packet metadata
        PacketData& packetData(PacketCounter index) { return _packets_data[size_t(index % _packets_data.size())]; }
        const PacketData& packetData(PacketCounter index) const { return _packets_data[size_t(index % _packets_data.size())]; }

        // Read contiguous packets, at most up to end of buffer.
        void readContiguousPackets();
    };
}


// Constructor of one file to compare.
ts::FileToCompare::FileToCompare(TSCompareOptions& opt, const UString& filename) :
    _opt(opt),
    _by_pid(),
    _file(),
    _packets_buffer(_opt.buffered_packets),
    _packets_data(_opt.buffered_packets),
    _packet_index(0),
    _packet_count(0),
    _missing_start(NONE),
    _missing_packets(0),
    _missing_chunks(0),
    _end_of_file(!_file.openRead(filename, 1, _opt.byte_offset, _opt, _opt.format))
{
    fillBuffer();
}


// Update first index to next packet, refill the buffer if necessary.
void ts::FileToCompare::moveNext()
{
    assert(_packet_count > 0);
    // Move to next logical packet. Skip ignored packets (already matched).
    do {
        _packet_index++;
        _packet_count--;
    } while (_packet_count > 0 && packetData(_packet_index).ignore);
    // Refill buffer when empty.
    if (_packet_count == 0) {
        fillBuffer();
    }
}


// Fill a file buffer.
void ts::FileToCompare::fillBuffer()
{
    // Read only when possible.
    if (!_end_of_file && _packet_count < _packets_buffer.size()) {
        // Read up to the end of buffer.
        readContiguousPackets();
        // Wrap up and read more at beginning of buffer if necessary.
        if (!_end_of_file && _packet_count < _packets_buffer.size()) {
            assert(_packet_index % _packets_buffer.size() == 0);
            readContiguousPackets();
        }
    }
}


// Read contiguous packets, at most up to end of buffer.
void ts::FileToCompare::readContiguousPackets()
{
    // Read up to the end of buffer.
    const size_t start = size_t((_packet_index + _packet_count) % _packets_buffer.size());
    const size_t max_count = std::min(_packets_buffer.size() - size_t(_packet_count), _packets_buffer.size() - start);
    const size_t count = _file.readPackets(&_packets_buffer[start], nullptr, max_count, _opt);
    _end_of_file = count < max_count;
    _packet_count += count;

    // Initialize packet metadata.
    for (size_t i = start; i < start + count; ++i) {
        _packets_data[i].count_in_pid = _by_pid[_packets_buffer[i].getPID()]++;
        _packets_data[i].ignore = false;
    }
}

// Declare that the current packet is a missing area.
void ts::FileToCompare::startMissingArea()
{
    if (_missing_start == NONE) {
        _missing_start = _packet_index;
    }
}

// Check if we are in a missing area. Return either 0 or the number of missing packets. Reset the missing area.
ts::PacketCounter ts::FileToCompare::wasInMissingArea()
{
    if (_missing_start == NONE) {
        return 0;
    }
    else {
        assert(_missing_start < _packet_index);
        const PacketCounter count = _packet_index - _missing_start;
        _missing_start = NONE;
        _missing_packets += count;
        _missing_chunks++;
        return count;
    }
}

// Find a sequence of packets (beginning of this buffer's file) in another file.
bool ts::FileToCompare::findPackets(FileToCompare& other, PacketCounter& other_index, PacketCounter& count) const
{
    // Check only if each buffer has at least --min-reorder packets.
    if (_packet_count >= _opt.min_reorder && other._packet_count >= _opt.min_reorder) {
        const PacketCounter other_last = other._packet_index + other._packet_count - _opt.min_reorder;
        // Try successive slices in other buffer.
        for (other_index = other._packet_index; other_index <= other_last; other_index++) {
            const PacketCounter max_count = std::min(_packet_count, other._packet_count - (other_index - other._packet_index));
            for (count = 0; count < max_count && !packetData(_packet_index + count).ignore && !other.packetData(other_index + count).ignore; count++) {
                const PacketComparator comp(packet(_packet_index + count), other.packet(other_index + count), _opt);
                if (!comp.equal) {
                    break;
                }
            }
            if (count >= _opt.min_reorder) {
                return true;
            }
        }
    }
    other_index = NONE;
    count = 0;
    return false;
}

// Mark the corresponding packets as already processed (typically when found in a re-ordered set).
void ts::FileToCompare::ignore(PacketCounter index, PacketCounter count)
{
    assert(index >= _packet_index);
    assert(index + count <= _packet_index + _packet_count);
    if (index == _packet_index) {
        // Segment is at beginning of buffer, skip it.
        _packet_index += count;
        _packet_count -= count;
        // Skip the ignored packets which could follow.
        while (_packet_count > 0 && packetData(_packet_index).ignore) {
            _packet_index += count;
            _packet_count -= count;
        }
        // Refill the buffer if empty.
        if (_packet_count == 0) {
            fillBuffer();
        }
    }
    else {
        // Mark the segment as ignored.
        for (PacketCounter i = 0; i < count; i++) {
            packetData(index + i).ignore = true;
        }
    }
}


//----------------------------------------------------------------------------
// File comparator class
//----------------------------------------------------------------------------

namespace ts {
    class FileComparator
    {
        TS_NOBUILD_NOCOPY(FileComparator);
    public:
        // Constructor, compare the files.
        FileComparator(TSCompareOptions& opt);

        // Final status.
        bool success;

    private:
        TSCompareOptions& _opt;
        FileToCompare     _file0;
        FileToCompare     _file1;
        json::Object      _jroot;
        PacketCounter     _diff_count;

        void displayHeader();
        void displayFinal();
        void displayOneDifference(const PacketComparator& comp, PacketCounter index0, PacketCounter index1);
        void displayTruncated(size_t file_index, const FileToCompare& file);
        void displayMissingChunk(size_t ref_file_index, FileToCompare& ref_file,
                                 size_t miss_file_index, FileToCompare& miss_file);
        void displayReorder(size_t file0_index, const FileToCompare& file0, PacketCounter packet_index0,
                            size_t file1_index, const FileToCompare& file1, PacketCounter packet_index1,
                            PacketCounter count);
    };
}


// File comparator constructor.
ts::FileComparator::FileComparator(TSCompareOptions& opt) :
    success(false),
    _opt(opt),
    _file0(_opt, _opt.filename0),
    _file1(_opt, _opt.filename1),
    _jroot(),
    _diff_count(0)
{
    // No need to go further if at least one file is on error or empty.
    if (_file0.eof() || _file1.eof()) {
        return;
    }

    displayHeader();

    // Read and compare all packets in the files.
    // Stop at first difference in quiet mode (only report if equal) or not --continue.
    while (!_file0.eof() && !_file1.eof() && (_diff_count == 0 || (!_opt.quiet && _opt.continue_all))) {
        const PacketComparator comp(_file0.packet(), _file1.packet(), _opt);
        if (comp.equal) {
            // Current packets are identical.
            displayMissingChunk(0, _file0, 1, _file1);
            displayMissingChunk(1, _file1, 0, _file0);
            _file0.moveNext();
            _file1.moveNext();
        }
        else if (_opt.search_reorder) {
            // Start a deep comparison in the internal buffers. Make sure that they are full.
            _file0.fillBuffer();
            _file1.fillBuffer();
            PacketCounter index0 = 0;
            PacketCounter index1 = 0;
            PacketCounter count0 = 0;
            PacketCounter count1 = 0;
            const bool moved0 = _file0.findPackets(_file1, index1, count1);
            const bool moved1 = _file1.findPackets(_file0, index0, count0);
            if (!moved0) {
                // The current packet in _file0 is not found in _file1 buffer, consider it as lost.
                _file0.startMissingArea();
                _file0.moveNext();
            }
            if (!moved1) {
                // The current packet in _file1 is not found in _file0 buffer, consider it as lost.
                _file1.startMissingArea();
                _file1.moveNext();
            }
            if (moved0 && moved1) {
                // No missing packet, both sides are found re-ordered.
                const PacketCounter start0 = _file0.packetIndex();
                const PacketCounter start1 = _file1.packetIndex();
                if (index0 >= start0 + count1 && index1 >= start1 + count0) {
                    // Disjoint re-ordered sets of packets, report them both.
                    displayReorder(0, _file0, start0, 1, _file1, index1, count1);
                    _file0.ignore(start0, count1);
                    _file1.ignore(index1, count1);
                    displayReorder(1, _file1, start1, 0, _file0, index0, count0);
                    _file0.ignore(index0, count0);
                    _file1.ignore(start1, count0);
                }
                else if (count1 >= count0) {
                    // Overlapped sets of packets, they cannot be really reordered packets.
                    // The segment at beginning of _file0 is larger than the segment at beginning of _file1, use this one only.
                    displayReorder(0, _file0, start0, 1, _file1, index1, count1);
                    _file0.ignore(start0, count1);
                    _file1.ignore(index1, count1);
                }
                else {
                    // The segment at beginning of _file1 is larger than the segment at beginning of _file0, use this one only.
                    displayReorder(1, _file1, start1, 0, _file0, index0, count0);
                    _file0.ignore(index0, count0);
                    _file1.ignore(start1, count0);
                }
            }
        }
        else {
            // Simply report a difference between packets.
            displayOneDifference(comp, _file0.packetIndex(), _file1.packetIndex());
            _file0.moveNext();
            _file1.moveNext();
        }
    }

    displayMissingChunk(0, _file0, 1, _file1);
    displayMissingChunk(1, _file1, 0, _file0);
    if (_file0.eof() && !_file1.eof()) {
        displayTruncated(0, _file0);
    }
    else if (!_file0.eof() && _file1.eof()) {
        displayTruncated(1, _file1);
    }
    displayFinal();

    success = _diff_count == 0 && _opt.valid() && !_opt.gotErrors();
}


// Display initial headers.
void ts::FileComparator::displayHeader()
{
    if (_opt.json.useJSON()) {
        _jroot.query(u"files[0]", true).add(u"name", AbsoluteFilePath(_file0.fileName()));
        _jroot.query(u"files[1]", true).add(u"name", AbsoluteFilePath(_file1.fileName()));
    }
    else if (!_opt.normalized && _opt.verbose() && !_opt.json.useFile()) {
        std::cout << "* Comparing " << _file0.fileName() << " and " << _file1.fileName() << std::endl;
    }
}


// Display final report.
void ts::FileComparator::displayFinal()
{
    if (_opt.json.useJSON()) {
        json::Value& jv0(_jroot.query(u"files[0]"));
        jv0.add(u"packets", _file0.readPacketsCount());
        jv0.add(u"missing", _file0.missingPackets());
        jv0.add(u"holes", _file0.missingChunks());
        json::Value& jv1(_jroot.query(u"files[1]"));
        jv1.add(u"packets", _file1.readPacketsCount());
        jv1.add(u"missing", _file1.missingPackets());
        jv1.add(u"holes", _file1.missingChunks());
        _jroot.query(u"summary", true).add(u"differences", _diff_count);
    }
    if (_opt.normalized) {
        std::cout << "file:file=1:filename=" << _file0.fileName()
                  << ":packets=" << _file0.readPacketsCount()
                  << ":missing=" << _file0.missingPackets()
                  << ":holes=" << _file0.missingChunks()
                  << ":" << std::endl;
        std::cout << "file:file=2:filename=" << _file1.fileName()
                  << ":packets=" << _file1.readPacketsCount()
                  << ":missing=" << _file1.missingPackets()
                  << ":holes=" << _file1.missingChunks()
                  << ":" << std::endl;
        std::cout << "total:diff=" << _diff_count
                  << ":" << std::endl;
    }
    else if (_opt.verbose() && !_opt.json.useFile()) {
        std::cout << "* Found " << UString::Decimal(_diff_count) << " differences" << std::endl;
        if (_file0.missingPackets() > 0) {
            std::cout << "* " << _file0.fileName() << ", " << UString::Decimal(_file0.readPacketsCount()) << " packets, missing "
                      << UString::Decimal(_file0.missingPackets()) << " packets in " << UString::Decimal(_file0.missingChunks()) << " holes"
                      << std::endl;
        }
        if (_file1.missingPackets() > 0) {
            std::cout << "* " << _file1.fileName() << ", " << UString::Decimal(_file1.readPacketsCount()) << " packets, missing "
                      << UString::Decimal(_file1.missingPackets()) << " packets in " << UString::Decimal(_file1.missingChunks()) << " holes"
                      << std::endl;
        }
    }

    // JSON output if required.
    _opt.json.report(_jroot, std::cout, _opt);
}


// Report a difference in a packet.
void ts::FileComparator::displayOneDifference(const PacketComparator& comp, PacketCounter index0, PacketCounter index1)
{
    _diff_count++;

    const TSPacket& pkt0(_file0.packet(index0));
    const TSPacket& pkt1(_file1.packet(index1));
    const PID pid0 = pkt0.getPID();
    const PID pid1 = pkt1.getPID();
    const PacketCounter index_in_pid0 = _file0.countInPID(index0);
    const PacketCounter index_in_pid1 = _file1.countInPID(index1);

    if (_opt.json.useJSON()) {
        json::Value& jv(_jroot.query(u"events[]", true));
        jv.add(u"type", u"difference");
        jv.add(u"packet", index0);
        jv.add(u"payload-only", json::Bool(_opt.payload_only));
        jv.add(u"offset", comp.first_diff);
        jv.add(u"end-offset", comp.end_diff);
        jv.add(u"diff-bytes", comp.diff_count);
        jv.add(u"comp-size", comp.compared_size);
        jv.add(u"pid0", pid0);
        jv.add(u"pid1", pid1);
        jv.add(u"pid0-index", index_in_pid0);
        jv.add(u"pid1-index", index_in_pid1);
        jv.add(u"same-pid", json::Bool(pid0 == pid1));
        jv.add(u"same-index", json::Bool(index_in_pid0 == index_in_pid1));
    }
    if (_opt.normalized) {
        std::cout << "diff:packet=" << index0
                  << (_opt.payload_only ? ":payload" : "")
                  << ":offset=" << comp.first_diff
                  << ":endoffset=" << comp.end_diff
                  << ":diffbytes= " << comp.diff_count
                  << ":compsize=" << comp.compared_size
                  << ":pid1=" << pid0
                  << ":pid2=" << pid1
                  << (pid1 == pid0 ? ":samepid" : "")
                  << ":pid1index=" << index_in_pid0
                  << ":pid2index=" << index_in_pid1
                  << (index_in_pid0 == index_in_pid1 ? ":sameindex" : "")
                  << ":" << std::endl;
    }
    else if (!_opt.quiet && !_opt.json.useFile()) {
        std::cout << "* Packet " << UString::Decimal(index0) << " differ at offset " << comp.first_diff;
        if (_opt.payload_only) {
            std::cout << " in payload";
        }
        std::cout << ", " << comp.diff_count;
        if (comp.diff_count != comp.end_diff - comp.first_diff) {
            std::cout << "/" << (comp.end_diff - comp.first_diff);
        }
        std::cout << " bytes differ, PID " << pid0;
        if (pid1 != pid0) {
            std::cout << "/" << pid1;
        }
        std::cout << ", packet " << UString::Decimal(index_in_pid0);
        if (pid0 != pid1 || index_in_pid0 != index_in_pid1) {
            std::cout << "/" << UString::Decimal(index_in_pid1);
        }
        std::cout << " in PID" << std::endl;
        if (_opt.dump) {
            std::cout << "  Packet from " << _file0.fileName() << ":" << std::endl;
            pkt0.display (std::cout, _opt.dump_flags, 6);
            std::cout << "  Packet from " << _file1.fileName() << ":" << std::endl;
            pkt1.display (std::cout, _opt.dump_flags, 6);
            std::cout << "  Differing area from " << _file0.fileName() << ":" << std::endl
                      << UString::Dump(pkt0.b + (_opt.payload_only ? pkt0.getHeaderSize() : 0) + comp.first_diff, comp.end_diff - comp.first_diff, _opt.dump_flags, 6)
                      << "  Differing area from " << _file1.fileName() << ":" << std::endl
                      << UString::Dump(pkt1.b + (_opt.payload_only ? pkt1.getHeaderSize() : 0) + comp.first_diff, comp.end_diff - comp.first_diff, _opt.dump_flags, 6);
        }
    }
}


// Report a truncated file.
void ts::FileComparator::displayTruncated(size_t file_index, const FileToCompare& file)
{
    if (_opt.json.useJSON()) {
        json::Value& jv(_jroot.query(u"events[]", true));
        jv.add(u"type", u"truncated");
        jv.add(u"packet", file.readPacketsCount());
        jv.add(u"file-index", file_index);
    }
    if (_opt.normalized) {
        std::cout << "truncated:file=" << file_index << ":packet=" << file.readPacketsCount() << ":filename=" << file.fileName() << ":" << std::endl;
    }
    else if (!_opt.quiet && !_opt.json.useFile()) {
        std::cout << "* Packet " << UString::Decimal(file.readPacketsCount()) << ": file " << file.fileName() << " is truncated" << std::endl;
    }
    _diff_count++;
}


// Report resynchronization after missing packets
void ts::FileComparator::displayMissingChunk(size_t ref_file_index, FileToCompare& ref_file, size_t miss_file_index, FileToCompare& miss_file)
{
    const PacketCounter count = ref_file.wasInMissingArea();
    if (count > 0) {
        const PacketCounter start = ref_file.packetIndex() - count;
        if (_opt.json.useJSON()) {
            json::Value& jv(_jroot.query(u"events[]", true));
            jv.add(u"type", u"skipped");
            jv.add(u"packet", start);
            jv.add(u"skipped", count);
            jv.add(u"miss-file-index", miss_file_index);
            jv.add(u"ref-file-index", ref_file_index);
        }
        if (_opt.normalized) {
            std::cout << "skip:file=" << miss_file_index << ":packet=" << start << ":skipped=" << count << ":" << std::endl;
        }
        else if (!_opt.quiet && !_opt.json.useFile()) {
            std::cout << "* Packet " << UString::Decimal(start) << " in " << ref_file.fileName()
                      << ", missing " << UString::Decimal(count) << " packets in " << miss_file.fileName()
                      << std::endl;
        }
        _diff_count++;
    }
}

// Report packets in the wrong order.
void ts::FileComparator::displayReorder(size_t file0_index, const FileToCompare& file0, PacketCounter packet_index0,
                                        size_t file1_index, const FileToCompare& file1, PacketCounter packet_index1,
                                        PacketCounter count)
{
    if (_opt.json.useJSON()) {
        json::Value& jv(_jroot.query(u"events[]", true));
        jv.add(u"type", u"out-of-order");
        jv.add(u"count", count);
        jv.add(UString::Format(u"packet%d", {file0_index}), packet_index0);
        jv.add(UString::Format(u"packet%d", {file1_index}), packet_index1);
    }
    if (_opt.normalized) {
        std::cout << "outoforder:count=" << count << ":packet" << file0_index << "=" << packet_index0 << ":packet" << file1_index << "=" << packet_index1 << ":" << std::endl;
    }
    else if (!_opt.quiet && !_opt.json.useFile()) {
        std::cout << "* " << UString::Decimal(count) << " out of order packets"
                  << ", at index " << UString::Decimal(packet_index0) << " in file " << file0.fileName()
                  << ", at index " << UString::Decimal(packet_index1) << " in file " << file1.fileName()
                  << std::endl;
    }
    _diff_count++;
}


//----------------------------------------------------------------------------
// Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    ts::TSCompareOptions opt(argc, argv);
    ts::FileComparator comp(opt);
    return comp.success ? EXIT_SUCCESS : EXIT_FAILURE;
}
