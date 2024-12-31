//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstsmuxOutputExecutor.h"


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
        std::unique_lock<std::recursive_mutex> lock(_mutex);
        _got_freespace.wait(lock, [this]() { return _terminate || _packets_count < _buffer_size; });

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
            _got_packets.notify_one();
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
            std::unique_lock<std::recursive_mutex> lock(_mutex);
            _got_packets.wait(lock, [this]() { return _packets_count > 0 || _terminate; });
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
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                _packets_count -= send_count;
                _packets_first = (_packets_first + send_count) % _buffer_size;
                count -= send_count;
                first = (first + send_count) % _buffer_size;
                // Signal that there are some free space in the buffer.
                _got_freespace.notify_one();
            }
            else if (_opt.outputOnce) {
                // Terminates when the output plugin fails.
                _terminate = true;
            }
            else {
                // Restart when the plugin fails.
                verbose(u"restarting output plugin '%s' after failure", pluginName());
                _output->stop();
                while (!_terminate && !_output->start()) {
                    std::this_thread::sleep_for(_opt.outputRestartDelay);
                }
            }
        }
    }

    // Stop the plugin.
    _output->stop();
    debug(u"output thread terminated");
}
