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

        ts::DuckContext            duck {this};   //!< TSDuck execution context.
        fs::path                   infile {};     //!< Input file name (empty => stdin).
        ts::TSPacketFormat         format = ts::TSPacketFormat::AUTODETECT;
        ts::PID                    pid = ts::PID_NULL;
        ts::DSMCCExtractor::Options ext {};
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Extract DSM-CC carousel content from an MPEG transport stream", u"[options] [filename]")
{
    ts::DefineTSPacketFormatInputOption(*this);

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"Input transport stream file (standard input if omitted).");

    option(u"pid", 'p', PIDVAL);
    help(u"pid", u"PID carrying the DSM-CC carousel (DSI/DII/DDB sections). Required.");

    option(u"output-directory", 'o', STRING);
    help(u"output-directory", u"Directory where carousel files will be extracted. "
                              u"Required unless --list is set.");

    option(u"list", 'l');
    help(u"list", u"List-only mode: print the carousel tree, module table and statistics "
                  u"without writing any files. --output-directory is not required.");

    option(u"dump-modules");
    help(u"dump-modules", u"Also write raw assembled module payloads to "
                          u"<output-directory>/modules/. Mutually exclusive "
                          u"with --list and with --data-carousel.");

    option(u"data-carousel");
    help(u"data-carousel", u"Treat the PID as a plain data carousel (e.g. DVB-SSU) "
                           u"instead of an object carousel: skip BIOP parsing and "
                           u"write each completed module directly as "
                           u"<output-directory>/module_XXXX.bin. Mutually exclusive "
                           u"with --dump-modules.");

    analyze(argc, argv);

    duck.loadArgs(*this);

    getPathValue(infile, u"");
    format = ts::LoadTSPacketFormatInputOption(*this);

    getIntValue(pid, u"pid", ts::PID_NULL);
    getValue(ext.out_dir, u"output-directory");
    ext.list_mode = present(u"list");
    ext.dump_modules = present(u"dump-modules");
    ext.data_carousel = present(u"data-carousel");

    if (pid == ts::PID_NULL) {
        error(u"a PID must be specified using --pid");
    }
    if (ext.data_carousel && ext.dump_modules) {
        error(u"--data-carousel and --dump-modules are mutually exclusive");
    }
    if (ext.list_mode && ext.dump_modules) {
        error(u"--list and --dump-modules are mutually exclusive");
    }
    if (!ext.list_mode && ext.out_dir.empty()) {
        error(u"an output directory must be specified with --output-directory (or use --list)");
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);

    ts::DSMCCExtractor extractor(opt.duck, opt.ext);
    extractor.setPID(opt.pid);

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
