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
//  Dump the packets from a transport stream.
//  Also generic hexa/ascii dump utility (option --raw).
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsInputRedirector.h"
#include "tsTSPacket.h"
#include "tsVersionInfo.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    uint32_t    dump_flags;  // Dump options for Hexa and Packet::dump
    bool        raw_file;    // Raw dump of file, not TS packets
    ts::UString infile;      // Input file name
};

Options::Options(int argc, char *argv[]) :
    Args(u"MPEG Transport Stream Packet Dump Utility.", u"[options] [filename]"),
    dump_flags(0),
    raw_file(false),
    infile()
{
    option(u"",              0,  Args::STRING, 0, 1);
    option(u"ascii",        'a');
    option(u"binary",       'b');
    option(u"c-style",      'c');
    option(u"headers-only", 'h');
    option(u"nibble",       'n');
    option(u"offset",       'o');
    option(u"payload",      'p');
    option(u"raw-file",     'r');

    setHelp(u"Input file:\n"
            u"\n"
            u"  MPEG capture file (standard input if omitted).\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -a\n"
            u"  --ascii\n"
            u"      Include ASCII dump in addition to hexadecimal.\n"
            u"\n"
            u"  -b\n"
            u"  --binary\n"
            u"      Include binary dump in addition to hexadecimal.\n"
            u"\n"
            u"  -c\n"
            u"  --c-style\n"
            u"      Same as --raw-dump (no interpretation of packet) but dump the\n"
            u"      bytes in C-language style.\n"
            u"\n"
            u"  -h\n"
            u"  --headers-only\n"
            u"      Dump packet headers only, not payload.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -n\n"
            u"  --nibble\n"
            u"      Same as --binary but add separator between 4-bit nibbles.\n"
            u"\n"
            u"  -o\n"
            u"  --offset\n"
            u"      Include offset from start of packet with hexadecimal dump.\n"
            u"\n"
            u"  -p\n"
            u"  --payload\n"
            u"      Hexadecimal dump of TS payload only, skip TS header.\n"
            u"\n"
            u"  -r\n"
            u"  --raw-file\n"
            u"      Raw dump of file, do not interpret as TS packets.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    analyze(argc, argv);

    infile = value(u"");
    raw_file = present(u"raw-file");

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
    if (present(u"headers-only")) {
        dump_flags &= ~ts::TSPacket::DUMP_RAW;
    }
    if (present(u"nibble")) {
        dump_flags |= ts::UString::BIN_NIBBLE | ts::UString::BINARY;
    }
    if (present(u"offset")) {
        dump_flags |= ts::UString::OFFSET;
    }
    if (present(u"payload")) {
        dump_flags &= ~ts::TSPacket::DUMP_RAW;
        dump_flags |= ~ts::TSPacket::DUMP_PAYLOAD;
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    TSDuckLibCheckVersion();
    Options opt(argc, argv);
    ts::InputRedirector input(opt.infile, opt);

    // Dump the file

    if (opt.raw_file) {
        // Raw dump of file
        opt.dump_flags = (opt.dump_flags & 0x0000FFFF) | ts::UString::BPL | ts::UString::WIDE_OFFSET;
        const size_t MAX_RAW_BPL = 16;
        const size_t raw_bpl = (opt.dump_flags & ts::UString::BINARY) ? 8 : 16;  // Bytes per line in raw mode
        size_t offset = 0;
        while (std::cin) {
            int c;
            size_t size;
            uint8_t buffer[MAX_RAW_BPL];
            for (size = 0; size < raw_bpl && (c = std::cin.get()) != EOF; size++) {
                buffer[size] = uint8_t(c);
            }
            std::cout << ts::UString::Dump(buffer, size, opt.dump_flags, 0, raw_bpl, offset);
            offset += size;
        }
    }
    else {
        // Read all packets in the file
        ts::TSPacket pkt;
        for (ts::PacketCounter packet_index = 0; pkt.read(std::cin, true, opt); packet_index++) {
            std::cout << std::endl << "* Packet " << ts::UString::Decimal(packet_index) << std::endl;
            pkt.display (std::cout, opt.dump_flags, 2);
        }
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}
