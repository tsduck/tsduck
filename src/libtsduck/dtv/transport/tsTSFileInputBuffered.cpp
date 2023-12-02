//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSFileInputBuffered.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TSFileInputBuffered::TSFileInputBuffered(size_t buffer_size) :
    TSFile(),
    _buffer(std::max(buffer_size, MIN_BUFFER_SIZE)),
    _metadata(_buffer.size())
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

bool ts::TSFileInputBuffered::setBufferSize(size_t buffer_size, Report& report)
{
    if (isOpen()) {
        report.error(u"file %s is already open, cannot resize buffer", {getFileName()});
        return false;
    }
    else {
        _buffer.resize(std::max<size_t>(buffer_size, MIN_BUFFER_SIZE));
        _metadata.resize(_buffer.size());
        return true;
    }
}


//----------------------------------------------------------------------------
// Open file. Override TSFile::openRead().
//----------------------------------------------------------------------------

bool ts::TSFileInputBuffered::openRead(const fs::path& filename, size_t repeat_count, uint64_t start_offset, Report& report, TSPacketFormat format)
{
    if (isOpen()) {
        report.error(u"file %s is already open", {getFileName()});
        return false;
    }
    else {
        _first_index = 0;
        _current_offset = 0;
        _total_count = 0;
        return TSFile::openRead(filename, repeat_count, start_offset, report, format);
    }
}


//----------------------------------------------------------------------------
// Make sure that the generic open() returns an error.
//----------------------------------------------------------------------------

bool ts::TSFileInputBuffered::open(const fs::path& filename, OpenFlags flags, Report& report, TSPacketFormat format)
{
    // Accept read-only mode only.
    return (flags & (READ | WRITE | APPEND)) == READ && openRead(filename, 1, 0, report, format);
}


//----------------------------------------------------------------------------
// Return the number of read packets.
//----------------------------------------------------------------------------

ts::PacketCounter ts::TSFileInputBuffered::readPacketsCount() const
{
    // Make sure we do not report packets twice.
    return isOpen() ? TSFile::readPacketsCount() - (_total_count - _current_offset) : 0;
}


//----------------------------------------------------------------------------
// Check if we can seek to the specified absolute position
// (the "position" is the getPacketCount() value).
//----------------------------------------------------------------------------

bool ts::TSFileInputBuffered::canSeek(PacketCounter pos) const
{
    const int64_t rel = int64_t(pos) - int64_t(readPacketsCount());
    return isOpen() &&
        ((rel >= 0 && uint64_t(_current_offset) + uint64_t(rel) <= uint64_t(_total_count)) ||
         (rel < 0 && uint64_t(-rel) <= uint64_t(_current_offset)));
}


//----------------------------------------------------------------------------
// Seek to the specified absolute position, if it is inside the buffer.
//----------------------------------------------------------------------------

bool ts::TSFileInputBuffered::seek(PacketCounter pos, Report& report)
{
    if (canSeek(pos)) {
        _current_offset = size_t(int64_t(_current_offset) + int64_t(pos) - int64_t(readPacketsCount()));
        return true;
    }
    else {
        report.error(u"trying to seek buffered TS input file outside input buffer");
        return false;
    }
}


//----------------------------------------------------------------------------
// Relative seek the file inside the buffer.
//----------------------------------------------------------------------------

bool ts::TSFileInputBuffered::seekBackward(size_t packet_count, Report& report)
{
    if (!isOpen()) {
        report.error(u"file not open");
        return false;
    }
    else if (packet_count > _current_offset) {
        report.error(u"trying to seek TS input file backward too far");
        return false;
    }
    else {
        _current_offset -= packet_count;
        return true;
    }
}


bool ts::TSFileInputBuffered::seekForward(size_t packet_count, Report& report)
{
    if (!isOpen()) {
        report.error(u"file not open");
        return false;
    }
    else if (_current_offset + packet_count > _total_count) {
        report.error(u"trying to seek TS input file forward too far");
        return false;
    }
    else {
        _current_offset += packet_count;
        return true;
    }
}


//----------------------------------------------------------------------------
// Read TS packets. Override TSFile::read().
//----------------------------------------------------------------------------

size_t ts::TSFileInputBuffered::read(TSPacket* user_buffer, size_t max_packets, Report& report, TSPacketMetadata* user_metadata)
{
    if (!isOpen()) {
        report.error(u"file not open");
        return false;
    }

    const size_t buffer_size = _buffer.size();

    assert(_first_index < buffer_size);
    assert(_current_offset <= _total_count);
    assert(_total_count <= buffer_size);
    assert(_metadata.size() == buffer_size);

    // Total number of read packets (future returned value)
    size_t _in_packets = 0;

    // First, read as much packets as possible from the buffer.
    // The following loop is executed from 0 to 2 times only.
    while (_current_offset < _total_count && max_packets > 0) {
        const size_t current_index = (_first_index + _current_offset) % buffer_size;
        const size_t count = std::min(max_packets, buffer_size - current_index);
        assert(count > 0);
        TSPacket::Copy(user_buffer, &_buffer[current_index], count);
        if (user_metadata != nullptr) {
            TSPacketMetadata::Copy(user_metadata, &_metadata[current_index], count);
            user_metadata += count;
        }
        user_buffer += count;
        max_packets -= count;
        _current_offset += count;
        _in_packets += count;
    }

    // Then, read the rest directly from the file into the user's buffer.
    size_t user_count = TSFile::readPackets(user_buffer, user_metadata, max_packets, report);
    _in_packets += user_count;

    // Finally, read back the rest into our buffer. We do the exchanges that way to
    // optimize the transfer. If the number of read packets is greater than our buffer
    // size, it would be pointless to do many intermediate copies into our buffer.
    // Important: If the caller did not provide a metadata buffer, we reset the internal
    // metadata. So, if the caller requests metadata in the next call, it won't get them.
    // This means that an application shall always or never use metadata when reading a file.
    if (user_count >= buffer_size) {
        // Completely replace the buffer content.
        TSPacket::Copy(&_buffer[0], user_buffer + user_count - buffer_size, buffer_size);
        if (user_metadata != nullptr) {
            TSPacketMetadata::Copy(&_metadata[0], user_metadata + user_count - buffer_size, buffer_size);
        }
        else {
            TSPacketMetadata::Reset(&_metadata[0], buffer_size);
        }
        _first_index = 0;
        _current_offset = _total_count = buffer_size;
    }
    else if (user_count > 0) {
        // Replace part of the buffer with these user_count packets
        // First, fill the remaining free space from the buffer
        while (user_count > 0 && _total_count < buffer_size) {
            assert(_current_offset == _total_count);
            const size_t index = (_first_index + _total_count) % buffer_size;
            const size_t count = std::min(user_count, buffer_size - index);
            assert (count > 0);
            TSPacket::Copy(&_buffer[index], user_buffer, count);
            if (user_metadata != nullptr) {
                TSPacketMetadata::Copy(&_metadata[index], user_metadata, count);
                user_metadata += count;
            }
            else {
                TSPacketMetadata::Reset(&_metadata[index], count);
            }
            user_buffer += count;
            user_count -= count;
            _total_count += count;
            _current_offset += count;
        }
        // Then, override the beginning of the buffer
        while (user_count > 0) {
            // If we get there, the buffer must be full
            assert(_current_offset == buffer_size);
            assert(_total_count == buffer_size);
            const size_t count = std::min(user_count, buffer_size - _first_index);
            assert(count > 0);
            TSPacket::Copy(&_buffer[_first_index], user_buffer, count);
            if (user_metadata != nullptr) {
                TSPacketMetadata::Copy(&_metadata[_first_index], user_metadata, count);
                user_metadata += count;
            }
            else {
                TSPacketMetadata::Reset(&_metadata[_first_index], count);
            }
            user_buffer += count;
            user_count -= count;
            _first_index = (_first_index + count) % buffer_size;
        }
    }

    assert(_first_index < buffer_size);
    assert(_current_offset <= _total_count);
    assert(_total_count <= buffer_size);

    return _in_packets;
}
