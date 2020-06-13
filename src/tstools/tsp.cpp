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
//  Transport stream processor.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsTSProcessor.h"
#include "tsArgsWithPlugins.h"
#include "tsDuckContext.h"
#include "tsPluginRepository.h"
#include "tsAsyncReport.h"
#include "tsUserInterrupt.h"
#include "tsOutputPager.h"
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
    class TSPOptions: public ts::ArgsWithPlugins
    {
        TS_NOBUILD_NOCOPY(TSPOptions);
    public:
        TSPOptions(int argc, char *argv[]);

        // Option values
        ts::DuckContext     duck;             // TSDuck context
        int                 list_proc_flags;  // List processors, mask of PluginRepository::ListFlag.
        ts::AsyncReportArgs log_args;         // Asynchronous logger arguments.
        ts::TSProcessorArgs tsp_args;         // TS processing arguments.
    };
}

TSPOptions::TSPOptions(int argc, char *argv[]) :
    ts::ArgsWithPlugins(0, 1, 0, UNLIMITED_COUNT, 0, 1),
    duck(this),
    list_proc_flags(0),
    log_args(),
    tsp_args()
{
    setDescription(u"MPEG transport stream processor using a chain of plugins");

    setSyntax(u"[tsp-options] \\\n"
              u"    [-I input-name [input-options]] \\\n"
              u"    [-P processor-name [processor-options]] ... \\\n"
              u"    [-O output-name [output-options]]");

    duck.defineArgsForCAS(*this);
    duck.defineArgsForCharset(*this);
    duck.defineArgsForHFBand(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForStandards(*this);
    log_args.defineArgs(*this);
    tsp_args.defineArgs(*this);

    option(u"list-processors", 'l', ts::PluginRepository::ListProcessorEnum, 0, 1, true);
    help(u"list-processors", u"List all available processors.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    list_proc_flags = present(u"list-processors") ? intValue<int>(u"list-processors", ts::PluginRepository::LIST_ALL) : 0;
    duck.loadArgs(*this);
    log_args.loadArgs(duck, *this);
    tsp_args.loadArgs(duck, *this);

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
//  Interrupt handler
//----------------------------------------------------------------------------

class TSPInterruptHandler: public ts::InterruptHandler
{
    TS_NOCOPY(TSPInterruptHandler);
public:
    TSPInterruptHandler(ts::AsyncReport* report = nullptr, ts::TSProcessor* tsproc = nullptr);
    virtual void handleInterrupt() override;
private:
    ts::AsyncReport* _report;
    ts::TSProcessor* _tsproc;
};

TSPInterruptHandler::TSPInterruptHandler(ts::AsyncReport* report, ts::TSProcessor* tsproc) :
    _report(report),
    _tsproc(tsproc)
{
}

void TSPInterruptHandler::handleInterrupt()
{
    _report->info(u"tsp: user interrupt, terminating...");
    _tsproc->abort();
}


//----------------------------------------------------------------------------
//  Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Internal sanity check about TS packets.
    ts::TSPacket::SanityCheck();

    // Get command line options.
    TSPOptions opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());

    // Get the repository of plugins.
    ts::PluginRepository* plugins = ts::PluginRepository::Instance();
    ts::CheckNonNull(plugins);

    // If plugins were statically linked, disallow the dynamic loading of plugins.
#if defined(TSDUCK_STATIC_PLUGINS)
    plugins->setSharedLibraryAllowed(false);
#endif

    // Process the --list-processors option
    if (opt.list_proc_flags != 0) {
        // Build the list of plugins.
        const ts::UString text(plugins->listPlugins(true, opt, opt.list_proc_flags));
        // Try to page, raw output otherwise.
        ts::OutputPager pager;
        if ((opt.list_proc_flags & ts::PluginRepository::LIST_COMPACT) != 0) {
            // Compact output, no paging.
            std::cerr << text;
        }
        else if (pager.canPage() && pager.open(true, 0, opt)) {
            pager.write(text, opt);
            pager.write(u"\n", opt);
            pager.close(opt);
        }
        else {
            std::cerr << text << std::endl;
        }
        return EXIT_SUCCESS;
    }

    // Prevent from being killed when writing on broken pipes.
    ts::IgnorePipeSignal();

    // Create an asynchronous error logger. Can be used in multi-threaded context.
    ts::AsyncReport report(opt.maxSeverity(), opt.log_args);

    // The TS processing is performed into this object.
    ts::TSProcessor tsproc(report);

    // Use a Ctrl+C interrupt handler
    TSPInterruptHandler interrupt_handler(&report, &tsproc);
    ts::UserInterrupt interrupt_manager(&interrupt_handler, true, true);

    // Start the TS processing.
    if (!tsproc.start(opt.tsp_args)) {
        return EXIT_FAILURE;
    }

    // And wait for TS processing termination.
    tsproc.waitForTermination();
    return EXIT_SUCCESS;
}
