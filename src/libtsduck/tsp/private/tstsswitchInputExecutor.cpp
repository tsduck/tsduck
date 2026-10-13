//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
    _plugin_index(index),
    _buffer(opt.buffered_packets),
    _metadata(opt.buffered_packets)
{
    // Make sure that the input plugins display their index.
    setLogName(UString::Format(u"%s[%d]", pluginName(), _plugin_index));
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
    return _plugin_index;
}


//----------------------------------------------------------------------------
// Start input.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::startInput(bool isCurrent)
{
    debug(u"received start request, current: %s", isCurrent);

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _is_current = isCurrent;
    _start_request = true;
    _stop_request = false;
    _todo.notify_one();
}


//----------------------------------------------------------------------------
// Stop input.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::stopInput()
{
    debug(u"received stop request");

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _start_request = false;
    _stop_request = true;
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
    _is_current = isCurrent;
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
    first = &_buffer[_out_first];
    data = &_metadata[_out_first];
    count = std::min(_out_count, _buffer.size() - _out_first);
    _output_in_use = count > 0;
    _todo.notify_one();
}


//----------------------------------------------------------------------------
// Free output packets (after being sent).
// Indirectly called from the output plugin after sending packets.
//----------------------------------------------------------------------------

void ts::tsswitch::InputExecutor::freeOutput(size_t count)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    assert(count <= _out_count);
    _out_first = (_out_first + count) % _buffer.size();
    _out_count -= count;
    _output_in_use = false;
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
            _out_first = 0;
            _out_count = 0;
            // Wait for start or terminate.
            while (!_start_request && !_terminated) {
                _todo.wait(lock);
            }
            // Exit main loop when termination is requested.
            if (_terminated) {
                break;
            }
            // At this point, start is requested, reset trigger.
            _start_request = false;
            _stop_request = false;
            // Inform the TSP layer to reset plugin session accounting.
            restartPluginSession();
        }

        // Here, we need to start an input session.
        debug(u"starting input plugin");
        const bool started = _input->start();
        debug(u"input plugin started, status: %s", started);
        _core.inputStarted(_plugin_index, started);

        if (!started) {
            // Failed to start.
            _core.inputStopped(_plugin_index, false);
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
                while (_out_count >= _buffer.size() && !_stop_request && !_terminated) {
                    if (_is_current || !_opt.fast_switch) {
                        // This is the current input, we must not lose packet.
                        // Wait for the output thread to free some packets.
                        _todo.wait(lock);
                    }
                    else {
                        // Not the current input plugin in --fast-switch mode.
                        // Drop older packets, free at most --max-input-packets.
                        assert(_out_first < _buffer.size());
                        const size_t freeCount = std::min(_opt.max_input_packets, _buffer.size() - _out_first);
                        assert(freeCount <= _out_count);
                        _out_first = (_out_first + freeCount) % _buffer.size();
                        _out_count -= freeCount;
                    }
                }
                // Exit input when termination is requested.
                if (_stop_request || _terminated) {
                    debug(u"exiting session: stop request: %s, terminated: %s", _stop_request, _terminated);
                    break;
                }
                // There is some free buffer, compute first index and size of receive area.
                // The receive area is limited by end of buffer and max input size.
                inFirst = (_out_first + _out_count) % _buffer.size();
                inCount = std::min(_opt.max_input_packets, std::min(_buffer.size() - _out_count, _buffer.size() - inFirst));
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
                _out_count += inCount;
            }
            _core.inputReceived(_plugin_index);
        }

        // At end of session, make sure that the output buffer is not in use by the output plugin.
        {
            // Wait for the output plugin to release the buffer.
            // In case of normal end of input (no stop, no terminate), wait for all output to be gone.
            std::unique_lock<std::recursive_mutex> lock(_mutex);
            while (_output_in_use || (_out_count > 0 && !_stop_request && !_terminated)) {
                debug(u"input terminated, waiting for output plugin to release the buffer");
                _todo.wait(lock);
            }
            // And reset the output part of the buffer.
            _out_first = 0;
            _out_count = 0;
        }

        // End of input session.
        debug(u"stopping input plugin");
        _core.inputStopped(_plugin_index, _input->stop());
    }

    debug(u"input thread terminated");
}
