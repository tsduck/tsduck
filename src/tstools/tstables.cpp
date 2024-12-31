//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Collect selected PSI/SI tables from a transport stream.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTSFile.h"
#include "tsTablesDisplay.h"
#include "tsTablesLogger.h"
#include "tsPagerArgs.h"
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

        ts::DuckContext    duck {this};        // TSDuck execution context.
        ts::TablesDisplay  display {duck};     // Table formatting.
        ts::TablesLogger   logger {display};   // Table logging.
        ts::PagerArgs      pager {true, true}; // Output paging options.
        fs::path           infile {};          // Input file name.
        ts::TSPacketFormat format = ts::TSPacketFormat::AUTODETECT;
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Collect PSI/SI tables from an MPEG transport stream", u"[options] [filename]")
{
    duck.defineArgsForCAS(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForStandards(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForCharset(*this);
    pager.defineArgs(*this);
    logger.defineArgs(*this);
    display.defineArgs(*this);
    ts::DefineTSPacketFormatInputOption(*this);

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"Input transport stream file (standard input if omitted).");

    analyze(argc, argv);

    duck.loadArgs(*this);
    pager.loadArgs(duck, *this);
    logger.loadArgs(duck, *this);
    display.loadArgs(duck, *this);

    getPathValue(infile, u"");
    format = ts::LoadTSPacketFormatInputOption(*this);

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Decode command line options.
    Options opt(argc, argv);

    // Redirect display on pager process or stdout only.
    opt.duck.setOutput(&opt.pager.output(opt), false);

    // Open section logger.
    if (!opt.logger.open()) {
        return EXIT_FAILURE;
    }

    // Open the TS file.
    ts::TSFile file;
    if (!file.openRead(opt.infile, 1, 0, opt, opt.format)) {
        return EXIT_FAILURE;
    }

    // Read all packets in the file and pass them to the logger
    ts::TSPacket pkt;
    while (!opt.logger.completed() && file.readPackets(&pkt, nullptr, 1, opt) > 0) {
        opt.logger.feedPacket(pkt);
    }
    file.close(opt);
    opt.logger.close();

    // Report errors
    if (opt.verbose() && !opt.logger.hasErrors()) {
        opt.logger.reportDemuxErrors(std::cerr);
    }

    return opt.logger.hasErrors() ? EXIT_FAILURE : EXIT_SUCCESS;
}
