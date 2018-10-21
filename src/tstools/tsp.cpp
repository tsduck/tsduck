//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tspOptions.h"
#include "tspInputExecutor.h"
#include "tspOutputExecutor.h"
#include "tspProcessorExecutor.h"
#include "tsPluginRepository.h"
#include "tsAsyncReport.h"
#include "tsSystemMonitor.h"
#include "tsMonotonic.h"
#include "tsResidentBuffer.h"
#include "tsOutputPager.h"
TSDUCK_SOURCE;

// With static link, enforce a reference to MPEG/DVB structures.
#if defined(TSDUCK_STATIC_LIBRARY)
#include "tsStaticReferencesDVB.h"
const ts::StaticReferencesDVB dependenciesForStaticLib;
#endif


//----------------------------------------------------------------------------
//  Interrupt handler
//----------------------------------------------------------------------------

namespace ts {
    namespace tsp {
        class TSPInterruptHandler: public InterruptHandler
        {
        public:
            TSPInterruptHandler(AsyncReport* report = nullptr, PluginExecutor* first_plugin = nullptr);
            virtual void handleInterrupt() override;
        private:
            AsyncReport*    _report;
            PluginExecutor* _first_plugin;

            // Inaccessible operations
            TSPInterruptHandler(const TSPInterruptHandler&) = delete;
            TSPInterruptHandler& operator=(const TSPInterruptHandler&) = delete;
        };
    }
}

ts::tsp::TSPInterruptHandler::TSPInterruptHandler(AsyncReport* report, PluginExecutor* first_plugin) :
    _report(report),
    _first_plugin(first_plugin)
{
}

void ts::tsp::TSPInterruptHandler::handleInterrupt()
{
    _report->info(u"tsp: user interrupt, terminating...");

    // Place all threads in "aborted" state so that each thread will see its
    // successor as aborted. Notify all threads that something happened.

    PluginExecutor* proc = _first_plugin;
    do {
        proc->setAbort();
    } while ((proc = proc->ringNext<PluginExecutor>()) != _first_plugin);
}


//----------------------------------------------------------------------------
//  Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Internal sanity check about TS packets.
    ts::TSPacket::SanityCheck();

    // Get command line options.
    ts::tsp::Options opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());
    assert(opt.inputs.size() == 1);
    assert(opt.outputs.size() == 1);

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

    // There is one global mutex for protected operations.
    // The resulting bottleneck of this single mutex is acceptable as long
    // as all protected operations are fast (pointer update, simple arithmetic).
    ts::Mutex global_mutex;

    // Load all plugins and analyze their command line arguments.
    // The first plugin is always the input and the last one is the output.
    // The input thread has the highest priority to be always ready to load
    // incoming packets in the buffer (avoid missing packets). The output
    // plugin has a hight priority to make room in the buffer, but not as
    // high as the input which must remain the top-most priority?

    ts::tsp::InputExecutor* input = new ts::tsp::InputExecutor(&opt, &opt.inputs.front(), ts::ThreadAttributes().setPriority(ts::ThreadAttributes::GetMaximumPriority()), global_mutex);
    ts::tsp::OutputExecutor* output = new ts::tsp::OutputExecutor(&opt, &opt.outputs.front(), ts::ThreadAttributes().setPriority(ts::ThreadAttributes::GetHighPriority()), global_mutex);
    output->ringInsertAfter(input);

    // Check if at least one plugin prefers real-time defaults.
    bool realtime = opt.realtime == ts::TRUE || input->isRealTime() || output->isRealTime();

    for (auto it = opt.plugins.begin(); it != opt.plugins.end(); ++it) {
        ts::tsp::PluginExecutor* p = new ts::tsp::ProcessorExecutor(&opt, &*it, ts::ThreadAttributes(), global_mutex);
        p->ringInsertBefore(output);
        realtime = realtime || p->isRealTime();
    }

    // Check if realtime defaults are explicitly disabled.
    if (opt.realtime == ts::FALSE) {
        realtime = false;
    }

    // Now, we definitely know if we are in offline or realtime mode.
    // Adjust some default parameters.
    opt.applyDefaults(realtime);

    // Exit on error when initializing the plugins
    opt.exitOnError();

    // Create an asynchronous error logger. Can be used in multi-threaded context.
    ts::AsyncReport report(opt.maxSeverity(), opt.timed_log, opt.log_msg_count, opt.sync_log);

    // Initialize all executors.
    ts::tsp::PluginExecutor* proc = input;
    do {
        // Set the asynchronous logger as report method for all executors.
        proc->setReport(&report);
        proc->setMaxSeverity(report.maxSeverity());
        // Also set realtime defaults.
        proc->setRealTimeForAll(realtime);
        // And decode command line parameters for the plugin.
        if (!proc->plugin()->getOptions()) {
            return EXIT_FAILURE;
        }
    } while ((proc = proc->ringNext<ts::tsp::PluginExecutor>()) != input);

    // Allocate a memory-resident buffer of TS packets
    ts::ResidentBuffer<ts::TSPacket> packet_buffer(opt.bufsize / ts::PKT_SIZE);
    if (!packet_buffer.isLocked()) {
        report.verbose(u"tsp: buffer failed to lock into physical memory (%d: %s), risk of real-time issue",
                       {packet_buffer.lockErrorCode(), ts::ErrorCodeMessage(packet_buffer.lockErrorCode())});
    }
    report.debug(u"tsp: buffer size: %'d TS packets, %'d bytes", {packet_buffer.count(), packet_buffer.count() * ts::PKT_SIZE});

    // Start all processors, except output, in reverse order (input last).
    // Exit application in case of error.
    for (proc = output->ringPrevious<ts::tsp::PluginExecutor>(); proc != output; proc = proc->ringPrevious<ts::tsp::PluginExecutor>()) {
        if (!proc->plugin()->start()) {
            return EXIT_FAILURE;
        }
    }

    // Initialize packet buffer in the ring of executors.
    // Exit application in case of error.
    if (!input->initAllBuffers(&packet_buffer)) {
        return EXIT_FAILURE;
    }

    // Start the output device (we now have an idea of the bitrate).
    // Exit application in case of error.
    assert(output != nullptr);
    assert(output->plugin() != nullptr);
    if (!output->plugin()->start()) {
        return EXIT_FAILURE;
    }

    // Use a Ctrl+C interrupt handler
    ts::tsp::TSPInterruptHandler interrupt_handler(&report, input);
    ts::UserInterrupt interrupt_manager(&interrupt_handler, true, true);

    // Create a monitoring thread if required.
    ts::SystemMonitor monitor(&report);
    if (opt.monitor) {
        monitor.start();
    }

    // Create all plugin executors threads.
    proc = input;
    do {
        proc->start();
    } while ((proc = proc->ringNext<ts::tsp::PluginExecutor>()) != input);

    // Wait for threads to terminate
    proc = input;
    do {
        proc->waitForTermination();
    } while ((proc = proc->ringNext<ts::tsp::PluginExecutor>()) != input);

    // Deallocate all plugins and plugin executor
    bool last;
    proc = input;
    do {
        last = proc->ringAlone();
        ts::tsp::PluginExecutor* next = proc->ringNext<ts::tsp::PluginExecutor>();
        proc->ringRemove();
        delete proc;
        proc = next;
    } while (!last);

    return EXIT_SUCCESS;
}

TS_MAIN(MainCode)
