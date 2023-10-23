//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsTSDumpArgs.h"
#include "tsTSPacket.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TSDumpArgs::defineArgs(Args& args)
{
    args.option(u"adaptation-field");
    args.help(u"adaptation-field", u"Include formatting of the adaptation field.");

    args.option(u"ascii", 'a');
    args.help(u"ascii", u"Include ASCII dump in addition to hexadecimal.");

    args.option(u"binary", 'b');
    args.help(u"binary", u"Include binary dump in addition to hexadecimal.");

    args.option(u"headers-only", 'h');
    args.help(u"headers-only", u"Dump packet headers only, not payload.");

    args.option(u"log", 'l');
    args.help(u"log", u"Display a short one-line log of each packet instead of full dump.");

    args.option(u"log-size", 0, Args::UNSIGNED);
    args.help(u"log-size",
              u"With option --log, specify how many bytes are displayed in each packet. "
              u"The default is 188 bytes (complete packet).");

    args.option(u"nibble", 'n');
    args.help(u"nibble", u"Same as --binary but add separator between 4-bit nibbles.");

    args.option(u"no-headers");
    args.help(u"no-headers", u"Do not display header information.");

    args.option(u"offset", 'o');
    args.help(u"offset", u"Include offset from start of packet with hexadecimal dump.");

    args.option(u"payload");
    args.help(u"payload", u"Hexadecimal dump of TS payload only, skip TS header.");

    args.option(u"pid", 'p', Args::PIDVAL, 0, Args::UNLIMITED_COUNT);
    args.help(u"pid", u"pid1[-pid2]",
              u"Dump only packets with these PID values. "
              u"Several --pid options may be specified. "
              u"By default, all packets are displayed.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::TSDumpArgs::loadArgs(DuckContext& duck, Args& args)
{
    log = args.present(u"log");
    args.getIntValue(log_size, u"log-size", PKT_SIZE);
    args.getIntValues(pids, u"pid", true);

    dump_flags =
        TSPacket::DUMP_TS_HEADER |    // Format TS headers
        TSPacket::DUMP_PES_HEADER |   // Format PES headers
        TSPacket::DUMP_RAW |          // Full dump of packet
        UString::HEXA;                // Hexadecimal dump (for TSPacket::DUMP_RAW)

    if (args.present(u"adaptation-field")) {
        dump_flags |= TSPacket::DUMP_AF;
    }
    if (args.present(u"ascii")) {
        dump_flags |= UString::ASCII;
    }
    if (args.present(u"binary")) {
        dump_flags |= UString::BINARY;
    }
    if (log) {
        dump_flags |= UString::SINGLE_LINE;
    }
    if (args.present(u"headers-only")) {
        dump_flags &= ~TSPacket::DUMP_RAW;
    }
    if (args.present(u"no-headers")) {
        dump_flags &= ~TSPacket::DUMP_TS_HEADER;
    }
    if (args.present(u"nibble")) {
        dump_flags |= UString::BIN_NIBBLE | UString::BINARY;
    }
    if (args.present(u"offset")) {
        dump_flags |= UString::OFFSET;
    }
    if (args.present(u"payload")) {
        dump_flags &= ~TSPacket::DUMP_RAW;
        dump_flags |= TSPacket::DUMP_PAYLOAD;
    }
    return true;
}
