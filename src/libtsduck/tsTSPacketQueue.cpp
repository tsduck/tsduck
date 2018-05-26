//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsTSPacketQueue.h"
#include "tsGuard.h"
#include "tsGuardCondition.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TSPacketQueue::TSPacketQueue(size_t size) :
    _eof(false),
    _stopped(false),
    _mutex(),
    _freed(),
    _buffer(size),
    _pcr(1, 12),
    _inCount(0),
    _readIndex(0),
    _writeIndex(0),
    _bitrate(0)
{
}


//----------------------------------------------------------------------------
// Reset and resize the buffer.
//----------------------------------------------------------------------------

void ts::TSPacketQueue::reset(size_t size)
{
    Guard lock(_mutex);

    // Refuse to shrink too much. Keep at least one packet.
    _buffer.resize(std::max<size_t>(size, 1));

    _eof = false;
    _stopped = false;
    _inCount = 0;
    _readIndex = 0;
    _writeIndex = 0;
    _bitrate = 0;
}


//----------------------------------------------------------------------------
// Get the size of the buffer in packets.
//----------------------------------------------------------------------------

size_t ts::TSPacketQueue::bufferSize() const
{
    Guard lock(_mutex);
    return _buffer.size();
}


//----------------------------------------------------------------------------
// Called by the writer thread to get a write buffer.
//----------------------------------------------------------------------------

bool ts::TSPacketQueue::lockWriteBuffer(TSPacket*& buffer, size_t& buffer_size, size_t min_size)
{
    GuardCondition lock(_mutex, _freed);

    // Maximum size we can allocate to the write window.
    assert(_readIndex < _buffer.size());
    assert(_writeIndex < _buffer.size());
    const size_t max_size = _buffer.size() - _writeIndex;

    // We cannot ask for more than the distance to the end of the buffer.
    // But we also need to wait for at least one packet.
    min_size = std::max<size_t>(1, std::min(min_size, max_size));

    // Wait until we get enough free space.
    while (!_stopped && _buffer.size() - _inCount < min_size) {
        lock.waitCondition();
    }

    // Return the write window.
    buffer = &_buffer[_writeIndex];
    if (_stopped) {
        // The reader thread has reported a stop condition, we can no longer write into the buffer.
        buffer_size = 0;
    }
    else if (_readIndex > _writeIndex) {
        // The write window extends up to the read index (where packets were not yet consumed).
        buffer_size = _readIndex - _writeIndex;
    }
    else {
        // The write window wraps up at the end of the buffer.
        // Return only the first contiguous part of the write window.
        buffer_size = max_size;
    }

    // A write buffer is returned only when the reader thread does not want to terminate.
    return !_stopped;
}


//----------------------------------------------------------------------------
// Called by the writer thread to release the write buffer.
//----------------------------------------------------------------------------

void ts::TSPacketQueue::releaseWriteBuffer(size_t count)
{
    Guard lock(_mutex);

    // Verify that the specified size is compatible with the current write window.
    assert(_readIndex < _buffer.size());
    assert(_writeIndex < _buffer.size());
    const size_t max_count = (_readIndex > _writeIndex ? _readIndex : _buffer.size()) - _writeIndex;

    // This is a bug in the application to specify more than the max size.
    assert(count <= max_count);

    // When assertions are disabled, simply reduce.
    if (count > max_count) {
        count = max_count;
    }

    // When the writer thread did not specify a bitrate, analyze PCR's.
    if (_bitrate == 0) {
        for (size_t i = 0; i < count; ++i) {
            _pcr.feedPacket(_buffer[_writeIndex + i]);
        }
    }

    // Mark written packets as part of the buffer.
    _inCount += count;
    _writeIndex = (_writeIndex + count) % _buffer.size();
}


//----------------------------------------------------------------------------
// Called by the writer thread to report the input bitrate.
//----------------------------------------------------------------------------

void ts::TSPacketQueue::setBitrate(BitRate bitrate)
{
    Guard lock(_mutex);

    // Remember the bitrate value.
    _bitrate = bitrate;

    // If a specific value is given, reset PCR analysis.
    if (bitrate > 0) {
        _pcr.reset();
    }
}


//----------------------------------------------------------------------------
// Called by the writer thread to report the end of input thread.
//----------------------------------------------------------------------------

void ts::TSPacketQueue::setEOF()
{
    // No explicit synchronization required here because the reader thread is never suspended.
    _eof = true;
}


//----------------------------------------------------------------------------
// Called by the reader thread to get the next packet.
//----------------------------------------------------------------------------

bool ts::TSPacketQueue::getPacket(TSPacket& packet, BitRate& bitrate)
{
    GuardCondition lock(_mutex, _freed);

    // Get bitrate, either from reader thread or from PCR analysis.
    if (_bitrate != 0) {
        bitrate = _bitrate;
    }
    else if (_pcr.bitrateIsValid()) {
        bitrate = _pcr.bitrate188();
    }
    else {
        bitrate = 0;
    }

    // Get packet when available.
    if (_inCount == 0) {
        // No packet available.
        return false;
    }
    else {
        // Return next packet.
        packet = _buffer[_readIndex];
        _readIndex = (_readIndex + 1) % _buffer.size();
        _inCount--;

        // Signal the condition that a packet was freed.
        lock.signal();

        return true;
    }
}


//----------------------------------------------------------------------------
// Called by the reader thread to tell the writer thread to stop immediately.
//----------------------------------------------------------------------------

void ts::TSPacketQueue::stop()
{
    GuardCondition lock(_mutex, _freed);

    // Report a stop condition.
    _stopped = true;

    // Signal the condition that a packet was freed. This is not really freeing
    // a packet but it means that the writer thread should wake up.
    lock.signal();
}
