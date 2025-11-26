//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFluteDemuxArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::FluteDemuxArgs::defineArgs(Args& args)
{
    args.option(u"dump-flute-payload");
    args.help(u"dump-flute-payload",
              u"Same as --log-flute-packets and also dump the payload of each FLUTE packet.");

    args.option(u"log-flute-packets");
    args.help(u"log-flute-packets",
              u"Log a message describing the structure of each FLUTE packet.");

    args.option(u"max-file-size", 0, Args::UINT63);
    args.help(u"max-file-size",
              u"Maximum size of files to analyze or extract. "
              u"Each received file is accumulated in memory, chunk by chunk, until the file is complete. "
              u"After processing the file, all chunks are freeed. "
              u"This option is useful when the stream contains many large files which clutter the memory during their reception. "
              u"All files which are announced as larger than the specified size are ignored. "
              u"By default, all files are received and analyzed or saved.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::FluteDemuxArgs::loadArgs(DuckContext& duck, Args& args)
{
    dump_flute_payload = args.present(u"dump-flute-payload");
    log_flute_packets = dump_flute_payload || args.present(u"log-flute-packets");
    args.getIntValue(max_file_size, u"max-file-size");
    return true;
}
