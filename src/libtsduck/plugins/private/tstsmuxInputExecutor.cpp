//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstsmuxInputExecutor.h"
#include "tsGuardMutex.h"
#include "tsGuardCondition.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsmux::InputExecutor::InputExecutor(const MuxerArgs& opt, const PluginEventHandlerRegistry& handlers, size_t index, Report& log) :
    // Input threads have a high priority to be always ready to load incoming packets in the buffer.
    PluginExecutor(opt, handlers, PluginType::INPUT, opt.inputs[index], ThreadAttributes().setPriority(ThreadAttributes::GetHighPriority()), log),
    _input(dynamic_cast<InputPlugin*>(PluginThread::plugin())),
    _pluginIndex(index)
{
    // Make sure that the input plugins display their index.
    setLogName(UString::Format(u"%s[%d]", {pluginName(), _pluginIndex}));
}

ts::tsmux::InputExecutor::~InputExecutor()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// Implementation of TSP.
//----------------------------------------------------------------------------

size_t ts::tsmux::InputExecutor::pluginIndex() const
{
    return _pluginIndex;
}


//----------------------------------------------------------------------------
// Terminate input, also abort input in progress when possible.
//----------------------------------------------------------------------------

void ts::tsmux::InputExecutor::terminate()
{
    // Signal termination.
    PluginExecutor::terminate();

    // Then abort input in progress if there is one to avoid blocking.
    _input->abortInput();
}


//----------------------------------------------------------------------------
// Copy packets from the input buffer.
//----------------------------------------------------------------------------

bool ts::tsmux::InputExecutor::getPackets(TSPacket* pkt, TSPacketMetadata* mdata, size_t max_count, size_t& ret_count, bool blocking)
{
    // In blocking mode, loop until there is some packet in the buffer.
    GuardCondition lock(_mutex, _got_packets);
    while (!_terminate && blocking && _packets_count == 0) {
        lock.waitCondition();
    }

    // Return error if the input is terminated _and_ there is no more packet to read.
    if (_terminate && _packets_count == 0) {
        ret_count = 0;
        return false;
    }

    // Fill what can be filled from the buffer. We are still under the mutex protection.
    assert(_packets_count <= _buffer_size);

    // Number of packets to copy from the buffer:
    ret_count = std::min(std::min(max_count, _packets_count), _buffer_size - _packets_first);

    // Copy packets if there are some.
    if (ret_count > 0) {
        TSPacket::Copy(pkt, &_packets[_packets_first], ret_count);
        TSPacketMetadata::Copy(mdata, &_metadata[_packets_first], ret_count);
        _packets_first = (_packets_first + ret_count) % _buffer_size;
        _packets_count -= ret_count;

        // Signal that there are some free space.
        // The mutex was initially locked for the _got_packets condition because we needed to wait
        // for that condition but we can also use it to signal the _got_freespace condition.
        _got_freespace.signal();
    }
    return true;
}


//----------------------------------------------------------------------------
// Invoked in the context of the plugin thread.
//----------------------------------------------------------------------------

void ts::tsmux::InputExecutor::main()
{
    debug(u"input thread started");

    // Loop until we are instructed to stop.
    while (!_terminate) {

        // Wait for free space to be available in the input buffer.
        size_t first = 0;
        size_t count = 0;
        {
            GuardCondition lock(_mutex, _got_freespace);
            // In case of lossy input, drop oldest packets when the buffer is full.
            if (_opt.lossyInput && _packets_count >= _buffer_size) {
                const size_t dropped = std::min(_opt.lossyReclaim, _buffer_size);
                _packets_first = (_packets_first + dropped) % _buffer_size;
                _packets_count -= dropped;
            }
            // Wait for free space in the buffer.
            while (!_terminate && _packets_count >= _buffer_size) {
                lock.waitCondition();
            }
            // We can use this contiguous free area at the end of already received packets.
            first = (_packets_first + _packets_count) % _buffer_size;
            count = std::min(_buffer_size - _packets_count, _buffer_size - first);
        }

        // Read some packets.
        if (!_terminate) {
            count = _input->receive(&_packets[first], &_metadata[first], std::min(count, _opt.maxInputPackets));
            if (count > 0) {
                // Packets successfully received.
                GuardCondition lock(_mutex, _got_packets);
                _packets_count += count;
                // Signal that there are some new packets in the buffer.
                lock.signal();
            }
            else if (_opt.inputOnce) {
                // Terminates when the input plugin terminates or fails.
                _terminate = true;
            }
            else {
                // Restart when the plugin terminates or fails.
                verbose(u"restarting input plugin '%s' after end of stream or failure", {pluginName()});
                _input->stop();
                while (!_terminate && !_input->start()) {
                    SleepThread(_opt.outputRestartDelay);
                }
            }
        }
    }

    // Stop the plugin.
    _input->stop();
    debug(u"input thread terminated");
}
