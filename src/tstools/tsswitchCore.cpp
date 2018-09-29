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

#include "tsswitchCore.h"
#include "tsGuard.h"
#include "tsGuardCondition.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsswitch::Core::Core(int argc, char *argv[]) :
    opt(argc, argv),
    log(opt.maxSeverity(), opt.logTimeStamp, opt.logMaxBuffer, opt.logSynchronous),
    _inputs(opt.inputs.size(), 0),
    _output(*this), // load plugin and analyze options
    _mutex(),
    _gotInput(),
    _curPlugin(opt.firstInput),
    _curCycle(0),
    _terminate(false),
    _actions(),
    _events()
{
    // Load all input plugins, analyze their options.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i] = new InputExecutor(*this, i);
        CheckNonNull(_inputs[i]);
        // Set the asynchronous logger as report method for all executors.
        _inputs[i]->setReport(&log);
        _inputs[i]->setMaxSeverity(log.maxSeverity());
    }

    // Set the asynchronous logger as report method for output as well.
    _output.setReport(&log);
    _output.setMaxSeverity(log.maxSeverity());
}

ts::tsswitch::Core::~Core()
{
    // Deallocate all input plugins.
    // The destructor of each plugin waits for its termination.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        delete _inputs[i];
    }
    _inputs.clear();
}


//----------------------------------------------------------------------------
// Start the tsswitch processing.
//----------------------------------------------------------------------------

bool ts::tsswitch::Core::start()
{
    // Get all input plugin options.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        if (!_inputs[i]->plugin()->getOptions()) {
            return false;
        }
    }

    // Start output plugin.
    if (!_output.plugin()->getOptions() ||  // Let plugin fetch its command line options.
        !_output.plugin()->start() ||       // Open the output "device", whatever it means.
        !_output.start())                   // Start the output thread.
    {
        return false;
    }

    // Start with the designated first input plugin.
    assert(opt.firstInput < _inputs.size());
    _curPlugin = opt.firstInput;

    // Start all input threads (but do not open the input "devices").
    bool success = true;
    for (size_t i = 0; success && i < _inputs.size(); ++i) {
        // Here, start() means start the thread, not start input plugin.
        success = _inputs[i]->start();
    }

    if (!success) {
        // If one input thread could not start, abort all started threads.
        stop(false);
    }
    else if (opt.fastSwitch) {
        // Option --fast-switch, start all plugins, they continue to receive in parallel.
        for (size_t i = 0; i < _inputs.size(); ++i) {
            _inputs[i]->startInput(i == _curPlugin);
        }
    }
    else {
        // Start the first plugin only.
        _inputs[_curPlugin]->startInput(true);
    }

    return success;
}


//----------------------------------------------------------------------------
// Stop the tsswitch processing.
//----------------------------------------------------------------------------

void ts::tsswitch::Core::stop(bool success)
{
    // Wake up all threads waiting for something on the Switch object.
    {
        GuardCondition lock(_mutex, _gotInput);
        _terminate = true;
        lock.signal();
    }
    // Tell the output plugin to terminate.
    _output.terminateOutput();
    // Tell all input plugins to terminate.
    for (size_t i = 0; success && i < _inputs.size(); ++i) {
        _inputs[i]->terminateInput();
    }
}


//----------------------------------------------------------------------------
// Switch input plugins.
//----------------------------------------------------------------------------

void ts::tsswitch::Core::setInput(size_t index)
{
    Guard lock(_mutex);
    setInputLocked(index);
}

void ts::tsswitch::Core::nextInput()
{
    Guard lock(_mutex);
    setInputLocked((_curPlugin + 1) % _inputs.size());
}

void ts::tsswitch::Core::prevInput()
{
    Guard lock(_mutex);
    setInputLocked((_curPlugin > 0 ? _curPlugin : _inputs.size()) - 1);
}


//----------------------------------------------------------------------------
// Change input plugin with mutex already held.
//----------------------------------------------------------------------------

void ts::tsswitch::Core::setInputLocked(size_t index)
{
    if (index >= _inputs.size()) {
        log.warning(u"invalid input index %d", {index});
    }
    else if (index != _curPlugin) {
        log.debug(u"switch input %d to %d", {_curPlugin, index});

        // The processing depends on the switching mode.
        if (opt.fastSwitch) {
            // Don't start/stop plugins. Just inform the plugin that it is current.
            // The only impact is that the non-current plugins will drop packets on buffer overflow.
            enqueue(Action(NOTIF_CURRENT, _curPlugin, false));
            enqueue(Action(SET_CURRENT, index));
            enqueue(Action(NOTIF_CURRENT, index, true));
        }
        else if (opt.delayedSwitch) {
            // With --delayed-switch, first start the next plugin.
            // The current plugin will be stopped when the first packet is received in the next plugin.
            enqueue(Action(START, index, false));
            enqueue(Action(WAIT_INPUT, index));
            enqueue(Action(SET_CURRENT, index));
            enqueue(Action(STOP, _curPlugin));
            enqueue(Action(WAIT_STOPPED, _curPlugin));
        }
        else {
            // Default switch mode.
            enqueue(Action(STOP, _curPlugin));
            enqueue(Action(WAIT_STOPPED, _curPlugin));
            enqueue(Action(SET_CURRENT, index));
            enqueue(Action(START, index, true));
            enqueue(Action(WAIT_STARTED, index));
        }

        // Execute actions.
        execute();
    }
}


//----------------------------------------------------------------------------
// Names of actions for debug messages.
//----------------------------------------------------------------------------

const ts::Enumeration ts::tsswitch::Core::_actionNames({
    {u"NONE",          NONE},
    {u"START",         START},
    {u"WAIT_STARTED",  WAIT_STARTED},
    {u"WAIT_INPUT",    WAIT_INPUT},
    {u"STOP",          STOP},
    {u"WAIT_STOPPED",  WAIT_STOPPED},
    {u"NOTIF_CURRENT", NOTIF_CURRENT},
    {u"SET_CURRENT",   SET_CURRENT},
});


//----------------------------------------------------------------------------
// Stringify an Action object.
//----------------------------------------------------------------------------

ts::UString ts::tsswitch::Core::Action::toString() const
{
    return UString::Format(u"%s, %d, %s", {_actionNames.name(type), index, flag});
}


//----------------------------------------------------------------------------
// Operator "less" for containers of Action objects.
//----------------------------------------------------------------------------

bool ts::tsswitch::Core::Action::operator<(const Action& a) const
{
    if (type != a.type) {
        return type < a.type;
    }
    else if (index != a.index) {
        return index < a.index;
    }
    else {
        return int(flag) < int(a.flag);
    }
}


//----------------------------------------------------------------------------
// Enqueue an action.
//----------------------------------------------------------------------------

void ts::tsswitch::Core::enqueue(const Action& action)
{
    log.debug(u"enqueue action %s", {action});
    _actions.push_back(action);
}


//----------------------------------------------------------------------------
// Execute all commands until one needs to wait.
//----------------------------------------------------------------------------

void ts::tsswitch::Core::execute(const Action& event)
{
    // Ignore flag in event.
    Action eventNoFlag(event);
    eventNoFlag.flag = false;

    // Set current event.
    if (_events.find(eventNoFlag) == _events.end()) {
        // The event was not present.
        _events.insert(eventNoFlag);
        log.debug(u"setting event: %s", {event});
    }

    // Loop on all enqueued commands.
    while (!_actions.empty()) {

        // Inpect front command. Will be dequeued if executed.
        const Action& action(_actions.front());
        log.debug(u"executing action %s", {action});
        assert(action.index < _inputs.size());

        // Try to execute the front command. Return if wait is required.
        switch (action.type) {
            case NONE: {
                break;
            }
            case START: {
                _inputs[action.index]->startInput(action.flag);
                break;
            }
            case STOP: {
                _inputs[action.index]->stopInput();
                break;
            }
            case NOTIF_CURRENT: {
                _inputs[action.index]->setCurrent(action.flag);
                break;
            }
            case SET_CURRENT: {
                _curPlugin = action.index;
                break;
            }
            case WAIT_STARTED:
            case WAIT_INPUT:
            case WAIT_STOPPED: {
                // Wait commands, check if an event of this type is pending.
                const ActionSet::const_iterator it(_events.find(eventNoFlag));
                if (it == _events.end()) {
                    // Event not found, cannot execute further, keep the action in queue and retry later.
                    log.debug(u"not ready, waiting: %s", {action});
                    return;
                }
                // Clear the event.
                _events.erase(it);
                break;
            }
            default: {
                // Unknown action.
                assert(false);
            }
        }

        // Command executed, dequeue it.
        _actions.pop_front();
    }
}


//----------------------------------------------------------------------------
// Get some packets to output (called by output plugin).
//----------------------------------------------------------------------------

bool ts::tsswitch::Core::getOutputArea(size_t& pluginIndex, TSPacket*& first, size_t& count)
{
    assert(pluginIndex < _inputs.size());

    // Loop on _gotInput condition until the current input plugin has something to output.
    GuardCondition lock(_mutex, _gotInput);
    for (;;) {
        if (_terminate) {
            first = 0;
            count = 0;
        }
        else {
            _inputs[_curPlugin]->getOutputArea(first, count);
        }
        // Return when there is something to output in current plugin or the application terminates.
        if (count > 0 || _terminate) {
            // Tell the output plugin which input plugin is used.
            pluginIndex = _curPlugin;
            // Return false when the application terminates.
            return !_terminate;
        }
        // Otherwise, sleep on _gotInput condition.
        lock.waitCondition();
    }
}


//----------------------------------------------------------------------------
// Report output packets (called by output plugin).
//----------------------------------------------------------------------------

bool ts::tsswitch::Core::outputSent(size_t pluginIndex, size_t count)
{
    assert(pluginIndex < _inputs.size());

    // Inform the input plugin that the packets can be reused for input.
    // We notify the original input plugin from which the packets came.
    // The "current" input plugin may have changed in the meantime.
    _inputs[pluginIndex]->freeOutput(count);

    // Return false when the application terminates.
    return !_terminate;
}


//----------------------------------------------------------------------------
// Report completion of input start (called by input plugins).
//----------------------------------------------------------------------------

bool ts::tsswitch::Core::inputStarted(size_t pluginIndex, bool success)
{
    Guard lock(_mutex);

    // Execute all commands if waiting on this event.
    execute(Action(WAIT_STARTED, pluginIndex, success));

    // Return false when the application terminates.
    return !_terminate;
}


//----------------------------------------------------------------------------
// Report input reception of packets (called by input plugins).
//----------------------------------------------------------------------------

bool ts::tsswitch::Core::inputReceived(size_t pluginIndex)
{
    GuardCondition lock(_mutex, _gotInput);

    // Execute all commands if waiting on this event.
    execute(Action(WAIT_INPUT, pluginIndex));

    if (pluginIndex == _curPlugin) {
        // Wake up output plugin if it is sleeping, waiting for packets to output.
        lock.signal();
    }

    // Return false when the application terminates.
    return !_terminate;
}


//----------------------------------------------------------------------------
// Report completion of input session (called by input plugins).
//----------------------------------------------------------------------------

bool ts::tsswitch::Core::inputStopped(size_t pluginIndex, bool success)
{
    bool stopRequest = false;

    // Locked sequence.
    {
        Guard lock(_mutex);
        log.debug(u"input %d completed, success: %s", {pluginIndex, success});

        // Count end of cycle when the last plugin terminates.
        if (pluginIndex == _inputs.size() - 1) {
            _curCycle++;
        }

        // Check if the complete processing is terminated.
        stopRequest = opt.terminate || (opt.cycleCount > 0 && _curCycle >= opt.cycleCount);

        // If the current plugin terminates and there is nothing else to execute, move to next plugin.
        if (pluginIndex == _curPlugin && _actions.empty()) {
            const size_t next = (_curPlugin + 1) % _inputs.size();
            enqueue(Action(SET_CURRENT, next));
            if (opt.fastSwitch) {
                // Already started, never stop, simply notify.
                enqueue(Action(NOTIF_CURRENT, next));
            }
            else {
                enqueue(Action(START, next));
                enqueue(Action(WAIT_STARTED, next));
            }
        }

        // Execute all commands if waiting on this event.
        execute(Action(WAIT_STOPPED, pluginIndex));
    }

    // Stop everything when we reach the end of the tsswitch processing.
    // This must be done outside the locked sequence to avoid deadlocks.
    if (stopRequest) {
        stop(true);
    }

    // Return false when the application terminates.
    return !_terminate;
}


//----------------------------------------------------------------------------
// Wait for completion of all plugins.
//----------------------------------------------------------------------------

void ts::tsswitch::Core::waitForTermination()
{
    // Wait for output termination.
    _output.waitForTermination();

    // Wait for all input termination.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i]->waitForTermination();
    }
}
