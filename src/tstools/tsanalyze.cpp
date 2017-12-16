//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

#include "tsTSAnalyzerReport.h"
#include "tsTSAnalyzerOptions.h"
#include "tsInputRedirector.h"
#include "tsVersionInfo.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::TSAnalyzerOptions
{
    Options(int argc, char *argv[]);

    ts::BitRate bitrate;  // Expected bitrate (188-byte packets)
    ts::UString infile;   // Input file name
};

Options::Options(int argc, char *argv[]) :
    ts::TSAnalyzerOptions(u"MPEG Transport Stream Analysis Utility.", u"[options] [filename]"),
    bitrate(0),
    infile()
{
    option(u"",         0,  Args::STRING, 0, 1);
    option(u"bitrate", 'b', Args::UNSIGNED);

    setHelp(u"Input file:\n"
            u"\n"
            u"  MPEG capture file (standard input if omitted).\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -b value\n"
            u"  --bitrate value\n"
            u"      Specifies the bitrate of the transport stream in bits/second\n"
            u"      (based on 188-byte packets). By default, the bitrate is\n"
            u"      evaluated using the PCR in the transport stream.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -v\n"
            u"  --verbose\n"
            u"      Produce verbose output.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    analyze(argc, argv);

    infile = value(u"");
    bitrate = intValue<ts::BitRate>(u"bitrate");

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    TSDuckLibCheckVersion();
    Options opt(argc, argv);
    ts::TSAnalyzerReport analyzer(opt.bitrate);
    ts::InputRedirector input(opt.infile, opt);
    ts::TSPacket pkt;

    analyzer.setAnalysisOptions(opt);

    while (pkt.read(std::cin, true, opt)) {
        analyzer.feedPacket(pkt);
    }

    analyzer.report(std::cout, opt);

    return EXIT_SUCCESS;
}
