//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Amos Cheung
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

        ts::DuckContext         duck;                 // TSDuck context
        ts::AsyncReportArgs     log_args;             // Asynchronous logger arguments.
        ts::LatencyMonitorArgs  latency_monitor_args; // TS processing arguments.
    };
}

Options::Options(int argc, char *argv[]) :
    ts::ArgsWithPlugins(2, 2, 0, 0, 0, 0, u"Monitor latency between two TS input sources", u"[options]"),
    duck(this),
    log_args(),
    latency_monitor_args()
{
    log_args.defineArgs(*this);
    latency_monitor_args.defineArgs(*this);

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    log_args.loadArgs(duck, *this);
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
