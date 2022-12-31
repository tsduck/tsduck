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

        ts::DuckContext       duck;      // TSDuck execution context.
        ts::BitRate           bitrate;   // Expected bitrate (188-byte packets)
        ts::UString           infile;    // Input file name
        ts::TSPacketFormat    format;    // Input file format.
        ts::TSAnalyzerOptions analysis;  // Analysis options.
        ts::PagerArgs         pager;     // Output paging options.
    };
}

Options::Options(int argc, char *argv[]) :
    ts::Args(u"Analyze the structure of a transport stream", u"[options] [filename]"),
    duck(this),
    bitrate(0),
    infile(),
    format(ts::TSPacketFormat::AUTODETECT),
    analysis(),
    pager(true, true)
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
    pager.loadArgs(duck, *this);
    analysis.loadArgs(duck, *this);

    getValue(infile, u"");
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
    while (file.readPackets(&pkt, nullptr, 1, opt) > 0) {
        analyzer.feedPacket(pkt);
    }
    file.close(opt);

    // Display analysis results.
    analyzer.report(opt.pager.output(opt), opt.analysis, opt);

    return EXIT_SUCCESS;
}
