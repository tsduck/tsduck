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
