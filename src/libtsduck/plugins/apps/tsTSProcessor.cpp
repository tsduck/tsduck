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

#include "tsTSProcessor.h"
#include "tstspInputExecutor.h"
#include "tstspOutputExecutor.h"
#include "tstspProcessorExecutor.h"
#include "tstspControlServer.h"
#include "tsMonotonic.h"
#include "tsGuardMutex.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::TSProcessor::TSProcessor(Report& report) :
    PluginEventHandlerRegistry(),
    _report(report),
    _mutex(),
    _terminating(false),
    _args(),
    _input(nullptr),
    _output(nullptr),
    _control(nullptr),
    _packet_buffer(nullptr),
    _metadata_buffer(nullptr)
{
}

ts::TSProcessor::~TSProcessor()
{
    // Wait for processing termination to avoid other threads accessing a destroyed object.
    waitForTermination();
}


//----------------------------------------------------------------------------
// Deallocate and cleanup internal resources.
//----------------------------------------------------------------------------

void ts::TSProcessor::cleanupInternal()
{
    // Terminate and delete the control server.
    // This must be done first since the control server accesses the plugin executors.
    if (_control != nullptr) {
        // Deleting the object terminates the server thread.
        delete _control;
        _control = nullptr;
    }

    // Abort and wait for threads to terminate
    tsp::PluginExecutor* proc = _input;
    do {
        proc->setAbort();
        proc->waitForTermination();
    } while ((proc = proc->ringNext<tsp::PluginExecutor>()) != _input);

    // Deallocate all plugin executors.
    bool last = false;
    proc = _input;
    do {
        last = proc->ringAlone();
        tsp::PluginExecutor* next = proc->ringNext<ts::tsp::PluginExecutor>();
        proc->ringRemove();
        delete proc;
        proc = next;
    } while (!last);

    _input = nullptr;
    _output = nullptr;

    // Deallocate packet buffers.
    if (_packet_buffer != nullptr) {
        delete _packet_buffer;
        _packet_buffer = nullptr;
    }
    if (_metadata_buffer != nullptr) {
        delete _metadata_buffer;
        _metadata_buffer = nullptr;
    }
}


//----------------------------------------------------------------------------
// Start the TS processing.
//----------------------------------------------------------------------------

bool ts::TSProcessor::start(const TSProcessorArgs& args)
{
    // Initial sequence under mutex protection.
    {
        GuardMutex lock(_mutex);

        // Check if we are already started.
        if (_input != nullptr || _terminating) {
            _report.error(u"TS processing already started");
            return false;
        }

        // Keep command line options for further use.
        _args = args;

        // Check or adjust a few parameters.
        _args.ts_buffer_size = std::max(_args.ts_buffer_size, TSProcessorArgs::MIN_BUFFER_SIZE);

        // Clear errors on the report, used to check further initialisation errors.
        _report.resetErrors();

        // Load all plugins and analyze their command line arguments.
        // The first plugin is always the input and the last one is the output.
        // The input thread has the highest priority to be always ready to load
        // incoming packets in the buffer (avoid missing packets). The output
        // plugin has a hight priority to make room in the buffer, but not as
        // high as the input which must remain the top-most priority?

        _input = new tsp::InputExecutor(_args, *this, _args.input, ThreadAttributes().setPriority(ts::ThreadAttributes::GetMaximumPriority()), _mutex, &_report);
        CheckNonNull(_input);

        _output = new tsp::OutputExecutor(_args, *this, _args.output, ThreadAttributes().setPriority(ts::ThreadAttributes::GetHighPriority()), _mutex, &_report);
        CheckNonNull(_output);

        _output->ringInsertAfter(_input);

        // Check if at least one plugin prefers real-time defaults.
        bool realtime = _args.realtime == Tristate::True || _input->isRealTime() || _output->isRealTime();

        for (size_t i = 0; i < _args.plugins.size(); ++i) {
            tsp::PluginExecutor* p = new tsp::ProcessorExecutor(_args, *this, i, ThreadAttributes(), _mutex, &_report);
            CheckNonNull(p);
            p->ringInsertBefore(_output);
            realtime = realtime || p->isRealTime();
        }

        // Check if realtime defaults are explicitly disabled.
        if (_args.realtime == Tristate::False) {
            realtime = false;
        }

        // Now, we definitely know if we are in offline or realtime mode.
        // Adjust some default parameters.
        _args.applyDefaults(realtime);

        // Exit on error when initializing the plugins.
        if (_report.gotErrors()) {
            _report.debug(u"error when initializing the plugins");
            cleanupInternal();
            return false;
        }

        // Initialize all executors.
        tsp::PluginExecutor* proc = _input;
        do {
            // Set realtime defaults.
            proc->setRealTimeForAll(realtime);
            // Decode command line parameters for the plugin.
            if (!proc->plugin()->getOptions()) {
                _report.debug(u"getOptions() error in plugin %s", {proc->pluginName()});
                cleanupInternal();
                return false;
            }
        } while ((proc = proc->ringNext<ts::tsp::PluginExecutor>()) != _input);

        // Allocate a memory-resident buffer of TS packets
        _packet_buffer = new PacketBuffer(_args.ts_buffer_size / ts::PKT_SIZE);
        CheckNonNull(_packet_buffer);
        if (!_packet_buffer->isLocked()) {
            _report.debug(u"tsp: buffer failed to lock into physical memory (%d: %s), risk of real-time issue",
                          {_packet_buffer->lockErrorCode(), ts::SysErrorCodeMessage(_packet_buffer->lockErrorCode())});
        }
        _report.debug(u"tsp: buffer size: %'d TS packets, %'d bytes", {_packet_buffer->count(), _packet_buffer->count() * ts::PKT_SIZE});

        // Buffer for the packet metadata.
        // A packet and its metadata have the same index in their respective buffer.
        _metadata_buffer = new PacketMetadataBuffer(_packet_buffer->count());
        CheckNonNull(_metadata_buffer);

        // End of locked section.
    }

    // Start all processors, except output, in reverse order (input last).
    // Exit application in case of error.
    for (tsp::PluginExecutor* proc = _output->ringPrevious<tsp::PluginExecutor>(); proc != _output; proc = proc->ringPrevious<tsp::PluginExecutor>()) {
        if (!proc->plugin()->start()) {
            _report.debug(u"start() error in plugin %s", {proc->pluginName()});
            cleanupInternal();
            return false;
        }
    }

    // Initialize packet buffer in the ring of executors.
    // Exit application in case of error.
    if (!_input->initAllBuffers(_packet_buffer, _metadata_buffer)) {
        _report.debug(u"init buffer error");
        cleanupInternal();
        return false;
    }

    // Start the output device (we now have an idea of the bitrate).
    // Exit application in case of error.
    if (!_output->plugin()->start()) {
        _report.debug(u"start() error in output plugin %s", {_output->pluginName()});
        cleanupInternal();
        return false;
    }

    // Start all plugin executors threads.
    tsp::PluginExecutor* proc = _input;
    do {
        proc->start();
    } while ((proc = proc->ringNext<tsp::PluginExecutor>()) != _input);

    // Create a control server thread. Display but ignore errors (not a fatal error).
    _control = new tsp::ControlServer(_args, _report, _mutex, _input);
    CheckNonNull(_control);
    _control->open();

    return true;
}


//----------------------------------------------------------------------------
// Check if the TS processing is started.
//----------------------------------------------------------------------------

bool ts::TSProcessor::isStarted()
{
    GuardMutex lock(_mutex);
    return _input != nullptr && !_terminating;
}


//----------------------------------------------------------------------------
// Abort the processing.
//----------------------------------------------------------------------------

void ts::TSProcessor::abort()
{
    _report.debug(u"aborting all plugins...");

    GuardMutex lock(_mutex);

    if (_input != nullptr) {
        // Place all threads in "aborted" state so that each thread will see its
        // successor as aborted. Notify all threads that something happened.
        tsp::PluginExecutor* proc = _input;
        do {
            _report.debug(u"aborting plugin %s", {proc->pluginName()});
            proc->setAbort();
        } while ((proc = proc->ringNext<tsp::PluginExecutor>()) != _input);
    }
}


//----------------------------------------------------------------------------
// Suspend the calling thread until TS processing is completed.
//----------------------------------------------------------------------------

void ts::TSProcessor::waitForTermination()
{
    if (isStarted()) {
        // Wait for threads to terminate
        tsp::PluginExecutor* proc = _input;
        do {
            proc->waitForTermination();
        } while ((proc = proc->ringNext<tsp::PluginExecutor>()) != _input);

        // Make sure the control server thread is terminated before deleting plugins.
        _control->close();

        // Deallocate all plugins and plugin executor
        cleanupInternal();
    }
}
