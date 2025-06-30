//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TS input switch based on input plugins.
//
//  Implementation notes:
//
//  The class Core implements the core function of tsswitch. It is used
//  by all other classes to get their instructions and report their status.
//
//  Each instance of the class InputExecutor implements a thread running one
//  input plugin.
//
//  The class OutputExecutor implements the thread running the single output
//  plugin. When started, it simply waits for packets from the current input
//  plugin and outputs them. The output threads stops when instructed by the
//  Switch object or in case of output error. In case of error, the output
//  threads sends a global stop command to the Switch object.
//
//  If the option --remote is used, an instance of the class CommandListener
//  starts a thread which listens to UDP commands. The received commands are
//  sent to the Switch object.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsArgsWithPlugins.h"
#include "tsInputSwitcher.h"
#include "tsPluginRepository.h"
#include "tsSystemMonitor.h"
#include "tsAsyncReport.h"
#include "tsCerrReport.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class TSSwitchOptions: public ts::ArgsWithPlugins
    {
        TS_NOBUILD_NOCOPY(TSSwitchOptions);
    public:
        TSSwitchOptions(int argc, char *argv[]);

        ts::DuckContext       duck {this};        // TSDuck context
        bool                  monitor = false;    // Run a resource monitoring thread in the background.
        ts::UString           monitor_config {};  // System monitoring configuration file.
        ts::AsyncReportArgs   log_args {};        // Asynchronous logger arguments.
        ts::InputSwitcherArgs switch_args {};     // TS processing arguments.
    };
}

TSSwitchOptions::TSSwitchOptions(int argc, char *argv[]) :
    ts::ArgsWithPlugins(0, UNLIMITED_COUNT, 0, 0, 0, 1, u"TS input source switch using remote control", u"[tsswitch-options]")
{
    log_args.defineArgs(*this);
    switch_args.defineArgs(*this);

    option(u"monitor", 'm', STRING, 0, 1, 0, UNLIMITED_VALUE, true);
    help(u"monitor", u"filename",
         u"Continuously monitor the system resources which are used by tsswitch. "
         u"This includes CPU load, virtual memory usage. "
         u"Useful to verify the stability of the application. "
         u"The optional file is an XML monitoring configuration file.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    monitor = present(u"monitor");
    getValue(monitor_config, u"monitor");
    log_args.loadArgs(*this);
    switch_args.loadArgs(duck, *this);

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
//  Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    TSSwitchOptions opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());

    // If plugins were statically linked, disallow the dynamic loading of plugins.
#if defined(TSDUCK_STATIC_PLUGINS)
    ts::PluginRepository::Instance().setSharedLibraryAllowed(false);
#endif

    // Create and start an asynchronous log (separate thread).
    ts::AsyncReport report(opt.maxSeverity(), opt.log_args);

    // System monitor thread.
    ts::SystemMonitor monitor(report, opt.monitor_config);

    // Start the monitoring thread if required.
    if (opt.monitor) {
        monitor.start();
    }

    // The TS input processing is performed into this object.
    ts::InputSwitcher switcher(opt.switch_args, report);

    return switcher.success() ? EXIT_SUCCESS : EXIT_FAILURE;
}
