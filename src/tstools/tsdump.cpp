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
//  Dump the packets from a transport stream.
//  Also generic hexa/ascii dump utility (option --raw).
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsInputRedirector.h"
#include "tsTSPacket.h"
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
    Args(u"Dump and format MPEG transport stream packets", u"[options] [filename]"),
    dump_flags(0),
    raw_file(false),
    infile()
{
    option(u"", 0, STRING, 0, 1);
    help(u"", u"Input MPEG capture file (standard input if omitted).");

    option(u"ascii", 'a');
    help(u"ascii", u"Include ASCII dump in addition to hexadecimal.");

    option(u"binary", 'b');
    help(u"binary", u"Include binary dump in addition to hexadecimal.");

    option(u"c-style", 'c');
    help(u"c-style", u"Same as --raw-dump (no interpretation of packet) but dump the bytes in C-language style.");

    option(u"headers-only", 'h');
    help(u"headers-only", u"Dump packet headers only, not payload.");

    option(u"nibble", 'n');
    help(u"nibble", u"Same as --binary but add separator between 4-bit nibbles.");

    option(u"offset", 'o');
    help(u"offset", u"Include offset from start of packet with hexadecimal dump.");

    option(u"payload", 'p');
    help(u"payload", u"Hexadecimal dump of TS payload only, skip TS header.");

    option(u"raw-file", 'r');
    help(u"raw-file", u"Raw dump of file, do not interpret as TS packets.");

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

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
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

TSDuckMain(MainCode)
