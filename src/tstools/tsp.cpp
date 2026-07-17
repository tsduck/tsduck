//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
        bool                dot = false;         // Output the plugin chain in Graphviz DOT format.
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

    option(u"dot");
    help(u"dot",
         u"Output the top-level plugin chain in Graphviz DOT format and exit. "
         u"Plugin options are not included.");

    option(u"monitor", 'm', STRING, 0, 1, 0, UNLIMITED_VALUE, true);
    help(u"monitor", u"filename",
         u"Continuously monitor the system resources which are used by tsp. "
         u"This includes CPU load, virtual memory usage. "
         u"Useful to verify the stability of the application. "
         u"The optional file is an XML monitoring configuration file.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    dot = present(u"dot");
    monitor = present(u"monitor");
    getValue(monitor_config, u"monitor");
    duck.loadArgs(*this);
    log_args.loadArgs(*this);
    tsp_args.loadArgs(duck, *this);

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
//  Output the top-level plugin chain in Graphviz DOT format.
//----------------------------------------------------------------------------

namespace {
    void OutputGraph(const ts::TSProcessorArgs& args)
    {
        std::cout << "digraph tsp {" << std::endl
                  << "    rankdir = LR;" << std::endl
                  << "    node [shape = box];" << std::endl
                  << "    input [label = \"Input\\n" << args.input.name.toJSON() << "\"];" << std::endl;

        ts::UString previous(u"input");
        for (size_t index = 0; index < args.plugins.size(); ++index) {
            const ts::UString current(ts::UString::Format(u"processor_%d", index));
            std::cout << "    " << current << " [label = \"Processor " << index << "\\n"
                      << args.plugins[index].name.toJSON() << "\"];" << std::endl
                      << "    " << previous << " -> " << current << ";" << std::endl;
            previous = current;
        }

        std::cout << "    output [label = \"Output\\n" << args.output.name.toJSON() << "\"];" << std::endl
                  << "    " << previous << " -> output;" << std::endl
                  << "}" << std::endl;
    }
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

    // Describing the chain does not load plugins or process transport streams.
    if (opt.dot) {
        OutputGraph(opt.tsp_args);
        return EXIT_SUCCESS;
    }

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
    report.debug(u"main(): wait for TSProcessor termination");
    tsproc.waitForTermination();
    report.debug(u"main(): exit application");
    return EXIT_SUCCESS;
}
