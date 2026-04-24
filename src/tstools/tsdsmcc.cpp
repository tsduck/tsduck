//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  DSM-CC carousel extraction tool.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTSFile.h"
#include "tsDSMCCExtractor.h"
#include "tsDSMCCExtractorArgs.h"
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

        ts::DuckContext          duck {this};   //!< TSDuck execution context.
        fs::path                 infile {};     //!< Input file name (empty => stdin).
        ts::TSPacketFormat       format = ts::TSPacketFormat::AUTODETECT;
        ts::DSMCCExtractorArgs   ext {};
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Extract DSM-CC carousel content from an MPEG transport stream", u"[options] [filename]")
{
    ts::DefineTSPacketFormatInputOption(*this);

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"Input transport stream file (standard input if omitted).");

    ext.defineArgs(*this);

    analyze(argc, argv);

    duck.loadArgs(*this);
    getPathValue(infile, u"");
    format = ts::LoadTSPacketFormatInputOption(*this);
    ext.loadArgs(*this);

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);

    ts::DSMCCExtractor extractor(opt.duck, opt.ext.options);
    extractor.setPID(opt.ext.pid);

    ts::TSFile file;
    if (!file.openRead(opt.infile, 1, 0, opt, opt.format)) {
        return EXIT_FAILURE;
    }

    ts::TSPacket pkt;
    while (file.readPackets(&pkt, nullptr, 1, opt) > 0) {
        extractor.feedPacket(pkt);
    }
    file.close(opt);

    extractor.flush();

    return opt.gotErrors() ? EXIT_FAILURE : EXIT_SUCCESS;
}
