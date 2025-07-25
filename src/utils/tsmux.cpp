//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
// TS multiplexer (experimental program, not part of TSDuck).
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsMuxer.h"
#include "tsMuxerArgs.h"
#include "tsArgsWithPlugins.h"
#include "tsPluginRepository.h"
#include "tsSystemMonitor.h"
#include "tsAsyncReport.h"
#include "tsCerrReport.h"
#include "tsVersionInfo.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class TSMuxOptions: public ts::ArgsWithPlugins
    {
        TS_NOBUILD_NOCOPY(TSMuxOptions);
    public:
        TSMuxOptions(int argc, char *argv[]);

        bool                monitor = false;    // Run a resource monitoring thread in the background.
        ts::UString         monitor_config {};  // System monitoring configuration file.
        ts::DuckContext     duck {this};        // TSDuck context
        ts::AsyncReportArgs log_args {};        // Asynchronous logger arguments.
        ts::MuxerArgs       mux_args {};        // TS multiplexer arguments.
    };
}

TSMuxOptions::TSMuxOptions(int argc, char *argv[]) :
    ts::ArgsWithPlugins(0, UNLIMITED_COUNT, 0, 0, 0, 1, u"TS multiplexer", u"[tsmux-options]")
{
    log_args.defineArgs(*this);
    mux_args.defineArgs(*this);

    option(u"monitor", 'm', STRING, 0, 1, 0, UNLIMITED_VALUE, true);
    help(u"monitor", u"filename",
         u"Continuously monitor the system resources which are used by tsmux. "
         u"This includes CPU load, virtual memory usage. "
         u"Useful to verify the stability of the application. "
         u"The optional file is an XML monitoring configuration file.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    monitor = present(u"monitor");
    getValue(monitor_config, u"monitor");
    log_args.loadArgs(*this);
    mux_args.loadArgs(duck, *this);

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
//  Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    TSMuxOptions opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());

    // If plugins were statically linked, disallow the dynamic loading of plugins.
#if defined(TSDUCK_STATIC_PLUGINS)
    ts::PluginRepository::Instance().setSharedLibraryAllowed(false);
#endif

    // Prevent from being killed when writing on broken pipes.
    ts::IgnorePipeSignal();

    // Create and start an asynchronous log (separate thread).
    ts::AsyncReport report(opt.maxSeverity(), opt.log_args);

    // System monitor thread.
    ts::SystemMonitor monitor(report, opt.monitor_config);

    // The mux is performed into this object.
    ts::Muxer mux(report);

    // Start the monitoring thread if required.
    if (opt.monitor) {
        monitor.start();
    }

    // Start the mux.
    if (!mux.start(opt.mux_args)) {
        return EXIT_FAILURE;
    }

    // Start checking for new TSDuck version in the background.
    ts::VersionInfo version_check(report);
    version_check.startNewVersionDetection();

    // And wait for mux termination.
    mux.waitForTermination();
    return EXIT_SUCCESS;
}
