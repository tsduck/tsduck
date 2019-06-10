//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
#include "tsDuckContext.h"
#include "tsInputRedirector.h"
#include "tsPagerArgs.h"
#include "tsPSILogger.h"
TSDUCK_SOURCE;
TS_MAIN(MainCode);

// With static link, enforce a reference to MPEG/DVB structures.
#if defined(TSDUCK_STATIC_LIBRARY)
#include "tsStaticReferencesDVB.h"
const ts::StaticReferencesDVB dependenciesForStaticLib;
#endif


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

class Options: public ts::Args
{
public:
    Options(int argc, char *argv[]);
    virtual ~Options();

    ts::DuckContext       duck;     // TSDuck execution context.
    ts::UString           infile;   // Input file name
    ts::PSILoggerArgs     logger;   // Table logging options
    ts::TablesDisplayArgs display;  // Table formatting options.
    ts::PagerArgs         pager;    // Output paging options.

private:
    // Inaccessible operations.
    Options() = delete;
    Options(const Options&) = delete;
    Options(Options&&) = delete;
    Options& operator=(const Options&) = delete;
};

// Destructor.
Options::~Options() {}

// Constructor.
Options::Options(int argc, char *argv[]) :
    Args(u"Extract all standard PSI from an MPEG transport stream", u"[options] [filename]"),
    duck(this),
    infile(),
    logger(),
    display(duck),
    pager(true, true)
{
    duck.defineOptionsForPDS(*this);
    duck.defineOptionsForStandards(*this);
    duck.defineOptionsForDVBCharset(*this);
    pager.defineOptions(*this);
    logger.defineOptions(*this);
    display.defineOptions(*this);

    option(u"", 0, STRING, 0, 1);
    help(u"", u"Input MPEG capture file (standard input if omitted).");

    analyze(argc, argv);

    duck.loadOptions(*this);
    pager.load(*this);
    logger.load(*this);
    display.load(*this);

    infile = value(u"");

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);
    ts::InputRedirector input(opt.infile, opt);
    ts::TablesDisplay display(opt.display);
    ts::PSILogger logger(opt.logger, display);
    ts::TSPacket pkt;

    // Redirect display on pager process or stdout only.
    opt.duck.setOutput(&opt.pager.output(opt), false);

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
