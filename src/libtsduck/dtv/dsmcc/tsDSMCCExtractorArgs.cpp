//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCExtractorArgs.h"


void ts::DSMCCExtractorArgs::defineArgs(Args& args)
{
    args.option(u"pid", 'p', Args::PIDVAL);
    args.help(u"pid", u"PID carrying the DSM-CC carousel (DSI/DII/DDB sections). Required.");

    args.option(u"output-directory", 'o', Args::STRING);
    args.help(u"output-directory", u"Directory where carousel files will be extracted. "
                                   u"Required unless --list is set.");

    args.option(u"list", 'l');
    args.help(u"list", u"List-only mode: print the carousel tree, module table and statistics "
                       u"without writing any files. --output-directory is not required.");

    args.option(u"dump-modules");
    args.help(u"dump-modules", u"Also write raw assembled module payloads to "
                               u"<output-directory>/modules/. Mutually exclusive "
                               u"with --list and with --data-carousel.");

    args.option(u"data-carousel");
    args.help(u"data-carousel", u"Treat the PID as a plain data carousel (e.g. DVB-SSU) "
                                u"instead of an object carousel: skip BIOP parsing and "
                                u"write each completed module directly as "
                                u"<output-directory>/module_XXXX.bin. Mutually exclusive "
                                u"with --dump-modules.");
}


bool ts::DSMCCExtractorArgs::loadArgs(Args& args)
{
    args.getIntValue(pid, u"pid");
    args.getValue(options.out_dir, u"output-directory");
    options.list_mode = args.present(u"list");
    options.dump_modules = args.present(u"dump-modules");
    options.data_carousel = args.present(u"data-carousel");

    if (pid == PID_NULL) {
        args.error(u"a PID must be specified using --pid");
        return false;
    }
    if (options.data_carousel && options.dump_modules) {
        args.error(u"--data-carousel and --dump-modules are mutually exclusive");
        return false;
    }
    if (options.list_mode && options.dump_modules) {
        args.error(u"--list and --dump-modules are mutually exclusive");
        return false;
    }
    if (!options.list_mode && options.out_dir.empty()) {
        args.error(u"an output directory must be specified with --output-directory (or use --list)");
        return false;
    }
    return true;
}
