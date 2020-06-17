//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//  Collect selected PSI/SI tables from a transport stream.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTSFile.h"
#include "tsTablesLogger.h"
#include "tsPagerArgs.h"
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

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::DuckContext    duck;     // TSDuck execution context.
        ts::TablesDisplay  display;  // Table formatting.
        ts::TablesLogger   logger;   // Table logging.
        ts::PagerArgs      pager;    // Output paging options.
        ts::UString        infile;   // Input file name.
        ts::TSPacketFormat format;   // Input file format.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Collect PSI/SI tables from an MPEG transport stream", u"[options] [filename]"),
    duck(this),
    display(duck),
    logger(display),
    pager(true, true),
    infile(),
    format(ts::TSPacketFormat::AUTODETECT)
{
    duck.defineArgsForCAS(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForStandards(*this);
    duck.defineArgsForCharset(*this);
    pager.defineArgs(*this);
    logger.defineArgs(*this);
    display.defineArgs(*this);

    option(u"", 0, STRING, 0, 1);
    help(u"", u"Input transport stream file (standard input if omitted).");

    option(u"format", 0, ts::TSPacketFormatEnum);
    help(u"format", u"name",
         u"Specify the format of the input file. "
         u"By default, the format is automatically detected. "
         u"But the auto-detection may fail in some cases "
         u"(for instance when the first time-stamp of an M2TS file starts with 0x47). "
         u"Using this option forces a specific format.");

    analyze(argc, argv);

    duck.loadArgs(*this);
    pager.loadArgs(duck, *this);
    logger.loadArgs(duck, *this);
    display.loadArgs(duck, *this);

    infile = value(u"");
    format = enumValue<ts::TSPacketFormat>(u"format", ts::TSPacketFormat::AUTODETECT);

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
