//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport Stream analysis utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsTSAnalyzerReport.h"
#include "tsTSAnalyzerOptions.h"
#include "tsTSFile.h"
#include "tsPagerArgs.h"
#include "tsDuckContext.h"
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

        ts::DuckContext       duck {this};         // TSDuck execution context.
        ts::BitRate           bitrate = 0;         // Expected bitrate (188-byte packets)
        fs::path              infile {};           // Input file name
        ts::TSPacketFormat    format = ts::TSPacketFormat::AUTODETECT; // Input file format.
        ts::TSAnalyzerOptions analysis {};         // Analysis options.
        ts::PagerArgs         pager {true, true};  // Output paging options.
    };
}

Options::Options(int argc, char *argv[]) :
    ts::Args(u"Analyze the structure of a transport stream", u"[options] [filename]")
{
    // Define all standard analysis options.
    duck.defineArgsForStandards(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForCharset(*this);
    duck.defineArgsForPDS(*this);
    pager.defineArgs(*this);
    analysis.defineArgs(*this);
    ts::DefineTSPacketFormatInputOption(*this);

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"Input transport stream file (standard input if omitted).");

    option<ts::BitRate>(u"bitrate", 'b');
    help(u"bitrate",
         u"Specifies the bitrate of the transport stream in bits/second "
         u"(based on 188-byte packets). By default, the bitrate is "
         u"evaluated using the PCR in the transport stream.");

    analyze(argc, argv);

    // Define all standard analysis options.
    duck.loadArgs(*this);
    pager.loadArgs(*this);
    analysis.loadArgs(duck, *this);

    getPathValue(infile, u"");
    getValue(bitrate, u"bitrate");
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

    // Configure the TS analyzer.
    ts::TSAnalyzerReport analyzer(opt.duck, opt.bitrate, ts::BitRateConfidence::OVERRIDE);
    analyzer.setAnalysisOptions(opt.analysis);

    // Open the TS file.
    ts::TSFile file;
    if (!file.openRead(opt.infile, 1, 0, opt, opt.format)) {
        return EXIT_FAILURE;
    }

    // Analyze all packets in the file.
    ts::TSPacket pkt;
    ts::TSPacketMetadata mdata;
    while (file.readPackets(&pkt, &mdata, 1, opt) > 0) {
        analyzer.feedPacket(pkt, mdata);
    }
    file.close(opt);

    // Display analysis results.
    analyzer.report(opt.pager.output(opt), opt.analysis, opt);

    return EXIT_SUCCESS;
}
