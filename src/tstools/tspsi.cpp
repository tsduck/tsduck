//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Display PSI/SI information from a transport stream
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTSFile.h"
#include "tsPagerArgs.h"
#include "tsTablesDisplay.h"
#include "tsPSILogger.h"
#include "tsTSPacket.h"
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
        ts::TablesDisplay  display {duck};     // Table formatting options.
        ts::PSILogger      logger {display};   // Table logging options
        ts::PagerArgs      pager {true, true}; // Output paging options.
        ts::UString        infile {};          // Input file name.
        ts::TSPacketFormat format = ts::TSPacketFormat::AUTODETECT; // Input file format.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Extract all standard PSI from an MPEG transport stream", u"[options] [filename]")
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
    help(u"", u"Input MPEG capture file (standard input if omitted).");

    analyze(argc, argv);

    duck.loadArgs(*this);
    pager.loadArgs(*this);
    logger.loadArgs(duck, *this);
    display.loadArgs(duck, *this);

    getValue(infile, u"");
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

    // Open the TS file.
    ts::TSFile file;
    if (!file.openRead(opt.infile, 1, 0, opt, opt.format)) {
        return EXIT_FAILURE;
    }

    // Read all packets in the file and pass them to the logger
    ts::TSPacket pkt;
    if (!opt.logger.open()) {
        return EXIT_FAILURE;
    }
    while (!opt.logger.completed() && file.readPackets(&pkt, nullptr, 1, opt) > 0) {
        opt.logger.feedPacket(pkt);
    }
    file.close(opt);
    opt.logger.close();

    // Report errors
    if (opt.verbose()) {
        opt.logger.reportDemuxErrors();
    }
    return EXIT_SUCCESS;
}
