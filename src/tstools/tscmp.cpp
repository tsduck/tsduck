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
//  Compare two TS files
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsMemory.h"
#include "tsTSFileInputBuffered.h"
#include "tsBinaryTable.h"
#include "tsSection.h"
#include "tsPMT.h"
#include "tsStreamIdentifierDescriptor.h"
TSDUCK_SOURCE;
TS_MAIN(MainCode);

#define DEFAULT_BUFFERED_PACKETS 10000


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::TSPacketFormat format;
        ts::UString        filename1;
        ts::UString        filename2;
        uint64_t           byte_offset;
        size_t             buffered_packets;
        size_t             threshold_diff;
        bool               subset;
        bool               dump;
        uint32_t           dump_flags;
        bool               normalized;
        bool               quiet;
        bool               payload_only;
        bool               pcr_ignore;
        bool               pid_ignore;
        bool               cc_ignore;
        bool               continue_all;
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Compare two transport stream files", u"[options] filename-1 filename-2"),
    format(ts::TSPacketFormat::AUTODETECT),
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
    continue_all(false)
{
    option(u"", 0, STRING, 2, 2);
    help(u"", u"MPEG capture files to be compared.");

    option(u"buffered-packets", 0, UNSIGNED);
    help(u"buffered-packets",
         u"Specifies the files input buffer size in TS packets.\n"
         u"The default is " + ts::UString::Decimal(DEFAULT_BUFFERED_PACKETS) + u" TS packets.");

    option(u"byte-offset", 'b', UNSIGNED);
    help(u"byte-offset", u"Start reading the files at the specified byte offset (default: 0).");

    option(u"cc-ignore", 0);
    help(u"cc-ignore", u"Ignore continuity counters when comparing packets. Useful if one file has been resynchronized.");

    option(u"continue", 'c');
    help(u"continue", u"Continue the comparison up to the end of files. By default, stop after the first differing packet.");

    option(u"dump", 'd');
    help(u"dump", u"Dump the content of all differing packets.");

    option(u"format", 'f', ts::TSPacketFormatEnum);
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

    option(u"threshold-diff", 't', INTEGER, 0, 1, 0, ts::PKT_SIZE);
    help(u"threshold-diff",
         u"When used with --subset, this value specifies the maximum number of "
         u"differing bytes in packets to declare them equal. When two packets have "
         u"more differing bytes than this threshold, the packets are reported as "
         u"different and the first file is read ahead. The default is zero, which "
         u"means that two packets must be strictly identical to declare them equal.");

    analyze(argc, argv);

    getValue(filename1, u"", u"", 0);
    getValue(filename2, u"", u"", 1);

    format = enumValue<ts::TSPacketFormat>(u"format", ts::TSPacketFormat::AUTODETECT);
    buffered_packets = intValue<size_t>(u"buffered-packets", DEFAULT_BUFFERED_PACKETS);
    byte_offset = intValue<uint64_t>(u"byte-offset", intValue<uint64_t>(u"packet-offset", 0) * ts::PKT_SIZE);
    threshold_diff = intValue<size_t>(u"threshold-diff", 0);
    subset = present(u"subset");
    payload_only = present(u"payload-only");
    pcr_ignore = present(u"pcr-ignore");
    pid_ignore = present(u"pid-ignore");
    cc_ignore = present(u"cc-ignore");
    continue_all = present(u"continue");
    quiet = present(u"quiet");
    normalized = !quiet && present(u"normalized");
    dump = !quiet && present(u"dump");

    if (quiet) {
        setMaxSeverity(ts::Severity::Info);
    }

    dump_flags =
        ts::TSPacket::DUMP_TS_HEADER |    // Format TS headers
        ts::TSPacket::DUMP_PES_HEADER |   // Format PES headers
        ts::TSPacket::DUMP_RAW |          // Full dump of packet
        ts::UString::HEXA |               // Hexadecimal dump (for TSPacket::DUMP_RAW)
        ts::UString::ASCII;               // ASCII dump (for TSPacket::DUMP_RAW)

    exitOnError();
}


//----------------------------------------------------------------------------
//  Packet comparator class
//----------------------------------------------------------------------------

class Comparator
{
public:
    bool   equal;          // Compared packets are identical
    size_t compared_size;  // Size of compared data
    size_t first_diff;     // Offset of first difference
    size_t end_diff;       // Offset of last difference + 1
    size_t diff_count;     // Number of different bytes (can be lower than end_diff-first_diff)

    // Constructor
    Comparator(const ts::TSPacket& pkt1, const ts::TSPacket& pkt2, Options& opt);

    // Compare two TS packets, return equal
    bool compare(const ts::TSPacket& pkt1, const ts::TSPacket& pkt2, Options& opt);

private:
    // Compare two TS memory regions, return equal
    bool compare(const uint8_t* mem1, size_t size1, const uint8_t* mem2, size_t size2);
};


//----------------------------------------------------------------------------
//  Packet comparator constructor.
//----------------------------------------------------------------------------

Comparator::Comparator(const ts::TSPacket& pkt1, const ts::TSPacket& pkt2, Options& opt) :
    equal(false),
    compared_size(0),
    first_diff(0),
    end_diff(0),
    diff_count(0)
{
    compare(pkt1, pkt2, opt);
}

//----------------------------------------------------------------------------
//  Compare two memory regions.
//----------------------------------------------------------------------------

bool Comparator::compare(const uint8_t* mem1, size_t size1, const uint8_t* mem2, size_t size2)
{
    diff_count = 0;
    first_diff = end_diff = compared_size = std::min (size1, size2);
    for (size_t i = 0; i < compared_size; i++) {
        if (mem1[i] != mem2[i]) {
            diff_count++;
            end_diff = i + 1;
            if (first_diff == compared_size) {
                first_diff = i;
            }
        }
    }
    return equal = diff_count == 0 && size1 == size2;
}


//----------------------------------------------------------------------------
//  Compare two packets according to options.
//  Offset in packet or in payload, depending on opt.payload_only.
//----------------------------------------------------------------------------

bool Comparator::compare(const ts::TSPacket& pkt1, const ts::TSPacket& pkt2, Options& opt)
{
    if (pkt1.getPID() == ts::PID_NULL || pkt2.getPID() == ts::PID_NULL) {
        // Null packets are always considered as identical.
        // Non-null packets are always considered as different from null packets.
        compare(pkt1.b, ts::PKT_SIZE, pkt2.b, ts::PKT_SIZE);
        return equal = pkt1.getPID() == ts::PID_NULL && pkt2.getPID() == ts::PID_NULL;
    }
    else if (opt.payload_only) {
        // Compare payload only
        return compare(pkt1.getPayload(), pkt1.getPayloadSize(), pkt2.getPayload(), pkt2.getPayloadSize());
    }
    else if (!opt.pcr_ignore && !opt.pid_ignore && !opt.cc_ignore) {
        // Compare full original packets
        return compare(pkt1.b, ts::PKT_SIZE, pkt2.b, ts::PKT_SIZE);
    }
    else {
        // Some fields should be ignored, reset them to zero in local copies
        ts::TSPacket p1, p2;
        p1 = pkt1;
        p2 = pkt2;
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
            p1.setPID(ts::PID_NULL);
            p2.setPID(ts::PID_NULL);
        }
        if (opt.cc_ignore) {
            p1.setCC(0);
            p2.setCC(0);
        }
        return compare(p1.b, ts::PKT_SIZE, p2.b, ts::PKT_SIZE);
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt (argc, argv);
    ts::TSFileInputBuffered file1(opt.buffered_packets);
    ts::TSFileInputBuffered file2(opt.buffered_packets);

    // Open files
    file1.openRead(opt.filename1, 1, opt.byte_offset, opt, opt.format);
    file2.openRead(opt.filename2, 1, opt.byte_offset, opt, opt.format);
    opt.exitOnError();

    // Display headers
    if (opt.normalized) {
        std::cout << "file:file=1:filename=" << file1.getFileName() << ":" << std::endl
                  << "file:file=2:filename=" << file2.getFileName() << ":" << std::endl;

    }
    else if (opt.verbose()) {
        std::cout << "* Comparing " << file1.getFileName() << " and " << file2.getFileName() << std::endl;
    }

    // Count packets in PIDs in each file
    ts::PacketCounter count1[ts::PID_MAX];
    ts::PacketCounter count2[ts::PID_MAX];
    TS_ZERO(count1);
    TS_ZERO(count2);

    // Currently skipped packets in file1 when --subset
    ts::PacketCounter subset_skipped = 0;
    ts::PacketCounter total_subset_skipped = 0;
    ts::PacketCounter subset_skipped_chunks = 0;

    // Number of differences in file
    ts::PacketCounter diff_count = 0;

    // Read and compare all packets in the files
    ts::TSPacket pkt1, pkt2;
    size_t read2 = 0;
    ts::PID pid2 = ts::PID_NULL;

    for (;;) {

        // Read one packet in file1
        size_t read1 = file1.read (&pkt1, 1, opt);
        ts::PID pid1 = pkt1.getPID();
        count1[pid1]++;

        // If currently not skipping packets, read one packet in file2
        if (subset_skipped == 0) {
            read2 = file2.read (&pkt2, 1, opt);
            pid2 = pkt2.getPID();
            count2[pid2]++;
        }

        // Exit if at least one file is terminated
        if (read1 == 0 || read2 == 0) {
            if (read1 != 0 || read2 != 0) {
                diff_count++;
            }
            if (read1 != 0) {
                // File 2 is truncated
                if (opt.normalized) {
                    std::cout << "truncated:file=2:packet=" << file2.readPacketsCount()
                              << ":filename=" << file2.getFileName() << ":" << std::endl;
                }
                else if (!opt.quiet) {
                    std::cout << "* Packet " << ts::UString::Decimal(file2.readPacketsCount())
                              << ": file " << file2.getFileName() << " is truncated" << std::endl;
                }
            }
            if (read2 != 0) {
                // File 1 is truncated
                if (opt.normalized) {
                    std::cout << "truncated:file=1:packet=" << file1.readPacketsCount()
                              << ":filename=" << file1.getFileName() << ":" << std::endl;
                }
                else if (!opt.quiet) {
                    std::cout << "* Packet " << ts::UString::Decimal(file1.readPacketsCount())
                              << ": file " << file1.getFileName() << " is truncated" << std::endl;
                }
            }
            break;
        }

        // Compare one packet
        const Comparator comp (pkt1, pkt2, opt);

        // If file2 is a subset of file1 and an inacceptable difference has been found, read ahead file1.
        if (opt.subset && !comp.equal && comp.diff_count > opt.threshold_diff) {
            subset_skipped++;
            continue;
        }

        // Report resynchronization after missing packets
        if (subset_skipped > 0) {
            if (opt.normalized) {
                std::cout << "skip:packet=" << (file1.readPacketsCount() - 1 - subset_skipped)
                          << ":skipped=" << ts::UString::Decimal(subset_skipped)
                          << ":" << std::endl;
            }
            else {
                std::cout << "* Packet " << ts::UString::Decimal(file1.readPacketsCount() - 1 - subset_skipped)
                          << ", missing " << ts::UString::Decimal(subset_skipped)
                          << " packets in " << file2.getFileName() << std::endl;
            }
            total_subset_skipped += subset_skipped;
            subset_skipped_chunks++;
            subset_skipped = 0;
        }

        // Report a difference
        if (!comp.equal) {
            diff_count++;
            if (opt.normalized) {
                std::cout << "diff:packet=" << (file1.readPacketsCount() - 1)
                          << (opt.payload_only ? ":payload" : "")
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
            else if (!opt.quiet) {
                std::cout << "* Packet " << ts::UString::Decimal(file1.readPacketsCount() - 1) << " differ at offset " << comp.first_diff;
                if (opt.payload_only) {
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
                std::cout << ", packet " << ts::UString::Decimal(count1[pid1] - 1);
                if (pid2 != pid1 || count2[pid2] != count1[pid1]) {
                    std::cout << "/" << ts::UString::Decimal(count2[pid2] - 1);
                }
                std::cout << " in PID" << std::endl;
                if (opt.dump) {
                    std::cout << "  Packet from " << file1.getFileName() << ":" << std::endl;
                    pkt1.display (std::cout, opt.dump_flags, 6);
                    std::cout << "  Packet from " << file2.getFileName() << ":" << std::endl;
                    pkt2.display (std::cout, opt.dump_flags, 6);
                    std::cout << "  Differing area from " << file1.getFileName() << ":" << std::endl
                              << ts::UString::Dump(pkt1.b + (opt.payload_only ? pkt1.getHeaderSize() : 0) + comp.first_diff,
                                                   comp.end_diff - comp.first_diff, opt.dump_flags, 6)
                              << "  Differing area from " << file2.getFileName() << ":" << std::endl
                              << ts::UString::Dump(pkt2.b + (opt.payload_only ? pkt2.getHeaderSize() : 0) + comp.first_diff,
                                                   comp.end_diff - comp.first_diff, opt.dump_flags, 6);
                }
            }
            if (opt.quiet || !opt.continue_all) {
                break;
            }
        }
    }

    // Final report
    if (opt.normalized) {
        std::cout << "total:packets=" << file1.readPacketsCount()
                  << ":diff=" << diff_count
                  << ":missing=" << total_subset_skipped
                  << ":holes=" << subset_skipped_chunks
                  << ":" << std::endl;
    }
    else if (opt.verbose()) {
        std::cout << "* Read " << ts::UString::Decimal(file1.readPacketsCount())
                  << " packets, found " << ts::UString::Decimal(diff_count) << " differences";
        if (subset_skipped_chunks > 0) {
            std::cout << ", missing " << ts::UString::Decimal(total_subset_skipped)
                      << " packets in " << ts::UString::Decimal(subset_skipped_chunks) << " holes";
        }
        std::cout << std::endl;
    }

    // End of processing, close file
    file1.close (opt);
    file2.close (opt);
    return diff_count == 0 && opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
