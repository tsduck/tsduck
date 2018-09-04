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
//  Display PSI/SI information from a transport stream
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsInputRedirector.h"
#include "tsPSILogger.h"
TSDUCK_SOURCE;

// With static link, enforce a reference to MPEG/DVB structures.
#if defined(TSDUCK_STATIC_LIBRARY)
#include "tsStaticReferencesDVB.h"
const ts::StaticReferencesDVB dependenciesForStaticLib;
#endif


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    ts::UString           infile;   // Input file name
    ts::PSILoggerArgs     logger;   // Table logging options
    ts::TablesDisplayArgs display;  // Table formatting options.
};

Options::Options(int argc, char *argv[]) :
    Args(u"Extract all standard PSI from an MPEG transport stream", u"[options] [filename]"),
    infile(),
    logger(),
    display()
{
    option(u"", 0, STRING, 0, 1);
    help(u"", u"Input MPEG capture file (standard input if omitted).");

    logger.defineOptions(*this);
    display.defineOptions(*this);

    analyze(argc, argv);

    infile = value(u"");
    logger.load(*this);
    display.load(*this);

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);
    ts::InputRedirector input(opt.infile, opt);
    ts::TablesDisplay display(opt.display, opt);
    ts::PSILogger logger(opt.logger, display, opt);
    ts::TSPacket pkt;

    // Read all packets in the file and pass them to the logger
    while (!logger.completed() && pkt.read(std::cin, true, opt)) {
        logger.feedPacket(pkt);
    }

    // Report errors
    if (opt.verbose()) {
        logger.reportDemuxErrors();
    }

    return EXIT_SUCCESS;
}

TSDuckMain(MainCode)
