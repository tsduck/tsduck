//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Transport stream file input with seekable buffer.
//
//----------------------------------------------------------------------------

#include "tsTSFileInputBuffered.h"
TSDUCK_SOURCE;

#if defined (TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::TSFileInputBuffered::MIN_BUFFER_SIZE;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TSFileInputBuffered::TSFileInputBuffered (size_t buffer_size) :
    TSFileInput (),
    _buffer (std::max<size_t> (buffer_size, MIN_BUFFER_SIZE)),
    _first_index (0),
    _current_offset (0),
    _total_count (0)
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::TSFileInputBuffered::~TSFileInputBuffered()
{
}


//----------------------------------------------------------------------------
// Set the buffer size. Can be done only when the file is closed.
//----------------------------------------------------------------------------

bool ts::TSFileInputBuffered::setBufferSize (size_t buffer_size, ReportInterface& report)
{
    if (isOpen()) {
        report.error ("file " + getFileName() + " is already open, cannot resize buffer");
        return false;
    }
    else {
        _buffer.resize (std::max<size_t> (buffer_size, MIN_BUFFER_SIZE));
        return true;
    }
}


//----------------------------------------------------------------------------
// Open file. Override TSFileInput::open().
//----------------------------------------------------------------------------

bool ts::TSFileInputBuffered::open (const std::string& filename, size_t repeat_count, uint64_t start_offset, ReportInterface& report)
{
    if (isOpen()) {
        report.error ("file " + getFileName() + " is already open");
        return false;
    }
    else {
        _first_index = 0;
        _current_offset = 0;
        _total_count = 0;
        return TSFileInput::open (filename, repeat_count, start_offset, report);
    }
}

//----------------------------------------------------------------------------
// Return the number of read packets. Override TSFileInput::getPacketCount().
//----------------------------------------------------------------------------

ts::PacketCounter ts::TSFileInputBuffered::getPacketCount() const
{
    // Make sure we do not report packets twice.
    return isOpen() ? TSFileInput::getPacketCount() - (_total_count - _current_offset) : 0;
}


//----------------------------------------------------------------------------
// Check if we can seek to the specified absolute position
// (the "position" is the getPacketCount() value).
//----------------------------------------------------------------------------

bool ts::TSFileInputBuffered::canSeek (PacketCounter pos) const
{
    const int64_t rel = int64_t (pos) - int64_t (getPacketCount());
    return isOpen() &&
        ((rel >= 0 && uint64_t (_current_offset) + uint64_t (rel) <= uint64_t (_total_count)) ||
         (rel < 0 && uint64_t (-rel) <= uint64_t (_current_offset)));
}


//----------------------------------------------------------------------------
// Seek to the specified absolute position, if it is inside the buffer.
//----------------------------------------------------------------------------

bool ts::TSFileInputBuffered::seek (PacketCounter pos, ReportInterface& report)
{
    if (canSeek (pos)) {
        _current_offset = size_t (int64_t (_current_offset) + int64_t (pos) - int64_t (getPacketCount()));
        return true;
    }
    else {
        report.error ("trying to seek buffered TS input file outside input buffer");
        return false;
    }
}


//----------------------------------------------------------------------------
// Relative seek the file inside the buffer.
//----------------------------------------------------------------------------

bool ts::TSFileInputBuffered::seekBackward (size_t packet_count, ReportInterface& report)
{
    if (!isOpen()) {
        report.error ("file not open");
        return false;
    }
    else if (packet_count > _current_offset) {
        report.error ("trying to seek TS input file backward too far");
        return false;
    }
    else {
        _current_offset -= packet_count;
        return true;
    }
}


bool ts::TSFileInputBuffered::seekForward (size_t packet_count, ReportInterface& report)
{
    if (!isOpen()) {
        report.error ("file not open");
        return false;
    }
    else if (_current_offset + packet_count > _total_count) {
        report.error ("trying to seek TS input file forward too far");
        return false;
    }
    else {
        _current_offset += packet_count;
        return true;
    }
}


//----------------------------------------------------------------------------
// Read TS packets. Override TSFileInput::read().
//----------------------------------------------------------------------------

size_t ts::TSFileInputBuffered::read (TSPacket* user_buffer, size_t max_packets, ReportInterface& report)
{
    if (!isOpen()) {
        report.error ("file not open");
        return false;
    }

    const size_t buffer_size = _buffer.size();

    assert (_first_index < buffer_size);
    assert (_current_offset <= _total_count);
    assert (_total_count <= buffer_size);

    // Total number of read packets (future returned value)
    size_t _in_packets = 0;

    // First, read as much packets as possible from the buffer.
    // The following loop is executed from 0 to 2 times only.
    while (_current_offset < _total_count && max_packets > 0) {
        const size_t current_index = (_first_index + _current_offset) % buffer_size;
        const size_t count = std::min (max_packets, buffer_size - current_index);
        assert (count > 0);
        ::memcpy (user_buffer, &_buffer[current_index], count * PKT_SIZE);
        user_buffer += count;
        max_packets -= count;
        _current_offset += count;
        _in_packets += count;
    }

    // Then, read the rest directly from the file into the user's buffer.
    size_t user_count = TSFileInput::read (user_buffer, max_packets, report);
    _in_packets += user_count;

    // Finally, read back the rest into our buffer. We do the exchanges that way
    // to optimize the transfer. If the number of read packets is greater than
    // our buffer size, it would be pointless to do many intermediate copies
    // into our buffer.
    if (user_count >= buffer_size) {
        // Completely replace the buffer content.
        ::memcpy (&_buffer[0], user_buffer + user_count - buffer_size, buffer_size * PKT_SIZE);
        _first_index = 0;
        _current_offset = _total_count = buffer_size;
    }
    else if (user_count > 0) {
        // Replace part of the buffer with these user_count packets
        // First, fill the remaining free space from the buffer
        while (user_count > 0 && _total_count < buffer_size) {
            assert (_current_offset == _total_count);
            const size_t index = (_first_index + _total_count) % buffer_size;
            const size_t count = std::min (user_count, buffer_size - index);
            assert (count > 0);
            ::memcpy (&_buffer[index], user_buffer, count * PKT_SIZE);
            user_buffer += count;
            user_count -= count;
            _total_count += count;
            _current_offset += count;
        }
        // Then, override the beginning of the buffer
        while (user_count > 0) {
            // If we get there, the buffer must be full
            assert (_current_offset == buffer_size);
            assert (_total_count == buffer_size);
            const size_t count = std::min (user_count, buffer_size - _first_index);
            assert (count > 0);
            ::memcpy (&_buffer[_first_index], user_buffer, count * PKT_SIZE);
            user_buffer += count;
            user_count -= count;
            _first_index = (_first_index + count) % buffer_size;
        }
    }

    assert (_first_index < buffer_size);
    assert (_current_offset <= _total_count);
    assert (_total_count <= buffer_size);

    return _in_packets;
}
