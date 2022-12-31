//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  TS multiplexer.
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

        bool                monitor;         // Run a resource monitoring thread in the background.
        ts::UString         monitor_config;  // System monitoring configuration file.
        ts::DuckContext     duck;            // TSDuck context
        ts::AsyncReportArgs log_args;        // Asynchronous logger arguments.
        ts::MuxerArgs       mux_args;        // TS multiplexer arguments.
    };
}

TSMuxOptions::TSMuxOptions(int argc, char *argv[]) :
    ts::ArgsWithPlugins(0, UNLIMITED_COUNT, 0, 0, 0, 1, u"TS multiplexer", u"[tsmux-options]"),
    monitor(false),
    monitor_config(),
    duck(this),
    log_args(),
    mux_args()
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
    log_args.loadArgs(duck, *this);
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
    ts::PluginRepository::Instance()->setSharedLibraryAllowed(false);
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
