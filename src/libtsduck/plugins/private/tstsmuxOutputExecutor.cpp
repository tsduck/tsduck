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

#include "tstsmuxOutputExecutor.h"
#include "tsGuardCondition.h"
#include "tsGuardMutex.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsmux::OutputExecutor::OutputExecutor(const MuxerArgs& opt, const PluginEventHandlerRegistry& handlers, Report& log) :
    PluginExecutor(opt, handlers, PluginType::OUTPUT, opt.output, ThreadAttributes(), log),
    _output(dynamic_cast<OutputPlugin*>(plugin()))
{
}

ts::tsmux::OutputExecutor::~OutputExecutor()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// Implementation of TSP.
//----------------------------------------------------------------------------

size_t ts::tsmux::OutputExecutor::pluginIndex() const
{
    // The output plugin comes last.
    return _opt.inputs.size();
}


//----------------------------------------------------------------------------
// Copy packets in the output buffer.
//----------------------------------------------------------------------------

bool ts::tsmux::OutputExecutor::send(const TSPacket* pkt, const TSPacketMetadata* mdata, size_t count)
{
    // Loop until everything is copied in the buffer or termination.
    while (!_terminate && count > 0) {

        // Loop until there is some free space in the buffer.
        GuardCondition lock(_mutex, _got_freespace);
        while (!_terminate && _packets_count >= _buffer_size) {
            lock.waitCondition();
        }

        // Fill what can be filled in the buffer. We are still under the mutex protection.
        if (!_terminate) {
            assert(_packets_count <= _buffer_size);
            // Number of free packets in the buffer:
            const size_t free_size = _buffer_size - _packets_count;
            // End of output area, where to copy packets:
            const size_t copy_first = (_packets_first + _packets_count) % _buffer_size;
            // Number of contiguous packets which can be copied:
            const size_t fill_count = std::min(std::min(count, free_size), _buffer_size - copy_first);
            // Copy packets.
            TSPacket::Copy(&_packets[copy_first], pkt, fill_count);
            TSPacketMetadata::Copy(&_metadata[copy_first], mdata, fill_count);
            count -= fill_count;
            _packets_count += fill_count;
            pkt += fill_count;
            mdata += fill_count;

            // Signal that there are some packets to send.
            // The mutex was initially locked for the _got_freespace condition because we needed to wait
            // for that condition but we can also use it to signal the _got_packets condition.
            _got_packets.signal();
        }
    }
    return !_terminate;
}


//----------------------------------------------------------------------------
// Invoked in the context of the output plugin thread.
//----------------------------------------------------------------------------

void ts::tsmux::OutputExecutor::main()
{
    debug(u"output thread started");

    // Loop until we are instructed to stop.
    while (!_terminate) {

        // Wait for packets to be available in the output buffer.
        size_t first = 0;
        size_t count = 0;
        {
            GuardCondition lock(_mutex, _got_packets);
            while (_packets_count == 0 && !_terminate) {
                lock.waitCondition();
            }
            // We can output these packets.
            first = _packets_first;
            count = _packets_count;
        }

        // Output available packets.
        while (count > 0 && !_terminate) {

            // Output some packets. Not more that --max-output-packets, not more than up to end of circular buffer.
            const size_t send_count = std::min(std::min(count, _opt.maxOutputPackets), _buffer_size - _packets_first);
            if (_output->send(&_packets[first], &_metadata[first], send_count)) {
                // Packets successfully sent.
                GuardCondition lock(_mutex, _got_freespace);
                _packets_count -= send_count;
                _packets_first = (_packets_first + send_count) % _buffer_size;
                count -= send_count;
                first = (first + send_count) % _buffer_size;
                // Signal that there are some free space in the buffer.
                lock.signal();
            }
            else if (_opt.outputOnce) {
                // Terminates when the output plugin fails.
                _terminate = true;
            }
            else {
                // Restart when the plugin fails.
                verbose(u"restarting output plugin '%s' after failure", {pluginName()});
                _output->stop();
                while (!_terminate && !_output->start()) {
                    SleepThread(_opt.outputRestartDelay);
                }
            }
        }
    }

    // Stop the plugin.
    _output->stop();
    debug(u"output thread terminated");
}
