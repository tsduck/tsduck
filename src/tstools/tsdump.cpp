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
//  Dump the packets from a transport stream.
//  Also generic hexa/ascii dump utility (option --raw).
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsTSPacket.h"
#include "tsPagerArgs.h"
#include "tsUString.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);
    virtual ~Options();

    uint32_t          dump_flags;  // Dump options for Hexa and Packet::dump
    bool              raw_file;    // Raw dump of file, not TS packets
    bool              log;         // Option --log
    size_t            log_size;    // Size to display with --log
    ts::PIDSet        pids;        // PID values to dump.
    ts::PacketCounter max_packets; // Maximum number of packets to dump per file
    ts::UStringVector infiles;     // Input file names
    ts::PagerArgs     pager;       // Output paging options.
};

Options::Options(int argc, char *argv[]) :
    Args(u"Dump and format MPEG transport stream packets", u"[options] [filename ...]"),
    dump_flags(0),
    raw_file(false),
    log(false),
    log_size(0),
    pids(),
    max_packets(0),
    infiles(),
    pager(true, true)
{
    pager.defineOptions(*this);

    option(u"", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"", u"Any number of input MPEG TS files (standard input if omitted).");

    option(u"ascii", 'a');
    help(u"ascii", u"Include ASCII dump in addition to hexadecimal.");

    option(u"binary", 'b');
    help(u"binary", u"Include binary dump in addition to hexadecimal.");

    option(u"c-style", 'c');
    help(u"c-style", u"Same as --raw-dump (no interpretation of packet) but dump the bytes in C-language style.");

    option(u"headers-only", 'h');
    help(u"headers-only", u"Dump packet headers only, not payload.");

    option(u"log", 'l');
    help(u"log", u"Display a short one-line log of each packet instead of full dump.");

    option(u"log-size", 0, UNSIGNED);
    help(u"log-size",
         u"With option --log, specify how many bytes are displayed in each packet. "
         u"The default is 188 bytes (complete packet).");

    option(u"max-packets", 'm', UNSIGNED);
    help(u"max-packets", u"Maximum number of packets to dump per file.");

    option(u"nibble", 'n');
    help(u"nibble", u"Same as --binary but add separator between 4-bit nibbles.");

    option(u"no-headers");
    help(u"no-headers", u"Do not display header information.");

    option(u"offset", 'o');
    help(u"offset", u"Include offset from start of packet with hexadecimal dump.");

    option(u"payload");
    help(u"payload", u"Hexadecimal dump of TS payload only, skip TS header.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Dump only packets with these PID values. "
         u"Several --pid options may be specified. "
         u"By default, all packets are displayed.");

    option(u"raw-file", 'r');
    help(u"raw-file", u"Raw dump of file, do not interpret as TS packets.");

    analyze(argc, argv);

    pager.load(*this);

    getValues(infiles);
    raw_file = present(u"raw-file");
    log = present(u"log");
    max_packets = intValue<ts::PacketCounter>(u"max-packets", std::numeric_limits<ts::PacketCounter>::max());
    log_size = intValue<size_t>(u"log-size", ts::PKT_SIZE);
    getIntValues(pids, u"pid", true);

    dump_flags =
        ts::TSPacket::DUMP_TS_HEADER |    // Format TS headers
        ts::TSPacket::DUMP_PES_HEADER |   // Format PES headers
        ts::TSPacket::DUMP_RAW |          // Full dump of packet
        ts::UString::HEXA;                // Hexadecimal dump (for TSPacket::DUMP_RAW)

    if (present(u"ascii")) {
        dump_flags |= ts::UString::ASCII;
    }
    if (present(u"binary")) {
        dump_flags |= ts::UString::BINARY;
    }
    if (present(u"c-style")) {
        dump_flags |= ts::UString::C_STYLE;
        raw_file = true;
    }
    if (log) {
        dump_flags |= ts::UString::SINGLE_LINE;
    }
    if (present(u"headers-only")) {
        dump_flags &= ~ts::TSPacket::DUMP_RAW;
    }
    if (present(u"no-headers")) {
        dump_flags &= ~ts::TSPacket::DUMP_TS_HEADER;
    }
    if (present(u"nibble")) {
        dump_flags |= ts::UString::BIN_NIBBLE | ts::UString::BINARY;
    }
    if (present(u"offset")) {
        dump_flags |= ts::UString::OFFSET;
    }
    if (present(u"payload")) {
        dump_flags &= ~ts::TSPacket::DUMP_RAW;
        dump_flags |= ts::TSPacket::DUMP_PAYLOAD;
    }

    // Filter TS-specific options when used with --raw-file.
    if (raw_file && (log || present(u"max-packets") || present(u"pid") || present(u"headers-only") || present(u"no-headers") || present(u"payload"))) {
        error(u"--raw-file is incompatible with TS-specific options --pid --log --max-packets --headers-only --no-header --payload");
    }

    exitOnError();
}

Options::~Options()
{
}


//----------------------------------------------------------------------------
// Perform the dump on one input file.
//----------------------------------------------------------------------------

void DumpFile(Options& opt, std::istream& in, std::ostream & out)
{
    if (opt.raw_file) {
        // Raw dump of file
        const uint32_t flags = (opt.dump_flags & 0x0000FFFF) | ts::UString::BPL | ts::UString::WIDE_OFFSET;
        const size_t MAX_RAW_BPL = 16;
        const size_t raw_bpl = (flags & ts::UString::BINARY) ? 8 : 16;  // Bytes per line in raw mode
        size_t offset = 0;
        while (in) {
            int c;
            size_t size;
            uint8_t buffer[MAX_RAW_BPL];
            for (size = 0; size < raw_bpl && (c = in.get()) != EOF; size++) {
                buffer[size] = uint8_t(c);
            }
            out << ts::UString::Dump(buffer, size, flags, 0, raw_bpl, offset);
            offset += size;
        }
    }
    else {
        // Read all packets in the file
        ts::TSPacket pkt;
        for (ts::PacketCounter packet_index = 0; packet_index < opt.max_packets && pkt.read(in, true, opt); packet_index++) {
            if (opt.pids.test(pkt.getPID())) {
                if (!opt.log) {
                    out << std::endl << "* Packet " << ts::UString::Decimal(packet_index) << std::endl;
                }
                pkt.display(out, opt.dump_flags, opt.log ? 0 : 2, opt.log_size);
            }
        }
        if (!opt.log) {
            out << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Decode command line.
    Options opt(argc, argv);
    std::ostream& out(opt.pager.output(opt));

    if (opt.infiles.empty()) {
        // Try to put standard input in binary mode
        SetBinaryModeStdin(opt);
        // Dump standard input.
        DumpFile(opt, std::cin, out);
    }
    else {
        // Dump named files.
        for (size_t i = 0; i < opt.infiles.size(); ++i) {
            // Open the file in binary mode.
            std::ifstream file(opt.infiles[i].toUTF8().c_str(), std::ios::binary);
            if (file) {
                if (opt.infiles.size() > 1 && !opt.raw_file && !opt.log) {
                    out << "* File " << opt.infiles[i] << std::endl;
                }
                DumpFile(opt, file, out);
            }
            else {
                opt.error(u"cannot open file %s", {opt.infiles[i]});
            }
        }
    }

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}

TS_MAIN(MainCode)
