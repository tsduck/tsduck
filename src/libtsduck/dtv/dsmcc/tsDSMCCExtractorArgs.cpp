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
                                   u"When omitted, the extractor runs in list-only mode: "
                                   u"prints the carousel tree, group/module table and "
                                   u"per-module DII descriptors without writing any files.");

    args.option(u"dump-modules");
    args.help(u"dump-modules", u"Also write raw assembled module payloads to "
                               u"<output-directory>/modules/. Requires --output-directory. "
                               u"Mutually exclusive with --data-carousel.");

    args.option(u"data-carousel");
    args.help(u"data-carousel", u"Treat the PID as a plain data carousel (e.g. DVB-SSU) "
                                u"instead of an object carousel: skip BIOP parsing and "
                                u"write each completed module under "
                                u"<output-directory>/<download_id>/<label_or_module_XXXX>.bin, "
                                u"mirroring the carousel's group hierarchy on disk. The "
                                u"leaf file name is taken from the module's label_descriptor "
                                u"when present. Mutually exclusive with --dump-modules.");
}


bool ts::DSMCCExtractorArgs::loadArgs(Args& args)
{
    args.getIntValue(pid, u"pid");
    args.getValue(options.out_dir, u"output-directory");
    options.list_mode = options.out_dir.empty();
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
        args.error(u"--dump-modules requires --output-directory");
        return false;
    }
    return true;
}
