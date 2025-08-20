//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstsswitchInputExecutor.h"
#include "tstsswitchCore.h"


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
    _buffer(opt.buffered_packets),
    _metadata(opt.buffered_packets)
{
    // Make sure that the input plugins display their index.
    setLogName(UString::Format(u"%s[%d]", pluginName(), _pluginIndex));
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
    debug(u"received start request, current: %s", isCurrent);

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _isCurrent = isCurrent;
    _startRequest = true;
    _stopRequest = false;
    _todo.notify_one();
}


//----------------------------------------------------------------------------
// Stop input.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::stopInput()
{
    debug(u"received stop request");

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _startRequest = false;
    _stopRequest = true;
    _todo.notify_one();
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
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _isCurrent = isCurrent;
}


//----------------------------------------------------------------------------
// Terminate input.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::terminateInput()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _terminated = true;
    _todo.notify_one();
}


//----------------------------------------------------------------------------
// Get some packets to output.
// Indirectly called from the output plugin when it needs some packets.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::getOutputArea(ts::TSPacket*& first, TSPacketMetadata*& data, size_t& count)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    first = &_buffer[_outFirst];
    data = &_metadata[_outFirst];
    count = std::min(_outCount, _buffer.size() - _outFirst);
    _outputInUse = count > 0;
    _todo.notify_one();
}


//----------------------------------------------------------------------------
// Free output packets (after being sent).
// Indirectly called from the output plugin after sending packets.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::freeOutput(size_t count)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    assert(count <= _outCount);
    _outFirst = (_outFirst + count) % _buffer.size();
    _outCount -= count;
    _outputInUse = false;
    _todo.notify_one();
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
            std::unique_lock<std::recursive_mutex> lock(_mutex);
            // Reset input buffer.
            _outFirst = 0;
            _outCount = 0;
            // Wait for start or terminate.
            while (!_startRequest && !_terminated) {
                _todo.wait(lock);
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
        debug(u"input plugin started, status: %s", started);
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
                std::unique_lock<std::recursive_mutex> lock(_mutex);
                while (_outCount >= _buffer.size() && !_stopRequest && !_terminated) {
                    if (_isCurrent || !_opt.fast_switch) {
                        // This is the current input, we must not lose packet.
                        // Wait for the output thread to free some packets.
                        _todo.wait(lock);
                    }
                    else {
                        // Not the current input plugin in --fast-switch mode.
                        // Drop older packets, free at most --max-input-packets.
                        assert(_outFirst < _buffer.size());
                        const size_t freeCount = std::min(_opt.max_input_packets, _buffer.size() - _outFirst);
                        assert(freeCount <= _outCount);
                        _outFirst = (_outFirst + freeCount) % _buffer.size();
                        _outCount -= freeCount;
                    }
                }
                // Exit input when termination is requested.
                if (_stopRequest || _terminated) {
                    debug(u"exiting session: stop request: %s, terminated: %s", _stopRequest, _terminated);
                    break;
                }
                // There is some free buffer, compute first index and size of receive area.
                // The receive area is limited by end of buffer and max input size.
                inFirst = (_outFirst + _outCount) % _buffer.size();
                inCount = std::min(_opt.max_input_packets, std::min(_buffer.size() - _outCount, _buffer.size() - inFirst));
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
                const cn::nanoseconds current = monotonic_time::clock::now() - _start_time;
                for (size_t n = 0; n < inCount; ++n) {
                    _metadata[inFirst + n].setInputTimeStamp(current, TimeSource::TSP);
                }
            }

            // Signal the presence of received packets.
            {
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                _outCount += inCount;
            }
            _core.inputReceived(_pluginIndex);
        }

        // At end of session, make sure that the output buffer is not in use by the output plugin.
        {
            // Wait for the output plugin to release the buffer.
            // In case of normal end of input (no stop, no terminate), wait for all output to be gone.
            std::unique_lock<std::recursive_mutex> lock(_mutex);
            while (_outputInUse || (_outCount > 0 && !_stopRequest && !_terminated)) {
                debug(u"input terminated, waiting for output plugin to release the buffer");
                _todo.wait(lock);
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
