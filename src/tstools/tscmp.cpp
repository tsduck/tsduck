//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2022, Thierry Lelegard
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
#include "tsMemory.h"
#include "tsjsonOutputArgs.h"
#include "tsTSFileInputBuffered.h"
#include "tsTextFormatter.h"
#include "tsFileUtils.h"
#include "tsjsonObject.h"
#include "tsjsonString.h"
#include "tsjsonNumber.h"
TS_MAIN(MainCode);

#define DEFAULT_BUFFERED_PACKETS 10000


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
        UString          filename1;
        UString          filename2;
        uint64_t         byte_offset;
        size_t           buffered_packets;
        size_t           threshold_diff;
        bool             subset;
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

ts::TSCompareOptions::~TSCompareOptions()
{
}

ts::TSCompareOptions::TSCompareOptions(int argc, char *argv[]) :
    Args(u"Compare two transport stream files", u"[options] filename-1 filename-2"),
    duck(this),
    format(TSPacketFormat::AUTODETECT),
    filename1(),
    filename2(),
    byte_offset(0),
    buffered_packets(0),
    threshold_diff(0),
    subset(false),
    dump(false),
    dump_flags(0),
    normalized(false),
    quiet(false),
    payload_only(false),
    pcr_ignore(false),
    pid_ignore(false),
    cc_ignore(false),
    continue_all(false),
    json(true)
{
    option(u"", 0, FILENAME, 2, 2);
    help(u"", u"MPEG capture files to be compared.");

    option(u"buffered-packets", 0, UNSIGNED);
    help(u"buffered-packets",
         u"Specifies the files input buffer size in TS packets.\n"
         u"The default is " + UString::Decimal(DEFAULT_BUFFERED_PACKETS) + u" TS packets.");

    option(u"byte-offset", 'b', UNSIGNED);
    help(u"byte-offset", u"Start reading the files at the specified byte offset (default: 0).");

    option(u"cc-ignore", 0);
    help(u"cc-ignore", u"Ignore continuity counters when comparing packets. Useful if one file has been resynchronized.");

    option(u"continue", 'c');
    help(u"continue", u"Continue the comparison up to the end of files. By default, stop after the first differing packet.");

    option(u"dump", 'd');
    help(u"dump", u"Dump the content of all differing packets.");

    option(u"format", 'f', TSPacketFormatEnum);
    help(u"format", u"name",
         u"Specify the format of the input files. "
         u"By default, the format is automatically and independently detected for each file. "
         u"But the auto-detection may fail in some cases "
         u"(for instance when the first time-stamp of an M2TS file starts with 0x47). "
         u"Using this option forces a specific format. "
         u"If a specific format is specified, the two input files must have the same format.");

    option(u"normalized", 'n');
    help(u"normalized", u"Report in a normalized output format (useful for automatic analysis).");

    option(u"packet-offset", 'p', UNSIGNED);
    help(u"packet-offset", u"Start reading the files at the specified TS packet (default: 0).");

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

    option(u"subset", 's');
    help(u"subset",
         u"Specifies that the second file is a subset of the first one. This means "
         u"that the second file is expected to be identical to the first one, except "
         u"that some packets may be missing. When a difference is found, the first "
         u"file is read ahead until a matching packet is found.\n"
         u"See also --threshold-diff.");

    option(u"threshold-diff", 't', INTEGER, 0, 1, 0, PKT_SIZE);
    help(u"threshold-diff",
         u"When used with --subset, this value specifies the maximum number of "
         u"differing bytes in packets to declare them equal. When two packets have "
         u"more differing bytes than this threshold, the packets are reported as "
         u"different and the first file is read ahead. The default is zero, which "
         u"means that two packets must be strictly identical to declare them equal.");

    json.defineArgs(*this);

    analyze(argc, argv);

    getValue(filename1, u"", u"", 0);
    getValue(filename2, u"", u"", 1);

    getIntValue(format, u"format", TSPacketFormat::AUTODETECT);
    getIntValue(buffered_packets, u"buffered-packets", DEFAULT_BUFFERED_PACKETS);
    byte_offset = intValue<uint64_t>(u"byte-offset", intValue<uint64_t>(u"packet-offset", 0) * PKT_SIZE);
    getIntValue(threshold_diff, u"threshold-diff", 0);
    subset = present(u"subset");
    payload_only = present(u"payload-only");
    pcr_ignore = present(u"pcr-ignore");
    pid_ignore = present(u"pid-ignore");
    cc_ignore = present(u"cc-ignore");
    continue_all = present(u"continue");
    quiet = present(u"quiet");
    normalized = !quiet && present(u"normalized");
    dump = !quiet && present(u"dump");

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


//----------------------------------------------------------------------------
// Packet comparator class
//----------------------------------------------------------------------------

namespace ts {
    class PacketComparator
    {
        TS_NOBUILD_NOCOPY(PacketComparator);
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


//----------------------------------------------------------------------------
// Packet comparator constructor.
//----------------------------------------------------------------------------

ts::PacketComparator::PacketComparator(const TSPacket& pkt1, const TSPacket& pkt2, TSCompareOptions& opt) :
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


//----------------------------------------------------------------------------
// Compare two memory regions.
//----------------------------------------------------------------------------

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
    equal = diff_count == 0 && size1 == size2;
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
        TSCompareOptions&   _opt;
        TSFileInputBuffered _file1;
        TSFileInputBuffered _file2;
        json::Object        _json_root;
        PacketCounter       _diff_count;         // Number of differences in file
        PacketCounter       _subset_skipped;     // Currently skipped packets in file1 when --subset
        PacketCounter       _total_subset_skipped;
        PacketCounter       _subset_skipped_chunks;

        // Report a truncated file.
        void reportTruncated(size_t file_index);

        // Report resynchronization after missing packets
        void reportSkippedSubset(PacketCounter remove_count);
    };
}


//----------------------------------------------------------------------------
// File comparator constructor.
//----------------------------------------------------------------------------

ts::FileComparator::FileComparator(TSCompareOptions& opt) :
    success(false),
    _opt(opt),
    _file1(opt.buffered_packets),
    _file2(opt.buffered_packets),
    _json_root(),
    _diff_count(0),
    _subset_skipped(0),
    _total_subset_skipped(0),
    _subset_skipped_chunks(0)
{
    // Open files
    _file1.openRead(_opt.filename1, 1, _opt.byte_offset, _opt, _opt.format);
    _file2.openRead(_opt.filename2, 1, _opt.byte_offset, _opt, _opt.format);
    _opt.exitOnError();

    // Display headers
    if (_opt.json.useJSON()) {
        json::Value& jfiles(_json_root.query(u"files", true, json::Type::Array));
        jfiles.set(AbsoluteFilePath(_file1.getFileName()));
        jfiles.set(AbsoluteFilePath(_file2.getFileName()));
    }
    if (_opt.normalized) {
        std::cout << "file:file=1:filename=" << _file1.getFileName() << ":" << std::endl;
        std::cout << "file:file=2:filename=" << _file2.getFileName() << ":" << std::endl;
    }
    else if (_opt.verbose() && !_opt.json.useFile()) {
        std::cout << "* Comparing " << _file1.getFileName() << " and " << _file2.getFileName() << std::endl;
    }

    // Count packets in PIDs in each file
    PacketCounter count1[PID_MAX];
    PacketCounter count2[PID_MAX];
    TS_ZERO(count1);
    TS_ZERO(count2);

    // Read and compare all packets in the files
    TSPacket pkt1, pkt2;
    size_t read2 = 0;
    PID pid2 = PID_NULL;

    for (;;) {
        // Read one packet in file1
        size_t read1 = _file1.read(&pkt1, 1, _opt);
        PID pid1 = pkt1.getPID();
        count1[pid1]++;

        // If currently not skipping packets, read one packet in file2
        if (_subset_skipped == 0) {
            read2 = _file2.read (&pkt2, 1, _opt);
            pid2 = pkt2.getPID();
            count2[pid2]++;
        }

        // Exit if at least one file is terminated.
        if (read1 == 0 || read2 == 0) {
            // One file is not terminated, the other one is truncated.
            if (read1 != 0 || read2 != 0) {
                if (read1 != 0) {
                    reportTruncated(2);
                }
                if (read2 != 0) {
                    reportTruncated(1);
                }
            }
            break;
        }

        // Compare one packet
        const PacketComparator comp(pkt1, pkt2, _opt);

        // If file2 is a subset of file1 and an inacceptable difference has been found, read ahead file1.
        if (_opt.subset && !comp.equal && comp.diff_count > _opt.threshold_diff) {
            _subset_skipped++;
            continue;
        }

        // Report resynchronization after missing packets.
        // Do not count the current packet from file1, it is not in the skipped part.
        reportSkippedSubset(1);

        // Report a difference
        if (!comp.equal) {
            _diff_count++;
            if (_opt.json.useJSON()) {
                json::Value& jv(_json_root.query(u"events[]", true));
                jv.add(u"type", u"difference");
                jv.add(u"packet", _file1.readPacketsCount() - 1);
                jv.add(u"payload-only", json::Bool(_opt.payload_only));
                jv.add(u"offset", comp.first_diff);
                jv.add(u"end-offset", comp.end_diff);
                jv.add(u"diff-bytes", comp.diff_count);
                jv.add(u"comp-size", comp.compared_size);
                jv.add(u"pid0", pid1);
                jv.add(u"pid1", pid2);
                jv.add(u"pid0-index", count1[pid1] - 1);
                jv.add(u"pid1-index", count2[pid2] - 1);
                jv.add(u"same-pid", json::Bool(pid1 == pid2));
                jv.add(u"same-index", json::Bool(count2[pid2] == count1[pid1]));
            }
            if (_opt.normalized) {
                std::cout << "diff:packet=" << (_file1.readPacketsCount() - 1)
                          << (_opt.payload_only ? ":payload" : "")
                          << ":offset=" << comp.first_diff
                          << ":endoffset=" << comp.end_diff
                          << ":diffbytes= " << comp.diff_count
                          << ":compsize=" << comp.compared_size
                          << ":pid1=" << pid1
                          << ":pid2=" << pid2
                          << (pid1 == pid2 ? ":samepid" : "")
                          << ":pid1index=" << (count1[pid1] - 1)
                          << ":pid2index=" << (count2[pid2] - 1)
                          << (count2[pid2] == count1[pid1] ? ":sameindex" : "")
                          << ":" << std::endl;
            }
            else if (!_opt.quiet && !_opt.json.useFile()) {
                std::cout << "* Packet " << UString::Decimal(_file1.readPacketsCount() - 1) << " differ at offset " << comp.first_diff;
                if (_opt.payload_only) {
                    std::cout << " in payload";
                }
                std::cout << ", " << comp.diff_count;
                if (comp.diff_count != comp.end_diff - comp.first_diff) {
                    std::cout << "/" << (comp.end_diff - comp.first_diff);
                }
                std::cout << " bytes differ, PID " << pid1;
                if (pid2 != pid1) {
                    std::cout << "/" << pid2;
                }
                std::cout << ", packet " << UString::Decimal(count1[pid1] - 1);
                if (pid2 != pid1 || count2[pid2] != count1[pid1]) {
                    std::cout << "/" << UString::Decimal(count2[pid2] - 1);
                }
                std::cout << " in PID" << std::endl;
                if (_opt.dump) {
                    std::cout << "  Packet from " << _file1.getFileName() << ":" << std::endl;
                    pkt1.display (std::cout, _opt.dump_flags, 6);
                    std::cout << "  Packet from " << _file2.getFileName() << ":" << std::endl;
                    pkt2.display (std::cout, _opt.dump_flags, 6);
                    std::cout << "  Differing area from " << _file1.getFileName() << ":" << std::endl
                              << UString::Dump(pkt1.b + (_opt.payload_only ? pkt1.getHeaderSize() : 0) + comp.first_diff,
                                               comp.end_diff - comp.first_diff, _opt.dump_flags, 6)
                              << "  Differing area from " << _file2.getFileName() << ":" << std::endl
                              << UString::Dump(pkt2.b + (_opt.payload_only ? pkt2.getHeaderSize() : 0) + comp.first_diff,
                                               comp.end_diff - comp.first_diff, _opt.dump_flags, 6);
                }
            }
            if (_opt.quiet || !_opt.continue_all) {
                break;
            }
        }
    }

    // Report resynchronization after missing packets in file2, up to its end.
    reportSkippedSubset(0);

    // Final report
    if (_opt.json.useJSON()) {
        json::Value& jv(_json_root.query(u"summary", true));
        jv.add(u"packets", _file1.readPacketsCount());
        jv.add(u"differences", _diff_count);
        jv.add(u"missing", _total_subset_skipped);
        jv.add(u"holes", _subset_skipped_chunks);
    }
    if (_opt.normalized) {
        std::cout << "total:packets=" << _file1.readPacketsCount()
                  << ":diff=" << _diff_count
                  << ":missing=" << _total_subset_skipped
                  << ":holes=" << _subset_skipped_chunks
                  << ":" << std::endl;
    }
    else if (_opt.verbose() && !_opt.json.useFile()) {
        std::cout << "* Read " << UString::Decimal(_file1.readPacketsCount()) << " packets, found " << UString::Decimal(_diff_count) << " differences";
        if (_subset_skipped_chunks > 0) {
            std::cout << ", missing " << UString::Decimal(_total_subset_skipped) << " packets in " << UString::Decimal(_subset_skipped_chunks) << " holes";
        }
        std::cout << std::endl;
    }

    // JSON output if required.
    _opt.json.report(_json_root, std::cout, _opt);

    // End of processing, close file
    _file1.close(_opt);
    _file2.close(_opt);
    success = _diff_count == 0 && _opt.valid() && !_opt.gotErrors();
}


//----------------------------------------------------------------------------
// Report a truncated file.
//----------------------------------------------------------------------------

void ts::FileComparator::reportTruncated(size_t file_index)
{
    TSFileInputBuffered& file(file_index == 1 ? _file1 : _file2);
    if (_opt.json.useJSON()) {
        json::Value& jv(_json_root.query(u"events[]", true));
        jv.add(u"type", u"truncated");
        jv.add(u"packet", file.readPacketsCount());
        jv.add(u"file-index", file_index - 1);
    }
    if (_opt.normalized) {
        std::cout << "truncated:file=" << file_index << ":packet=" << file.readPacketsCount() << ":filename=" << file.getFileName() << ":" << std::endl;
    }
    else if (!_opt.quiet && !_opt.json.useFile()) {
        std::cout << "* Packet " << UString::Decimal(file.readPacketsCount()) << ": file " << file.getFileName() << " is truncated" << std::endl;
    }
    _diff_count++;
}


//----------------------------------------------------------------------------
// Report resynchronization after missing packets
//----------------------------------------------------------------------------

void ts::FileComparator::reportSkippedSubset(PacketCounter remove_count)
{
    if (_subset_skipped > 0) {
        const PacketCounter subset_start = _file1.readPacketsCount() - remove_count - _subset_skipped;
        if (_opt.json.useJSON()) {
            json::Value& jv(_json_root.query(u"events[]", true));
            jv.add(u"type", u"skipped");
            jv.add(u"packet", subset_start);
            jv.add(u"skipped", _subset_skipped);
        }
        if (_opt.normalized) {
            std::cout << "skip:packet=" << subset_start << ":skipped=" << _subset_skipped << ":" << std::endl;
        }
        else if (!_opt.json.useFile()) {
            std::cout << "* Packet " << UString::Decimal(subset_start) << ", missing " << UString::Decimal(_subset_skipped) << " packets in " << _file2.getFileName() << std::endl;
        }
        _total_subset_skipped += _subset_skipped;
        _subset_skipped_chunks++;
        _subset_skipped = 0;
        _diff_count++;
    }
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
