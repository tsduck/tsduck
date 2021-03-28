//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
#include "tsGuard.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsmux::OutputExecutor::OutputExecutor(const MuxerArgs& opt, const PluginEventHandlerRegistry& handlers, Report& log) :
    PluginExecutor(opt, handlers, PluginType::OUTPUT, opt.output, ThreadAttributes(), log),
    _output(dynamic_cast<OutputPlugin*>(plugin())),
    _mutex(),
    _to_send(),
    _to_fill(),
    _terminate(false),
    _output_first(0),
    _output_count(0),
    _packets(_opt.outBufferPackets),
    _metadata(_opt.outBufferPackets)
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
// Request the termination of the thread.
//----------------------------------------------------------------------------

void ts::tsmux::OutputExecutor::terminateOutput()
{
    // Locked the mutex on behalf of the two conditions.
    Guard lock(_mutex);
    _terminate = true;

    // Wake-up the output thread to tell it to terminate.
    _to_send.signal();

    // Wake-up any thread waiting to fill the buffer.
    _to_fill.signal();
}


//----------------------------------------------------------------------------
// Copy packets in the output buffer.
//----------------------------------------------------------------------------

bool ts::tsmux::OutputExecutor::send(const TSPacket* pkt, const TSPacketMetadata* mdata, size_t count)
{
    const size_t buffer_size = _packets.size();

    // Loop until everything is copied in the buffer or termination.
    while (!_terminate && count > 0) {

        // Loop until there is some free space in the buffer.
        GuardCondition lock(_mutex, _to_fill);
        while (!_terminate && _output_count >= buffer_size) {
            lock.waitCondition();
        }

        // Fill what can be filled in the buffer. We are still under the mutex protection.
        if (!_terminate) {
            assert(_output_count <= buffer_size);
            // Number of free packets in the buffer:
            const size_t free_size = buffer_size - _output_count;
            // End of output area, where to copy packets:
            const size_t first = (_output_first + _output_count) % buffer_size;
            // Number of contiguous packets which can be copied:
            const size_t fill_count = std::min(std::min(count, free_size), buffer_size - first);
            // Copy packets.
            TSPacket::Copy(&_packets[first], pkt, fill_count);
            TSPacketMetadata::Copy(&_metadata[first], mdata, fill_count);
            count -= fill_count;
            _output_count += fill_count;
            pkt += fill_count;
            mdata += fill_count;

            // Signal that there are some packets to send.
            // The mutex was initially locked for the _to_fill condition because we needed to wait for that condition
            // but we can also use it to signal the _to_send condition.
            _to_send.signal();
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
            GuardCondition lock(_mutex, _to_send);
            while (_output_count == 0 && !_terminate) {
                lock.waitCondition();
            }
            // We can output these packets.
            first = _output_first;
            count = _output_count;
        }

        // Output available packets.
        while (count > 0 && !_terminate) {

            // Output some packets. Not more that --max-output-packets, not more than up to end of circular buffer.
            const size_t send_count = std::min(std::min(count, _opt.maxOutputPackets), _packets.size() - _output_first);
            if (_output->send(&_packets[first], &_metadata[first], send_count)) {
                // Packets successfully sent.
                GuardCondition lock(_mutex, _to_fill);
                _output_count -= send_count;
                _output_first = (_output_first + send_count) % _packets.size();
                count -= send_count;
                first = (first + send_count) % _packets.size();
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
