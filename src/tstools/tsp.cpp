//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsSystemMonitor.h"
#include "tsVersionInfo.h"
TS_MAIN(MainCode);


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
        bool                monitor = false;     // Run a resource monitoring thread in the background.
        ts::UString         monitor_config {};   // System monitoring configuration file.S
        ts::DuckContext     duck {this};         // TSDuck context
        ts::AsyncReportArgs log_args {};         // Asynchronous logger arguments.
        ts::TSProcessorArgs tsp_args {};         // TS processing arguments.
    };
}

TSPOptions::TSPOptions(int argc, char *argv[]) :
    ts::ArgsWithPlugins(0, 1, 0, UNLIMITED_COUNT, 0, 1, u"MPEG transport stream processor using a chain of plugins", u"[tsp-options]")
{
    duck.defineArgsForCAS(*this);
    duck.defineArgsForCharset(*this);
    duck.defineArgsForHFBand(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForFixingPDS(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForStandards(*this);
    log_args.defineArgs(*this);
    tsp_args.defineArgs(*this);

    option(u"monitor", 'm', STRING, 0, 1, 0, UNLIMITED_VALUE, true);
    help(u"monitor", u"filename",
         u"Continuously monitor the system resources which are used by tsp. "
         u"This includes CPU load, virtual memory usage. "
         u"Useful to verify the stability of the application. "
         u"The optional file is an XML monitoring configuration file.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    monitor = present(u"monitor");
    getValue(monitor_config, u"monitor");
    duck.loadArgs(*this);
    log_args.loadArgs(*this);
    tsp_args.loadArgs(duck, *this);

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
//  Interrupt handler
//----------------------------------------------------------------------------

class TSPInterruptHandler: public ts::InterruptHandler
{
    TS_NOBUILD_NOCOPY(TSPInterruptHandler);
public:
    TSPInterruptHandler(ts::AsyncReport* report, ts::TSProcessor* tsproc);
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

    // If plugins were statically linked, disallow the dynamic loading of plugins.
#if defined(TSDUCK_STATIC_PLUGINS)
    ts::PluginRepository::Instance().setSharedLibraryAllowed(false);
#endif

    // Get command line options.
    TSPOptions opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());

    // Prevent from being killed when writing on broken pipes.
    ts::IgnorePipeSignal();

    // Create an asynchronous error logger. Can be used in multi-threaded context.
    ts::AsyncReport report(opt.maxSeverity(), opt.log_args);

    // System monitor thread.
    ts::SystemMonitor monitor(report, opt.monitor_config);

    // The TS processing is performed into this object.
    ts::TSProcessor tsproc(report);

    // Use a Ctrl+C interrupt handler
    TSPInterruptHandler interrupt_handler(&report, &tsproc);
    ts::UserInterrupt interrupt_manager(&interrupt_handler, true, true);

    // Start the monitoring thread if required.
    if (opt.monitor) {
        monitor.start();
    }

    // Start the TS processing.
    if (!tsproc.start(opt.tsp_args)) {
        return EXIT_FAILURE;
    }

    // Start checking for new TSDuck version in the background.
    ts::VersionInfo version_check(report);
    version_check.startNewVersionDetection();

    // And wait for TS processing termination.
    tsproc.waitForTermination();
    return EXIT_SUCCESS;
}
