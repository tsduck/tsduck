//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

#include "tsArgs.h"
#include "tsHexa.h"
#include "tsDecimal.h"
#include "tsFormat.h"
#include "tsMemoryUtils.h"
#include "tsTSFileInputBuffered.h"
#include "tsBinaryTable.h"
#include "tsSection.h"
#include "tsPMT.h"
#include "tsStreamIdentifierDescriptor.h"

using namespace ts;

#define DEFAULT_BUFFERED_PACKETS 10000


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public Args
{
    Options (int argc, char *argv[]);

    std::string filename1;
    std::string filename2;
    uint64_t    byte_offset;
    size_t      buffered_packets;
    size_t      threshold_diff;
    bool        subset;
    bool        dump;
    uint32_t    dump_flags;
    bool        normalized;
    bool        verbose;
    bool        quiet;
    bool        payload_only;
    bool        pcr_ignore;
    bool        pid_ignore;
    bool        cc_ignore;
    bool        continue_all;
};

Options::Options (int argc, char *argv[]) :
    Args ("MPEG Transport Stream Files Comparison Utility.", "[options] filename-1 filename-2")
{
    option ("",                 0,  Args::STRING, 2, 2);
    option ("buffered-packets", 0,  UNSIGNED);
    option ("byte-offset",     'b', UNSIGNED);
    option ("cc-ignore",        0);
    option ("continue",        'c');
    option ("dump",            'd');
    option ("normalized",      'n');
    option ("packet-offset",   'p', UNSIGNED);
    option ("payload-only",     0);
    option ("pcr-ignore",       0);
    option ("pid-ignore",       0);
    option ("subset",          's');
    option ("threshold-diff",  't', INTEGER, 0, 1, 0, PKT_SIZE);
    option ("quiet",           'q');
    option ("verbose",         'v');

    setHelp ("Files:\n"
             "\n"
             "  MPEG capture files to be compared.\n"
             "\n"
             "Options:\n"
             "\n"
             "  --buffered-packets value\n"
             "      Specifies the files input buffer size in TS packets.\n"
             "      The default is " + Decimal (DEFAULT_BUFFERED_PACKETS) + " TS packets.\n"
             "\n"
             "  -b value\n"
             "  --byte-offset value\n"
             "      Start reading the files at the specified byte offset (default: 0).\n"
             "\n"
             "  --cc-ignore\n"
             "      Ignore continuity counters when comparing packets. Useful if one file\n"
             "      has been resynchronized.\n"
             "\n"
             "  -c\n"
             "  --continue\n"
             "      Continue the comparison up to the end of files. By default, stop after\n"
             "      the first differing packet.\n"
             "\n"
             "  -d\n"
             "  --dump\n"
             "      Dump the content of all differing packets.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -n\n"
             "  --normalized\n"
             "      Report in a normalized output format (useful for automatic analysis).\n"
             "\n"
             "  -p value\n"
             "  --packet-offset value\n"
             "      Start reading the files at the specified TS packet (default: 0).\n"
             "\n"
             "  --payload-only\n"
             "      Compare only the payload of the packets, ignore header and adaptation\n"
             "      field.\n"
             "\n"
             "  --pcr-ignore\n"
             "      Ignore PCR and OPCR when comparing packets. Useful if one file has been\n"
             "      resynchronized.\n"
             "\n"
             "  --pid-ignore\n"
             "      Ignore PID value when comparing packets. Useful if one file has gone\n"
             "      through a remapping process.\n"
             "\n"
             "  -q\n"
             "  --quiet\n"
             "      Do not output any message. The process simply terminates with a success\n"
             "      status if the files are identical and a failure status if they differ.\n"
             "\n"
             "  -s\n"
             "  --subset\n"
             "      Specifies that the second file is a subset of the first one. This means\n"
             "      that the second file is expected to be identical to the first one, except\n"
             "      that some packets may be missing. When a difference is found, the first\n"
             "      file is read ahead until a matching packet is found.\n"
             "      See also --threshold-diff.\n"
             "\n"
             "  -t value\n"
             "  --threshold-diff value\n"
             "      When used with --subset, this value specifies the maximum number of\n"
             "      differing bytes in packets to declare them equal. When two packets have\n"
             "      more differing bytes than this threshold, the packets are reported as\n"
             "      different and the first file is read ahead. The default is zero, which\n"
             "      means that two packets must be strictly identical to declare them equal.\n"
             "\n"
             "  -v\n"
             "  --verbose\n"
             "      Produce verbose messages.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");

    analyze (argc, argv);

    getValue (filename1, "", "", 0);
    getValue (filename2, "", "", 1);

    buffered_packets = intValue<size_t> ("buffered-packets", DEFAULT_BUFFERED_PACKETS);
    byte_offset = intValue<uint64_t> ("byte-offset", intValue<uint64_t> ("packet-offset", 0) * PKT_SIZE);
    threshold_diff = intValue<size_t> ("threshold-diff", 0);
    subset = present ("subset");
    payload_only = present ("payload-only");
    pcr_ignore = present ("pcr-ignore");
    pid_ignore = present ("pid-ignore");
    cc_ignore = present ("cc-ignore");
    continue_all = present ("continue");
    quiet = present ("quiet");
    normalized = !quiet && present ("normalized");
    verbose = !quiet && present ("verbose");
    dump = !quiet && present ("dump");

    dump_flags = 
        TSPacket::DUMP_TS_HEADER |    // Format TS headers
        TSPacket::DUMP_PES_HEADER |   // Format PES headers
        TSPacket::DUMP_RAW |          // Full dump of packet
        hexa::HEXA |                  // Hexadecimal dump (for TSPacket::DUMP_RAW)
        hexa::ASCII;                  // ASCII dump (for TSPacket::DUMP_RAW)

    exitOnError();
}


//----------------------------------------------------------------------------
//  Packet comparator class
//----------------------------------------------------------------------------

class Comparator {
public:
    bool   equal;          // Compared packets are identical
    size_t compared_size;  // Size of compared data
    size_t first_diff;     // Offset of first difference
    size_t end_diff;       // Offset of last difference + 1
    size_t diff_count;     // Number of different bytes (can be lower than end_diff-first_diff)

    // Constructor
    Comparator (const TSPacket& pkt1, const TSPacket& pkt2, Options& opt)
    {
        compare (pkt1, pkt2, opt);
    }

    // Compare two TS packets, return equal
    bool compare (const TSPacket& pkt1, const TSPacket& pkt2, Options& opt);

private:
    // Compare two TS memory regions, return equal
    bool compare (const uint8_t* mem1, size_t size1, const uint8_t* mem2, size_t size2);
};


//----------------------------------------------------------------------------
//  Compare two memory regions.
//----------------------------------------------------------------------------

bool Comparator::compare (const uint8_t* mem1, size_t size1, const uint8_t* mem2, size_t size2)
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

bool Comparator::compare (const TSPacket& pkt1, const TSPacket& pkt2, Options& opt)
{
    if (pkt1.getPID() == PID_NULL || pkt2.getPID() == PID_NULL) {
        // Null packets are always considered as identical.
        // Non-null packets are always considered as different from null packets.
        compare (pkt1.b, PKT_SIZE, pkt2.b, PKT_SIZE);
        return equal = pkt1.getPID() == PID_NULL && pkt2.getPID() == PID_NULL;
    }
    else if (opt.payload_only) {
        // Compare payload only
        return compare (pkt1.getPayload(), pkt1.getPayloadSize(), pkt2.getPayload(), pkt2.getPayloadSize());
    }
    else if (!opt.pcr_ignore && !opt.pid_ignore && !opt.cc_ignore) {
        // Compare full original packets
        return compare (pkt1.b, PKT_SIZE, pkt2.b, PKT_SIZE);
    }
    else {
        // Some fields should be ignored, reset them to zero in local copies
        TSPacket p1, p2;
        p1 = pkt1;
        p2 = pkt2;
        if (opt.pcr_ignore) {
            if (p1.hasPCR()) {
                p1.setPCR (0);
            }
            if (p1.hasOPCR()) {
                p1.setOPCR (0);
            }
            if (p2.hasPCR()) {
                p2.setPCR (0);
            }
            if (p2.hasOPCR()) {
                p2.setOPCR (0);
            }
        }
        if (opt.pid_ignore) {
            p1.setPID (PID_NULL);
            p2.setPID (PID_NULL);
        }
        if (opt.cc_ignore) {
            p1.setCC (0);
            p2.setCC (0);
        }
        return compare (p1.b, PKT_SIZE, p2.b, PKT_SIZE);
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);
    TSFileInputBuffered file1 (opt.buffered_packets);
    TSFileInputBuffered file2 (opt.buffered_packets);

    // Open files
    file1.open (opt.filename1, 1, opt.byte_offset, opt);
    file2.open (opt.filename2, 1, opt.byte_offset, opt);
    opt.exitOnError();

    // Display headers
    if (opt.normalized) {
        std::cout << "file:file=1:filename=" << file1.getFileName() << ":" << std::endl
                  << "file:file=2:filename=" << file2.getFileName() << ":" << std::endl;
        
    }
    else if (opt.verbose) {
        std::cout << "* Comparing " << file1.getFileName() << " and " << file2.getFileName() << std::endl;
    }

    // Count packets in PIDs in each file
    PacketCounter count1[PID_MAX];
    PacketCounter count2[PID_MAX];
    TS_ZERO (count1);
    TS_ZERO (count2);

    // Currently skipped packets in file1 when --subset
    PacketCounter subset_skipped = 0;
    PacketCounter total_subset_skipped = 0;
    PacketCounter subset_skipped_chunks = 0;

    // Number of differences in file
    PacketCounter diff_count = 0;

    // Read and compare all packets in the files
    TSPacket pkt1, pkt2;
    size_t read1, read2 = 0;
    ts::PID pid1, pid2 = PID_NULL;

    for (;;) {

        // Read one packet in file1
        read1 = file1.read (&pkt1, 1, opt);
        pid1 = pkt1.getPID();
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
                    std::cout << "truncated:file=2:packet=" << file2.getPacketCount()
                              << ":filename=" << file2.getFileName() << ":" << std::endl;
                }
                else if (!opt.quiet) {
                    std::cout << "* Packet " << Decimal (file2.getPacketCount())
                              << ": file " << file2.getFileName() << " is truncated" << std::endl;
                }
            }
            if (read2 != 0) {
                // File 1 is truncated
                if (opt.normalized) {
                    std::cout << "truncated:file=1:packet=" << file1.getPacketCount()
                              << ":filename=" << file1.getFileName() << ":" << std::endl;
                }
                else if (!opt.quiet) {
                    std::cout << "* Packet " << Decimal (file1.getPacketCount())
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
                std::cout << "skip:packet=" << (file1.getPacketCount() - 1 - subset_skipped)
                          << ":skipped=" << Decimal (subset_skipped)
                          << ":" << std::endl;
            }
            else {
                std::cout << "* Packet " << Decimal (file1.getPacketCount() - 1 - subset_skipped)
                          << ", missing " << Decimal (subset_skipped)
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
                std::cout << "diff:packet=" << (file1.getPacketCount() - 1)
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
                std::cout << "* Packet " << Decimal (file1.getPacketCount() - 1) << " differ at offset " << comp.first_diff;
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
                std::cout << ", packet " << Decimal (count1[pid1] - 1);
                if (pid2 != pid1 || count2[pid2] != count1[pid1]) {
                    std::cout << "/" << Decimal (count2[pid2] - 1);
                }
                std::cout << " in PID" << std::endl;
                if (opt.dump) {
                    std::cout << "  Packet from " << file1.getFileName() << ":" << std::endl;
                    pkt1.display (std::cout, opt.dump_flags, 6);
                    std::cout << "  Packet from " << file2.getFileName() << ":" << std::endl;
                    pkt2.display (std::cout, opt.dump_flags, 6);
                    std::cout << "  Differing area from " << file1.getFileName() << ":" << std::endl
                              << Hexa (pkt1.b + (opt.payload_only ? pkt1.getHeaderSize() : 0) + comp.first_diff,
                                       comp.end_diff - comp.first_diff, opt.dump_flags, 6)
                              << "  Differing area from " << file2.getFileName() << ":" << std::endl
                              << Hexa (pkt2.b + (opt.payload_only ? pkt2.getHeaderSize() : 0) + comp.first_diff,
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
        std::cout << "total:packets=" << file1.getPacketCount()
                  << ":diff=" << diff_count
                  << ":missing=" << total_subset_skipped
                  << ":holes=" << subset_skipped_chunks
                  << ":" << std::endl;
    }
    else if (opt.verbose) {
        std::cout << "* Read " << Decimal (file1.getPacketCount())
                  << " packets, found " << Decimal (diff_count) << " differences";
        if (subset_skipped_chunks > 0) {
            std::cout << ", missing " << Decimal (total_subset_skipped)
                      << " packets in " << Decimal (subset_skipped_chunks) << " holes";
        }
        std::cout << std::endl;
    }

    // End of processing, close file
    file1.close (opt);
    file2.close (opt);
    return diff_count == 0 && opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
