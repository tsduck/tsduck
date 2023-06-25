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
//  Dump the packets from a transport stream.
//  Also generic hexa/ascii dump utility (option --raw).
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsTSPacket.h"
#include "tsTSFile.h"
#include "tsTSDumpArgs.h"
#include "tsPagerArgs.h"
#include "tsDuckContext.h"
#include "tsArgs.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::DuckContext    duck;          // TSDuck context
        bool               raw_file;      // Raw dump of file, not TS packets
        uint64_t           start_offset;  // Start offset in bytes
        ts::PacketCounter  max_packets;   // Maximum number of packets to dump per file
        ts::UStringVector  infiles;       // Input file names
        ts::TSPacketFormat format;        // Input file format
        ts::TSDumpArgs     dump;          // Packet dump options
        ts::PagerArgs      pager;         // Output paging options
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Dump and format MPEG transport stream packets", u"[options] [filename ...]"),
    duck(this),
    raw_file(false),
    start_offset(0),
    max_packets(0),
    infiles(),
    format(ts::TSPacketFormat::AUTODETECT),
    dump(),
    pager(true, true)
{
    dump.defineArgs(*this);
    pager.defineArgs(*this);
    ts::DefineTSPacketFormatInputOption(*this, 'f');

    option(u"", 0, FILENAME, 0, UNLIMITED_COUNT);
    help(u"", u"Any number of input MPEG TS files (standard input if omitted).");

    option(u"byte-offset", 0, UNSIGNED);
    help(u"byte-offset",
         u"Start reading each file at the specified byte offset (default: 0). "
         u"This option is allowed only if all input files are regular files.");

    option(u"c-style", 'c');
    help(u"c-style", u"Same as --raw-dump (no interpretation of packet) but dump the bytes in C-language style.");

    option(u"max-packets", 'm', UNSIGNED);
    help(u"max-packets", u"Maximum number of packets to dump per file.");

    option(u"packet-offset", 0, UNSIGNED);
    help(u"packet-offset",
         u"Start reading each file at the specified TS packet (default: 0). "
         u"This option is allowed only if all input files are regular files.");

    option(u"raw-file", 'r');
    help(u"raw-file", u"Raw dump of file, do not interpret as TS packets.");

    analyze(argc, argv);

    dump.loadArgs(duck, *this);
    pager.loadArgs(duck, *this);

    getValues(infiles);
    raw_file = present(u"raw-file");
    start_offset = intValue<uint64_t>(u"byte-offset", intValue<uint64_t>(u"packet-offset", 0) * ts::PKT_SIZE);
    getIntValue(max_packets, u"max-packets", std::numeric_limits<ts::PacketCounter>::max());
    format = ts::LoadTSPacketFormatInputOption(*this);

    if (present(u"c-style")) {
        dump.dump_flags |= ts::UString::C_STYLE;
        raw_file = true;
    }

    // Filter TS-specific options when used with --raw-file.
    if (raw_file && (dump.log || present(u"max-packets") || (dump.pids.any() && !dump.pids.all()))) {
        error(u"--raw-file is incompatible with TS-specific options --pid --log --max-packets");
    }

    exitOnError();
}


//----------------------------------------------------------------------------
// Perform the dump on one transport stream file.
//----------------------------------------------------------------------------

namespace {
    void DumpTSFile(Options& opt, const ts::UString& filename, std::ostream& out)
    {
        if (opt.infiles.size() > 1 && !opt.dump.log) {
            out << "* File " << filename << std::endl;
        }

        // Open the TS file.
        ts::TSFile file;
        if (!file.openRead(filename, 1, opt.start_offset, opt, opt.format)) {
            return;
        }

        // Read all packets in the file.
        ts::TSPacket pkt;
        for (ts::PacketCounter packet_index = 0; packet_index < opt.max_packets && file.readPackets(&pkt, nullptr, 1, opt) > 0; packet_index++) {
            if (opt.dump.pids.test(pkt.getPID())) {
                if (!opt.dump.log) {
                    out << std::endl << "* Packet " << ts::UString::Decimal(packet_index) << std::endl;
                }
                pkt.display(out, opt.dump.dump_flags, opt.dump.log ? 0 : 2, opt.dump.log_size);
            }
        }
        file.close(opt);

        if (!opt.dump.log) {
            out << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Perform the raw dump on one input file.
//----------------------------------------------------------------------------

namespace {
    void DumpRawFile(Options& opt, const ts::UString& filename, std::ostream& out)
    {
        std::istream* in = nullptr;
        std::ifstream file;

        // Open input file (standard input if no file is specified or file name is empty).
        if (filename.empty() || filename == u"-") {
            // Use standard input.
            in = &std::cin;
            // Try to put standard input in binary mode
            ts::SetBinaryModeStdin(opt);
        }
        else {
            // Dump named files. Open the file in binary mode. Will be closed by destructor.
            in = &file;
            file.open(filename.toUTF8().c_str(), std::ios::binary);
            if (!file) {
                opt.error(u"cannot open file %s", {filename});
                return;
            }
        }

        // Raw dump of file
        const uint32_t flags = (opt.dump.dump_flags & 0x0000FFFF) | ts::UString::BPL | ts::UString::WIDE_OFFSET;
        const size_t MAX_RAW_BPL = 16;
        const size_t raw_bpl = (flags & ts::UString::BINARY) ? 8 : 16;  // Bytes per line in raw mode
        size_t offset = 0;
        while (*in) {
            int c;
            size_t size;
            uint8_t buffer[MAX_RAW_BPL];
            for (size = 0; size < raw_bpl && (c = in->get()) != EOF; size++) {
                buffer[size] = uint8_t(c);
            }
            out << ts::UString::Dump(buffer, size, flags, 0, raw_bpl, offset);
            offset += size;
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

    // Setup an output pager if necessary.
    std::ostream& out(opt.pager.output(opt));

    if (opt.infiles.empty()) {
        // Dump standard input.
        if (opt.raw_file) {
            DumpRawFile(opt, ts::UString(), out);
        }
        else {
            DumpTSFile(opt, ts::UString(), out);
        }
    }
    else {
        // Dump named files.
        for (const auto& name : opt.infiles) {
            if (opt.raw_file) {
                DumpRawFile(opt, name, out);
            }
            else {
                DumpTSFile(opt, name, out);
            }
        }
    }

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
