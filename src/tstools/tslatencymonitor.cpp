//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Amos Cheung
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TS Latency Monitor based on input plugins.
//
//  Implementation notes:
//
//  The class tsLatencyMonitorCore implements the core function of tslatencymonitor.
//  It is used by all other classes to get their instructions and report their status.
//
//  Each instance of the class InputExecutor implements a thread running one
//  input plugin.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsArgsWithPlugins.h"
#include "tsLatencyMonitor.h"
#include "tsAsyncReport.h"

TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::ArgsWithPlugins
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::DuckContext         duck {this};             // TSDuck context
        ts::AsyncReportArgs     log_args {};             // Asynchronous logger arguments.
        ts::LatencyMonitorArgs  latency_monitor_args {}; // TS processing arguments.
    };
}

Options::Options(int argc, char *argv[]) :
    ts::ArgsWithPlugins(2, 2, 0, 0, 0, 0, u"Monitor latency between two TS input sources", u"[options]")
{
    log_args.defineArgs(*this);
    latency_monitor_args.defineArgs(*this);

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    log_args.loadArgs(*this);
    latency_monitor_args.loadArgs(*this);

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
//  Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    Options opt(argc, argv);

    // Create and start an asynchronous log (separate thread).
    ts::AsyncReport report(opt.maxSeverity(), opt.log_args);

    // The TS input processing is performed into this object.
    ts::LatencyMonitor core(opt.latency_monitor_args, report);

    return core.start() ? EXIT_SUCCESS : EXIT_FAILURE;
}
