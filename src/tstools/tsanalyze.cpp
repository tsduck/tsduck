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

using namespace ts;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public TSAnalyzerOptions
{
    Options (int argc, char *argv[]);

    BitRate     bitrate;     // Expected bitrate (188-byte packets)
    std::string infile;      // Input file name
};

Options::Options (int argc, char *argv[]) :
    TSAnalyzerOptions ("MPEG Transport Stream Analysis Utility.", "[options] [filename]")
{
    option ("",         0,  Args::STRING, 0, 1);
    option ("bitrate", 'b', Args::UNSIGNED);

    setHelp ("Input file:\n"
             "\n"
             "  MPEG capture file (standard input if omitted).\n"
             "\n"
             "Options:\n"
             "\n"
             "  -b value\n"
             "  --bitrate value\n"
             "      Specifies the bitrate of the transport stream in bits/second\n"
             "      (based on 188-byte packets). By default, the bitrate is\n"
             "      evaluated using the PCR in the transport stream.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");

    analyze (argc, argv);

    infile = value ("");
    bitrate = intValue<BitRate> ("bitrate");
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);
    TSAnalyzerReport analyzer (opt.bitrate);
    InputRedirector input (opt.infile, opt);
    TSPacket pkt;

    analyzer.setAnalysisOptions (opt);

    while (pkt.read (std::cin, true, opt)) {
        analyzer.feedPacket (pkt);
    }

    analyzer.report (std::cout, opt);

    return EXIT_SUCCESS;
}
