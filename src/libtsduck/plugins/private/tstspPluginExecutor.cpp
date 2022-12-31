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

#include "tstspPluginExecutor.h"
#include "tsPluginRepository.h"
#include "tsGuardCondition.h"
#include "tsGuardMutex.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::tsp::PluginExecutor::PluginExecutor(const TSProcessorArgs& options,
                                        const PluginEventHandlerRegistry& handlers,
                                        PluginType type,
                                        const PluginOptions& pl_options,
                                        const ThreadAttributes& attributes,
                                        Mutex& global_mutex,
                                        Report* report) :

    JointTermination(options, type, pl_options, attributes, global_mutex, report),
    RingNode(),
    _buffer(nullptr),
    _metadata(nullptr),
    _suspended(false),
    _handlers(handlers),
    _to_do(),
    _pkt_first(0),
    _pkt_cnt(0),
    _input_end(false),
    _bitrate(0),
    _br_confidence(BitRateConfidence::LOW),
    _restart(false),
    _restart_data()
{
    // Preset common default options.
    if (plugin() != nullptr) {
        plugin()->resetContext(options.duck_args);
    }
}

ts::tsp::PluginExecutor::~PluginExecutor()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// Number of plugins in the chain. Inherited from TSP.
//----------------------------------------------------------------------------

size_t ts::tsp::PluginExecutor::pluginCount() const
{
    // Input plugin, all processor plugins, output plugin.
    return _options.plugins.size() + 2;
}


//----------------------------------------------------------------------------
// Signal a plugin event.
//----------------------------------------------------------------------------

void ts::tsp::PluginExecutor::signalPluginEvent(uint32_t event_code, Object* plugin_data) const
{
    const PluginEventContext ctx(event_code, pluginName(), pluginIndex(), pluginCount(), plugin(), plugin_data, bitrate(), pluginPackets(), totalPacketsInThread());
    _handlers.callEventHandlers(ctx);
}


//----------------------------------------------------------------------------
// This method sets the current processor in an abort state.
//----------------------------------------------------------------------------

void ts::tsp::PluginExecutor::setAbort()
{
    GuardMutex lock(_global_mutex);
    _tsp_aborting = true;
    ringPrevious<PluginExecutor>()->_to_do.signal();
}


//----------------------------------------------------------------------------
// Check if the plugin a real time one.
//----------------------------------------------------------------------------

bool ts::tsp::PluginExecutor::isRealTime() const
{
    return plugin() != nullptr && plugin()->isRealTime();
}


//----------------------------------------------------------------------------
// Set the initial state of the buffer.
// Executed in synchronous environment, before starting all executor threads.
//----------------------------------------------------------------------------

void ts::tsp::PluginExecutor::initBuffer(PacketBuffer*         buffer,
                                         PacketMetadataBuffer* metadata,
                                         size_t                pkt_first,
                                         size_t                pkt_cnt,
                                         bool                  input_end,
                                         bool                  aborted,
                                         const BitRate&        bitrate,
                                         BitRateConfidence     br_confidence)
{
    log(10, u"initBuffer(..., pkt_first = %'d, pkt_cnt = %'d, input_end = %s, aborted = %s, bitrate = %'d)", {pkt_first, pkt_cnt, input_end, aborted, bitrate});

    _buffer = buffer;
    _metadata = metadata;
    _pkt_first = pkt_first;
    _pkt_cnt = pkt_cnt;
    _input_end = input_end;
    _tsp_aborting = aborted;
    _bitrate = bitrate;
    _br_confidence = br_confidence;
    _tsp_bitrate = bitrate;
    _tsp_bitrate_confidence = br_confidence;
}


//----------------------------------------------------------------------------
// Signal that the specified number of packets have been processed.
//----------------------------------------------------------------------------

bool ts::tsp::PluginExecutor::passPackets(size_t count, const BitRate& bitrate, BitRateConfidence br_confidence, bool input_end, bool aborted)
{
    assert(count <= _pkt_cnt);

    log(10, u"passPackets(count = %'d, bitrate = %'d, input_end = %s, aborted = %s)", {count, bitrate, input_end, aborted});

    // We access data under the protection of the global mutex.
    GuardMutex lock(_global_mutex);

    // Update our buffer: we remove the first 'count' packets from the beginning of our slice of the buffer.
    _pkt_first = (_pkt_first + count) % _buffer->count();
    _pkt_cnt -= count;

    // Update next processor's buffer: add 'count' packets at the end of its slice of the buffer.
    PluginExecutor* next = ringNext<PluginExecutor>();
    next->_pkt_cnt += count;

    // Propagate bitrate and end of input flag to next processor.
    next->_bitrate = bitrate;
    next->_br_confidence = br_confidence;
    next->_input_end = next->_input_end || input_end;

    // Wake the next processor when there is some new input data or end of input.
    if (count > 0 || input_end) {
        next->_to_do.signal();
    }

    // Force to abort our processor when the next one is aborting. Already done in waitWork() but force immediately.
    // Don't do that if current is output and next is input because there is no propagation of packets from output back to input.
    if (plugin()->type() != PluginType::OUTPUT) {
        aborted = aborted || next->_tsp_aborting;
    }

    // Wake the previous processor when we abort (propagate abort conditions backward).
    if (aborted) {
        _tsp_aborting = true; // volatile bool in TSP superclass
        ringPrevious<PluginExecutor>()->_to_do.signal();
    }

    // Return false when the current processor shall stop.
    return !input_end && !aborted;
}


//----------------------------------------------------------------------------
// Wait for packets to process or some error condition.
//----------------------------------------------------------------------------

void ts::tsp::PluginExecutor::waitWork(size_t min_pkt_cnt, size_t& pkt_first, size_t& pkt_cnt,
                                       BitRate& bitrate, BitRateConfidence& br_confidence,
                                       bool& input_end, bool& aborted, bool &timeout)
{
    log(10, u"waitWork(min_pkt_cnt = %'d, ...)", {min_pkt_cnt});

    // Cannot allocate more than the buffer size.
    if (min_pkt_cnt > _buffer->count()) {
        debug(u"requests too many packets at a time: %'d, larger than buffer size: %'d", {min_pkt_cnt, _buffer->count()});
        min_pkt_cnt = _buffer->count();
    }

    // We access data under the protection of the global mutex.
    GuardCondition lock(_global_mutex, _to_do);

    PluginExecutor* next = ringNext<PluginExecutor>();
    timeout = false;

    // Loop until enough packets are available (or some error condition).
    while (_pkt_cnt < min_pkt_cnt && !_input_end && !timeout && !next->_tsp_aborting) {
        // If packet area for this processor is empty, wait for some packet.
        // The mutex is implicitely released, we wait for the condition
        // '_to_do' and, once we get it, implicitely relock the mutex.
        // We loop on this until packets are actually available.
        // If there is a timeout in the packet reception, call the plugin handler.
        timeout = !lock.waitCondition(_tsp_timeout) && !plugin()->handlePacketTimeout();
    }

    // The number of returned packets is limited up to the wrap-up point of the circular buffer,
    // if allowed by the requested minimum number of packets.
    if (timeout) {
        // Nothing returned.
        pkt_cnt = 0;
    }
    else if (_pkt_first + min_pkt_cnt <= _buffer->count()) {
        // Return up to the wrap-up point. This will satisfy the requested minimum.
        pkt_cnt = std::min(_pkt_cnt, _buffer->count() - _pkt_first);
    }
    else {
        // The requested minimum does not fit into a contiguous area.
        pkt_cnt = _pkt_cnt;
    }

    pkt_first = _pkt_first;
    bitrate = _bitrate;
    br_confidence = _br_confidence;
    input_end = _input_end && pkt_cnt == _pkt_cnt;

    // Force to abort our processor when the next one is aborting.
    // Don't do that if current is output and next is input because
    // there is no propagation of packets from output back to input.
    aborted = plugin()->type() != PluginType::OUTPUT && next->_tsp_aborting;

    log(10, u"waitWork(min_pkt_cnt = %'d, pkt_first = %'d, pkt_cnt = %'d, bitrate = %'d, input_end = %s, aborted = %s, timeout = %s)",
        {min_pkt_cnt, pkt_first, pkt_cnt, bitrate, input_end, aborted, timeout});
}


//----------------------------------------------------------------------------
// Description of a restart operation (constructor).
//----------------------------------------------------------------------------

ts::tsp::PluginExecutor::RestartData::RestartData(const UStringVector& params, bool same, Report& rep) :
    report(rep),
    same_args(same),
    args(params),
    mutex(),
    condition(),
    completed(false)
{
}


//----------------------------------------------------------------------------
// Restart the plugin.
//----------------------------------------------------------------------------

void ts::tsp::PluginExecutor::restart(Report& report)
{
    restart(RestartDataPtr(new RestartData(UStringVector(), true, report)));
}

void ts::tsp::PluginExecutor::restart(const UStringVector& params, Report& report)
{
    restart(RestartDataPtr(new RestartData(params, false, report)));
}

void ts::tsp::PluginExecutor::restart(const RestartDataPtr& rd)
{
    // Acquire the global mutex to modify global data.
    // To avoid deadlocks, always acquire the global mutex first, then a RestartData mutex.
    {
        GuardCondition lock1(_global_mutex, _to_do);

        // If there was a previous pending restart operation, cancel it.
        if (!_restart_data.isNull()) {
            GuardCondition lock2(_restart_data->mutex, _restart_data->condition);
            _restart_data->completed = true;
            _restart_data->report.error(u"restart interrupted by another concurrent restart");
            // Notify the waiting thread that its restart command is aborted.
            lock2.signal();
        }

        // Declare this new restart operation.
        _restart_data = rd;
        _restart = true;

        // Signal the plugin thread that there is something to do.
        lock1.signal();
    }

    // Now wait for the restart operation to complete.
    GuardCondition lock3(rd->mutex, rd->condition);
    while (!rd->completed) {
        lock3.waitCondition();
    }
}


//----------------------------------------------------------------------------
// Check if there is a pending restart operation (but do not execute it).
//----------------------------------------------------------------------------

bool ts::tsp::PluginExecutor::pendingRestart()
{
    GuardMutex lock(_global_mutex);
    return _restart && !_restart_data.isNull();
}


//----------------------------------------------------------------------------
// Process a pending restart operation if there is one.
//----------------------------------------------------------------------------

bool ts::tsp::PluginExecutor::processPendingRestart(bool& restarted)
{
    // Run under the protection of the global mutex.
    // To avoid deadlocks, always acquire the global mutex first, then a RestartData mutex.
    GuardMutex lock1(_global_mutex);

    // If there is no pending restart, immediate success.
    if (!_restart || _restart_data.isNull()) {
        restarted = false;
        return true;
    }

    // There will be a restart attempt.
    restarted = true;

    // Now lock the content of the restart data.
    GuardCondition lock2(_restart_data->mutex, _restart_data->condition);

    // Verbose message in the current tsp process and back to the remote tspcontrol.
    verbose(u"restarting due to remote tspcontrol");
    _restart_data->report.verbose(u"restarting plugin %s", {pluginName()});

    // First, stop the current execution.
    plugin()->stop();

    // Inform the TSP layer to reset plugin session accounting.
    restartPluginSession();

    // Reset the execution context to cleanup previous plugin-specific options or accumulated data.
    plugin()->resetContext(_options.duck_args);

    // Redirect error messages from command line analysis to the remote tspcontrol.
    Report* previous_report = plugin()->redirectReport(&_restart_data->report);

    bool success = false;
    if (_restart_data->same_args) {
        // Restart with same arguments, no need to reanalyze the command.
        success = plugin()->start();
    }
    else {
        // Save previous arguments to restart with the previous configuration if the restart fails with the new arguments.
        UStringVector previous_args;
        plugin()->getCommandArgs(previous_args);

        // This command line analysis shall not affect the tsp process.
        plugin()->setFlags(plugin()->getFlags() | Args::NO_HELP | Args::NO_EXIT_ON_ERROR);

        // Try to restart with the new command line arguments.
        success = plugin()->analyze(pluginName(), _restart_data->args, false) && plugin()->getOptions() && plugin()->start();

        // In case of restart failure, try to restart with the previous arguments.
        if (!success) {
            _restart_data->report.warning(u"failed to restart plugin %s, restarting with previous parameters", {pluginName()});
            success = plugin()->analyze(pluginName(), previous_args, false) && plugin()->getOptions() && plugin()->start();
        }
    }

    // Restore error messages to previous report.
    plugin()->redirectReport(previous_report);

    // Finally notify the calling thread that the restart is completed.
    _restart_data->completed = true;
    lock2.signal();

    // Clear restart trigger.
    _restart = false;
    _restart_data.clear();

    debug(u"restarted plugin %s, status: %s", {pluginName(), success});
    return success;
}
