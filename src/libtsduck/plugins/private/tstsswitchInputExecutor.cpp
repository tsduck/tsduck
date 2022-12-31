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

#include "tstsswitchInputExecutor.h"
#include "tstsswitchCore.h"
#include "tsGuardMutex.h"
#include "tsGuardCondition.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsswitch::InputExecutor::InputExecutor(const InputSwitcherArgs& opt,
                                           const PluginEventHandlerRegistry& handlers,
                                           size_t index,
                                           Core& core,
                                           Report& log) :

    // Input threads have a high priority to be always ready to load incoming packets in the buffer.
    PluginExecutor(opt, handlers, PluginType::INPUT, opt.inputs[index], ThreadAttributes().setPriority(ThreadAttributes::GetHighPriority()), core, log),
    _input(dynamic_cast<InputPlugin*>(PluginThread::plugin())),
    _pluginIndex(index),
    _buffer(opt.bufferedPackets),
    _metadata(opt.bufferedPackets),
    _mutex(),
    _todo(),
    _isCurrent(false),
    _outputInUse(false),
    _startRequest(false),
    _stopRequest(false),
    _terminated(false),
    _outFirst(0),
    _outCount(0),
    _start_time(true) // initialized with current system time
{
    // Make sure that the input plugins display their index.
    setLogName(UString::Format(u"%s[%d]", {pluginName(), _pluginIndex}));
}

ts::tsswitch::InputExecutor::~InputExecutor()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// Implementation of TSP.
//----------------------------------------------------------------------------

size_t ts::tsswitch::InputExecutor::pluginIndex() const
{
    return _pluginIndex;
}


//----------------------------------------------------------------------------
// Start input.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::startInput(bool isCurrent)
{
    debug(u"received start request, current: %s", {isCurrent});

    GuardCondition lock(_mutex, _todo);
    _isCurrent = isCurrent;
    _startRequest = true;
    _stopRequest = false;
    lock.signal();
}


//----------------------------------------------------------------------------
// Stop input.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::stopInput()
{
    debug(u"received stop request");

    GuardCondition lock(_mutex, _todo);
    _startRequest = false;
    _stopRequest = true;
    lock.signal();
}


//----------------------------------------------------------------------------
// Abort the input operation currently in progress in the plugin.
//----------------------------------------------------------------------------

bool ts::tsswitch::InputExecutor::abortInput()
{
    return _input != nullptr && _input->abortInput();
}


//----------------------------------------------------------------------------
// Set/reset as current input plugin. Do not start or stop it.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::setCurrent(bool isCurrent)
{
    GuardMutex lock(_mutex);
    _isCurrent = isCurrent;
}


//----------------------------------------------------------------------------
// Terminate input.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::terminateInput()
{
    GuardCondition lock(_mutex, _todo);
    _terminated = true;
    lock.signal();
}


//----------------------------------------------------------------------------
// Get some packets to output.
// Indirectly called from the output plugin when it needs some packets.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::getOutputArea(ts::TSPacket*& first, TSPacketMetadata*& data, size_t& count)
{
    GuardCondition lock(_mutex, _todo);
    first = &_buffer[_outFirst];
    data = &_metadata[_outFirst];
    count = std::min(_outCount, _buffer.size() - _outFirst);
    _outputInUse = count > 0;
    lock.signal();
}


//----------------------------------------------------------------------------
// Free output packets (after being sent).
// Indirectly called from the output plugin after sending packets.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::freeOutput(size_t count)
{
    GuardCondition lock(_mutex, _todo);
    assert(count <= _outCount);
    _outFirst = (_outFirst + count) % _buffer.size();
    _outCount -= count;
    _outputInUse = false;
    lock.signal();
}


//----------------------------------------------------------------------------
// Invoked in the context of the plugin thread.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::main()
{
    debug(u"input thread started");

    // Main loop. Each iteration is a complete input session.
    for (;;) {

        // Initial sequence under mutex protection.
        debug(u"waiting for input session");
        {
            GuardCondition lock(_mutex, _todo);
            // Reset input buffer.
            _outFirst = 0;
            _outCount = 0;
            // Wait for start or terminate.
            while (!_startRequest && !_terminated) {
                lock.waitCondition();
            }
            // Exit main loop when termination is requested.
            if (_terminated) {
                break;
            }
            // At this point, start is requested, reset trigger.
            _startRequest = false;
            _stopRequest = false;
            // Inform the TSP layer to reset plugin session accounting.
            restartPluginSession();
        }

        // Here, we need to start an input session.
        debug(u"starting input plugin");
        const bool started = _input->start();
        debug(u"input plugin started, status: %s", {started});
        _core.inputStarted(_pluginIndex, started);

        if (!started) {
            // Failed to start.
            _core.inputStopped(_pluginIndex, false);
            // Loop back, waiting for a new session.
            continue;
        }

        // Loop on incoming packets.
        for (;;) {

            // Input area (first packet index and packet count).
            size_t inFirst = 0;
            size_t inCount = 0;

            // Initial sequence under mutex protection.
            {
                // Wait for free buffer or stop.
                GuardCondition lock(_mutex, _todo);
                while (_outCount >= _buffer.size() && !_stopRequest && !_terminated) {
                    if (_isCurrent || !_opt.fastSwitch) {
                        // This is the current input, we must not lose packet.
                        // Wait for the output thread to free some packets.
                        lock.waitCondition();
                    }
                    else {
                        // Not the current input plugin in --fast-switch mode.
                        // Drop older packets, free at most --max-input-packets.
                        assert(_outFirst < _buffer.size());
                        const size_t freeCount = std::min(_opt.maxInputPackets, _buffer.size() - _outFirst);
                        assert(freeCount <= _outCount);
                        _outFirst = (_outFirst + freeCount) % _buffer.size();
                        _outCount -= freeCount;
                    }
                }
                // Exit input when termination is requested.
                if (_stopRequest || _terminated) {
                    debug(u"exiting session: stop request: %s, terminated: %s", {_stopRequest, _terminated});
                    break;
                }
                // There is some free buffer, compute first index and size of receive area.
                // The receive area is limited by end of buffer and max input size.
                inFirst = (_outFirst + _outCount) % _buffer.size();
                inCount = std::min(_opt.maxInputPackets, std::min(_buffer.size() - _outCount, _buffer.size() - inFirst));
            }

            assert(inFirst < _buffer.size());
            assert(inFirst + inCount <= _buffer.size());

            // Reset packet metadata.
            for (size_t n = inFirst; n < inFirst + inCount; ++n) {
                _metadata[n].reset();
            }

            // Receive packets.
            if ((inCount = _input->receive(&_buffer[inFirst], &_metadata[inFirst], inCount)) == 0) {
                // End of input.
                debug(u"received end of input from plugin");
                break;
            }
            addPluginPackets(inCount);

            // Fill input time stamps with monotonic clock if none was provided by the input plugin.
            // Only check the first returned packet. Assume that the input plugin generates time stamps for all or none.
            if (!_metadata[inFirst].hasInputTimeStamp()) {
                const NanoSecond current = Monotonic(true) - _start_time;
                for (size_t n = 0; n < inCount; ++n) {
                    _metadata[inFirst + n].setInputTimeStamp(current, NanoSecPerSec, TimeSource::TSP);
                }
            }

            // Signal the presence of received packets.
            {
                GuardMutex lock(_mutex);
                _outCount += inCount;
            }
            _core.inputReceived(_pluginIndex);
        }

        // At end of session, make sure that the output buffer is not in use by the output plugin.
        {
            // Wait for the output plugin to release the buffer.
            // In case of normal end of input (no stop, no terminate), wait for all output to be gone.
            GuardCondition lock(_mutex, _todo);
            while (_outputInUse || (_outCount > 0 && !_stopRequest && !_terminated)) {
                debug(u"input terminated, waiting for output plugin to release the buffer");
                lock.waitCondition();
            }
            // And reset the output part of the buffer.
            _outFirst = 0;
            _outCount = 0;
        }

        // End of input session.
        debug(u"stopping input plugin");
        _core.inputStopped(_pluginIndex, _input->stop());
    }

    debug(u"input thread terminated");
}
