//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPushInputPlugin.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PushInputPlugin::PushInputPlugin(TSP* tsp_, const UString& description, const UString& syntax) :
    InputPlugin(tsp_, description, syntax),
    _receiver(this)
{
}

ts::PushInputPlugin::~PushInputPlugin()
{
    _receiver.waitForTermination();
}

ts::PushInputPlugin::Receiver::Receiver(PushInputPlugin* plugin) :
    _plugin(plugin)
{
}

ts::PushInputPlugin::Receiver::~Receiver()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// Tune the TS packet buffer.
//----------------------------------------------------------------------------

void ts::PushInputPlugin::setQueueSize(size_t count)
{
    _queue.reset(count);
}


//----------------------------------------------------------------------------
// Internal thread which receives TS packets.
//----------------------------------------------------------------------------

void ts::PushInputPlugin::Receiver::main()
{
    _plugin->tsp->debug(u"internal push-input thread started");

    // Simply let the subclass perform input until the end.
    _plugin->processInput();

    // Push an end of file mark.
    _plugin->_queue.setEOF();

    _plugin->tsp->debug(u"internal push-input thread completed");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PushInputPlugin::start()
{
    if (_started) {
        return false; // already started
    }
    else {
        // Reset the packet queue to restart a new session (in case of restart).
        _queue.reset();
        _interrupted = false;
        return true;
    }
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PushInputPlugin::stop()
{
    // Send the stop condition to the internal packet queue.
    _queue.stop();

    // Wait for received thread termination.
    _receiver.waitForTermination();
    _started = false;
    return true;
}


//----------------------------------------------------------------------------
// Abort input operation in progress
//----------------------------------------------------------------------------

bool ts::PushInputPlugin::abortInput()
{
    // Send the stop condition to the internal packet queue.
    _queue.stop();
    return true;
}


//----------------------------------------------------------------------------
// Standard input routine, now hidden from subclasses.
//----------------------------------------------------------------------------

size_t ts::PushInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    // Start the receiver thread the first time.
    if (!_started) {
        _receiver.setAttributes(ThreadAttributes().setStackSize(stackUsage()));
        if (!_receiver.start()) {
            return false;
        }
        _started = true;
    }

    size_t count = 0;
    BitRate bitrate = 0;

    // Wait for some packets from the receiver thread.
    if (!_queue.waitPackets(buffer, max_packets, count, bitrate)) {
        // End of input.
        count = 0;
    }

    assert(count <= max_packets);
    return count;
}


//----------------------------------------------------------------------------
// Push packet to the tsp chain.
//----------------------------------------------------------------------------

bool ts::PushInputPlugin::pushPackets(const TSPacket* buffer, size_t count)
{
    // We are executing in the context of the receiver thread.
    // Send packets by chunks, loop until everything is pushed.
    while (count > 0) {

        TSPacket* out_buffer = nullptr;
        size_t out_count = 0;

        // Abort now if the application is terminating.
        if (tsp->aborting() || _queue.stopped()) {
            _interrupted = true;
            return false;
        }

        // Wait for space in the queue buffer.
        if (!_queue.lockWriteBuffer(out_buffer, out_count, count)) {
            return false;
        }

        assert(out_buffer != nullptr);
        assert(out_count > 0);

        // Move packets into the queue.
        if (out_count > count) {
            out_count = count;
        }
        TSPacket::Copy(out_buffer, buffer, out_count);
        buffer += out_count;
        count -= out_count;

        // Signal the new packets in the queue.
        _queue.releaseWriteBuffer(out_count);
    }

    return true;
}
