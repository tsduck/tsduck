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
//  Transport Stream analysis utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsTSAnalyzerReport.h"
#include "tsTSAnalyzerOptions.h"
#include "tsInputRedirector.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    ts::BitRate           bitrate;   // Expected bitrate (188-byte packets)
    ts::UString           infile;    // Input file name
    ts::TSAnalyzerOptions analysis;  // Analysis options.
};

Options::Options(int argc, char *argv[]) :
    ts::Args(u"Analyze the structure of a transport stream", u"[options] [filename]"),
    bitrate(0),
    infile(),
    analysis()
{
    // Define all standard analysis options.
    analysis.defineOptions(*this);

    option(u"", 0, STRING, 0, 1);
    help(u"", u"Input MPEG capture file (standard input if omitted).");

    option(u"bitrate", 'b', UNSIGNED);
    help(u"bitrate",
         u"Specifies the bitrate of the transport stream in bits/second "
         u"(based on 188-byte packets). By default, the bitrate is "
         u"evaluated using the PCR in the transport stream.");

    analyze(argc, argv);

    infile = value(u"");
    bitrate = intValue<ts::BitRate>(u"bitrate");
    analysis.load(*this);

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);
    ts::TSAnalyzerReport analyzer(opt.bitrate);
    ts::InputRedirector input(opt.infile, opt);
    ts::TSPacket pkt;

    analyzer.setAnalysisOptions(opt.analysis);

    while (pkt.read(std::cin, true, opt)) {
        analyzer.feedPacket(pkt);
    }

    analyzer.report(std::cout, opt.analysis);

    return EXIT_SUCCESS;
}

TS_MAIN(MainCode)
