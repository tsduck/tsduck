//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIPAnalyzerArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::NIPAnalyzerArgs::defineArgs(Args& args)
{
    args.option(u"dump-flute-payload");
    args.help(u"dump-flute-payload",
              u"Same as --log-flute-packets and also dump the payload of each FLUTE packet.");

    args.option(u"log-fdt");
    args.help(u"log-fdt",
              u"Log a message describing each FLUTE File Delivery Table (FDT).");

    args.option(u"log-flute-packets");
    args.help(u"log-flute-packets",
              u"Log a message describing the structure of each FLUTE packet.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::NIPAnalyzerArgs::loadArgs(DuckContext& duck, Args& args)
{
    dump_flute_payload = args.present(u"dump-flute-payload");
    log_flute_packets = dump_flute_payload || args.present(u"log-flute-packets");
    log_fdt = args.present(u"log-fdt");

    return true;
}
