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

#include "tsMemoryPluginProxy.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsFatal.h"
#include "tsGuard.h"
#include "tsGuardCondition.h"
TSDUCK_SOURCE;

TS_DEFINE_SINGLETON(ts::MemoryPluginProxy);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::MemoryPluginProxy::MemoryPluginProxy() :
    _mutex(),
    _input_rendezvous(),
    _output_rendezvous(),
    _pull_handlers(),
    _push_handlers()
{
}

ts::MemoryPluginProxy::~MemoryPluginProxy()
{
    // The handlers are allocated and registered by the application.
    // The rendezvous, on the other hand, are internally allocated.
    // Since this is a singleton, the destructor is only called on application exit.
    // So, the deallocation are not really necessary but let's stay consistent.
    for (auto it = _input_rendezvous.begin(); it != _input_rendezvous.end(); ++it) {
        delete it->second;
    }
    for (auto it = _output_rendezvous.begin(); it != _output_rendezvous.end(); ++it) {
        delete it->second;
    }
    _input_rendezvous.clear();
    _output_rendezvous.clear();
}


//----------------------------------------------------------------------------
// Handler registration.
//----------------------------------------------------------------------------

void ts::MemoryPluginProxy::registerInputPullHandler(PortNumber port, MemoryPullHandlerInterface* handler)
{
    Guard lock(_mutex);
    _pull_handlers[port] = handler;
}

ts::MemoryPullHandlerInterface* ts::MemoryPluginProxy::getInputPullHandler(PortNumber port)
{
    Guard lock(_mutex);
    const auto it = _pull_handlers.find(port);
    return it == _pull_handlers.end() ? nullptr : it->second;
}

void ts::MemoryPluginProxy::registerOutputPushHandler(PortNumber port, MemoryPushHandlerInterface* handler)
{
    Guard lock(_mutex);
    _push_handlers[port] = handler;
}

ts::MemoryPushHandlerInterface* ts::MemoryPluginProxy::getOutputPushHandler(PortNumber port)
{
    Guard lock(_mutex);
    const auto it = _push_handlers.find(port);
    return it == _push_handlers.end() ? nullptr : it->second;
}


//----------------------------------------------------------------------------
// Get input and output rendezvous. Create if necessary, never null.
//----------------------------------------------------------------------------

ts::MemoryPluginProxy::RendezVous* ts::MemoryPluginProxy::getRendezVous(std::map<PortNumber, RendezVous*>& map, PortNumber port)
{
    Guard lock(_mutex);
    const auto it = map.find(port);
    if (it == map.end()) {
        RendezVous* rv = new RendezVous;
        CheckNonNull(rv);
        map.insert(std::make_pair(port, rv));
        return rv;
    }
    else {
        return it->second;
    }
}


//----------------------------------------------------------------------------
// Memory input plugin interface - Push mode
//----------------------------------------------------------------------------

void ts::MemoryPluginProxy::startPushInput(PortNumber port)
{
    getRendezVous(_input_rendezvous, port)->start();
}

bool ts::MemoryPluginProxy::pushInputPackets(PortNumber port, const TSPacket* packets, const TSPacketMetadata* metadata, size_t packets_count)
{
    return getRendezVous(_input_rendezvous, port)->putPackets(packets, metadata, packets_count);
}

void ts::MemoryPluginProxy::terminatePushInput(PortNumber port)
{
    getRendezVous(_input_rendezvous, port)->stop();
}

size_t ts::MemoryPluginProxy::getPushedInputPackets(PortNumber port, TSPacket* packets, TSPacketMetadata* metadata, size_t max_packets)
{
    return getRendezVous(_input_rendezvous, port)->getPackets(packets, metadata, max_packets);
}


//----------------------------------------------------------------------------
// Memory output plugin interface - Pull mode
//----------------------------------------------------------------------------

void ts::MemoryPluginProxy::startPullOutput(PortNumber port)
{
    getRendezVous(_output_rendezvous, port)->start();
}

size_t ts::MemoryPluginProxy::pullOutputPackets(PortNumber port, TSPacket* packets, TSPacketMetadata* metadata, size_t max_packets)
{
    return getRendezVous(_output_rendezvous, port)->getPackets(packets, metadata, max_packets);
}

void ts::MemoryPluginProxy::abortPullOutput(PortNumber port)
{
    getRendezVous(_output_rendezvous, port)->stop();
}

bool ts::MemoryPluginProxy::putPulledOutputPackets(PortNumber port, const TSPacket* packets, const TSPacketMetadata* metadata, size_t packets_count)
{
    return getRendezVous(_output_rendezvous, port)->putPackets(packets, metadata, packets_count);
}


//----------------------------------------------------------------------------
// Implementation of the internal rendezvous class.
//----------------------------------------------------------------------------

ts::MemoryPluginProxy::RendezVous::RendezVous() :
    _mutex(),
    _put_completed(),
    _get_completed(),
    _started(false),
    _put_packets(nullptr),
    _put_metadata(nullptr),
    _put_count(0),
    _get_packets(nullptr),
    _get_metadata(nullptr),
    _get_count(0)
{
}

void ts::MemoryPluginProxy::RendezVous::start()
{
    Guard lock(_mutex);
    _started = true;
}

void ts::MemoryPluginProxy::RendezVous::stop()
{
    Guard lock(_mutex);
    _started = false;

    // Wake up potential waiters.
    _put_completed.signal();
    _get_completed.signal();
}


//----------------------------------------------------------------------------
// Transfer packets during a rendezvous, with mutex held.
//----------------------------------------------------------------------------

bool ts::MemoryPluginProxy::RendezVous::transferPackets()
{
    const bool transfer = _started && _get_count > 0 && _put_count > 0;
    if (transfer) {
        const size_t count = std::min(_put_count, _get_count);
        if (_put_metadata != nullptr && _get_metadata != nullptr) {
            TSPacketMetadata::Copy(_get_metadata, _put_metadata, count);
            _get_metadata += count;
            _put_metadata += count;
        }
        else if (_put_metadata != nullptr) {
            _put_metadata += count;
        }
        else if (_get_metadata != nullptr) {
            _get_metadata += count;
        }
        TSPacket::Copy(_get_packets, _put_packets, count);
        _get_packets += count;
        _get_count -= count;
        _put_packets += count;
        _put_count -= count;
    }
    return transfer;
}


//----------------------------------------------------------------------------
// Synchronize on a rendezvous and put packets to getters.
//----------------------------------------------------------------------------

bool ts::MemoryPluginProxy::RendezVous::putPackets(const TSPacket* packets, const TSPacketMetadata* metadata, size_t packets_count)
{
    GuardCondition lock(_mutex, _put_completed);

    // Do we have the right to put packets?
    if (packets == nullptr || !_started || _put_packets != nullptr) {
        // Not started or some other thread already waiting on a putPackets().
        return false;
    }

    // Register ourselves as putter.
    _put_packets = packets;
    _put_metadata = metadata;
    _put_count = packets_count;

    // Loop until all our packets are read by other threads.
    while (_started && _put_count > 0) {

        // Wait until someone is willing to get some packets.
        while (_started && _get_count == 0 && _put_count > 0) {
            lock.waitCondition();
        }

        // If we can put some packets, copy them in the getter's buffer.
        if (transferPackets()) {
            // Signal to the getter that we provided something.
            _get_completed.signal();
        }
    }

    // Now unregister ourselves as putter.
    const bool success = _started && _put_count == 0; // all gone
    _put_packets = nullptr;
    _put_metadata = nullptr;
    _put_count = 0;
    return success;
}


//----------------------------------------------------------------------------
// Synchronize on a rendezvous and get packets from putters.
//----------------------------------------------------------------------------

size_t ts::MemoryPluginProxy::RendezVous::getPackets(TSPacket* packets, TSPacketMetadata* metadata, size_t max_packets)
{
    GuardCondition lock(_mutex, _get_completed);

    // Do we have the right to get packets?
    if (packets == nullptr || max_packets == 0 || !_started || _get_packets != nullptr) {
        // Not started or some other thread already waiting on a getPackets().
        return 0;
    }

    // Register ourselves as getter.
    _get_packets = packets;
    _get_metadata = metadata;
    _get_count = max_packets;

    // Loop until we get at least one packets.
    while (_started && _get_count == max_packets) {

        // Wait until someone is willing to put some packets.
        while (_started && _put_count == 0 && _get_count == max_packets) {
            lock.waitCondition();
        }

        // If we can get some packets, copy them from the putter's buffer.
        if (transferPackets()) {
            // Signal to the putter that we consumed something.
            _put_completed.signal();
        }
    }

    // Now unregister ourselves as getter.
    assert(max_packets >= _get_count);
    const size_t ret_count = max_packets - _get_count;
    _get_packets = nullptr;
    _get_metadata = nullptr;
    _get_count = 0;
    return ret_count;
}
