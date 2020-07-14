//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsBuffer.h"
#include "tsFatal.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::Buffer::DEFAULT_SIZE;
const size_t ts::Buffer::MINIMUM_SIZE;
#endif


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::Buffer::~Buffer()
{
    if (_allocated && _buffer != nullptr) {
        delete[] _buffer;
    }
    _buffer = nullptr;
    _buffer_size = 0;
    _buffer_max = 0;
}


//----------------------------------------------------------------------------
// Check the validity of a buffer.
//----------------------------------------------------------------------------

bool ts::Buffer::isValid() const
{
    assert(_state.rbyte <= _state.wbyte);
    assert(_buffer_max <= _buffer_size);
    assert(_state.wbyte <= _buffer_max);
    assert(_state.wbyte < _buffer_max || _state.wbit == 0);
    assert(8 * _state.rbyte + _state.rbit <= 8 * _state.wbyte + _state.wbit);
    return _buffer != nullptr;
}


//----------------------------------------------------------------------------
// Read/write state constructor.
//----------------------------------------------------------------------------

ts::Buffer::RWState::RWState() :
    rbyte(0),
    wbyte(0),
    rbit(0),
    wbit(0)
{
}


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::Buffer::Buffer(size_t size) :
    _buffer(nullptr), // adjusted later
    _buffer_size(std::max(MINIMUM_SIZE, size)),
    _buffer_max(size),
    _read_only(false),
    _allocated(true),
    _big_endian(true),
    _read_error(false),
    _write_error(false),
    _user_error(false),
    _state(),
    _saved_max(),
    _saved_states(),
    _realigned()
{
    _buffer = new uint8_t[_buffer_size];
    CheckNonNull(_buffer);
}


//----------------------------------------------------------------------------
// Reset the buffer using an internal buffer.
//----------------------------------------------------------------------------

void ts::Buffer::reset(size_t size)
{
    // Deallocate previous local resources.
    if (_allocated && _buffer != nullptr && _buffer_size < size) {
        delete[] _buffer;
        _buffer = nullptr;
        _buffer_size = 0;
        _buffer_max = 0;
    }

    // Allocate the new buffer.
    if (!_allocated || _buffer == nullptr) {
        _buffer_max = size;
        _buffer_size = std::max(MINIMUM_SIZE, size);
        _buffer = new uint8_t[_buffer_size];
        CheckNonNull(_buffer);
    }

    // Reset other properties.
    _read_only = false;
    _allocated = true;
    _read_error = false;
    _write_error = false;
    _user_error = false;
    _state.rbyte = 0;
    _state.rbit = 0;
    _state.wbyte = 0;
    _state.wbit = 0;
    _saved_max.clear();
    _saved_states.clear();
}


//----------------------------------------------------------------------------
// Constructor using an external memory area.
//----------------------------------------------------------------------------

ts::Buffer::Buffer(void* data, size_t size, bool read_only) :
    _buffer(reinterpret_cast<uint8_t*>(data)),
    _buffer_size(size),
    _buffer_max(size),
    _read_only(read_only),
    _allocated(false),
    _big_endian(true),
    _read_error(false),
    _write_error(false),
    _user_error(false),
    _state(),
    _saved_max(),
    _saved_states(),
    _realigned()
{
    if (_read_only) {
        _state.wbyte = _buffer_size;
    }
}


//----------------------------------------------------------------------------
// Reset the buffer using an external memory area .
//----------------------------------------------------------------------------

void ts::Buffer::reset(void* data, size_t size, bool read_only)
{
    // Deallocate previous local resources.
    if (_allocated && _buffer != nullptr) {
        delete[] _buffer;
    }

    // Point to external buffer.
    _buffer = reinterpret_cast<uint8_t*>(data);
    _buffer_size = size;
    _buffer_max = size;
    _read_only = read_only;
    _allocated = false;

    // Reset other properties.
    _read_error = false;
    _write_error = false;
    _user_error = false;
    _state.rbyte = 0;
    _state.rbit = 0;
    _state.wbyte = _read_only ? _buffer_size : 0;
    _state.wbit = 0;
    _saved_max.clear();
    _saved_states.clear();
}


//----------------------------------------------------------------------------
// Constructor using a read-only external memory area.
//----------------------------------------------------------------------------

ts::Buffer::Buffer(const void* data, size_t size) :
    _buffer(reinterpret_cast<uint8_t*>(const_cast<void*>(data))),
    _buffer_size(size),
    _buffer_max(size),
    _read_only(true),
    _allocated(false),
    _big_endian(true),
    _read_error(false),
    _write_error(false),
    _user_error(false),
    _state(),
    _saved_max(),
    _saved_states(),
    _realigned()
{
    _state.wbyte = _buffer_size;
}


//----------------------------------------------------------------------------
// Reset the buffer using a read-only external memory area.
//----------------------------------------------------------------------------

void ts::Buffer::reset(const void* data, size_t size)
{
    // Deallocate previous local resources.
    if (_allocated && _buffer != nullptr) {
        delete[] _buffer;
    }

    // Point to external buffer.
    _buffer = reinterpret_cast<uint8_t*>(const_cast<void*>(data));
    _buffer_size = size;
    _buffer_max = size;
    _read_only = true;
    _allocated = false;

    // Reset other properties.
    _read_error = false;
    _write_error = false;
    _user_error = false;
    _state.rbyte = 0;
    _state.rbit = 0;
    _state.wbyte = _buffer_size;
    _state.wbit = 0;
    _saved_max.clear();
    _saved_states.clear();
}


//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------

ts::Buffer::Buffer(const Buffer& other) :
    _buffer(other._buffer), // adjusted later
    _buffer_size(other._buffer_size),
    _buffer_max(other._buffer_max),
    _read_only(other._read_only),
    _allocated(other._allocated),
    _big_endian(other._big_endian),
    _read_error(other._read_error),
    _write_error(other._write_error),
    _user_error(other._user_error),
    _state(other._state),
    _saved_max(other._saved_max),
    _saved_states(other._saved_states),
    _realigned()
{
    if (_buffer != nullptr && _allocated) {
        // Private internal buffer, copy resources.
        _buffer = new uint8_t[_buffer_size];
        CheckNonNull(_buffer);
        ::memcpy(_buffer, other._buffer, _buffer_size);
    }
}


//----------------------------------------------------------------------------
// Move constructor
//----------------------------------------------------------------------------

ts::Buffer::Buffer(Buffer&& other) :
    _buffer(other._buffer),
    _buffer_size(other._buffer_size),
    _buffer_max(other._buffer_max),
    _read_only(other._read_only),
    _allocated(other._allocated),
    _big_endian(other._big_endian),
    _read_error(other._read_error),
    _write_error(other._write_error),
    _user_error(other._user_error),
    _state(other._state),
    _saved_max(std::move(other._saved_max)),
    _saved_states(std::move(other._saved_states)),
    _realigned()
{
    // Clear state of moved buffer.
    other._buffer = nullptr;
    other._buffer_size = 0;
    other._buffer_max = 0;
}


//----------------------------------------------------------------------------
// Assignment operator.
//----------------------------------------------------------------------------

ts::Buffer& ts::Buffer::operator=(const Buffer& other)
{
    if (&other != this) {
        // Deallocate previous local resources.
        if (_allocated && _buffer != nullptr) {
            delete[] _buffer;
        }

        // Copy buffer properties.
        _buffer = other._buffer; // adjusted later
        _buffer_size = other._buffer_size;
        _buffer_max = other._buffer_max;
        _read_only = other._read_only;
        _allocated = other._allocated;
        _big_endian = other._big_endian;
        _read_error = other._read_error;
        _write_error = other._write_error;
        _user_error = other._user_error;
        _state = other._state;
        _saved_max = other._saved_max;
        _saved_states = other._saved_states;

        // Process buffer content.
        if (_buffer != nullptr && _allocated) {
            // Private internal buffer, copy resources.
            _buffer = new uint8_t[_buffer_size];
            CheckNonNull(_buffer);
            ::memcpy(_buffer, other._buffer, _buffer_size);
        }
    }
    return *this;
}


//----------------------------------------------------------------------------
// Move-assignment operator.
//----------------------------------------------------------------------------

ts::Buffer& ts::Buffer::operator=(Buffer&& other)
{
    if (&other != this) {
        // Deallocate previous local resources.
        if (_allocated && _buffer != nullptr) {
            delete[] _buffer;
        }

        // Move resources between buffers.
        _buffer = other._buffer;
        _buffer_size = other._buffer_size;
        _buffer_max = other._buffer_max;
        _read_only = other._read_only;
        _allocated = other._allocated;
        _big_endian = other._big_endian;
        _read_error = other._read_error;
        _user_error = other._user_error;
        _write_error = other._write_error;
        _state = other._state;
        _saved_max = std::move(other._saved_max);
        _saved_states = std::move(other._saved_states);

        // Clear state of moved buffer.
        other._buffer = nullptr;
        other._buffer_max = 0;
        other._buffer_size = 0;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Push/pop the current state of the read/write streams.
//----------------------------------------------------------------------------

size_t ts::Buffer::pushReadWriteState()
{
    _saved_states.push_back(_state);
    return _saved_states.size() - 1;
}


size_t ts::Buffer::swapReadWriteState()
{
    if (_saved_states.empty()) {
        _saved_states.push_back(_state);
    }
    else {
        std::swap(_state, _saved_states.back());
    }
    return _saved_states.size() - 1;
}

bool ts::Buffer::popReadWriteState(size_t level)
{
    if (!_saved_states.empty() && level == NPOS) {
        _state = _saved_states.back();
        _saved_states.pop_back();
        return true;
    }
    else if (level >= _saved_states.size()) {
        return false;
    }
    else {
        _state = _saved_states.at(level);
        _saved_states.resize(level);
        return true;
    }
}

bool ts::Buffer::dropReadWriteState(size_t level)
{
    if (!_saved_states.empty() && level == NPOS) {
        _saved_states.pop_back();
        return true;
    }
    else if (level >= _saved_states.size()) {
        return false;
    }
    else {
        _saved_states.resize(level);
        return true;
    }
}


//----------------------------------------------------------------------------
// Set/push/pop the buffer size.
//----------------------------------------------------------------------------

bool ts::Buffer::resize(size_t size, bool reallocate)
{
    // Get the max write pointer in saved values.
    size_t new_size = _state.wbyte + (_state.wbit + 7) / 8;
    for (auto it = _saved_states.begin(); it != _saved_states.end(); ++it) {
        new_size = std::max<size_t>(new_size, it->wbyte + (it->wbit + 7) / 8);
    }
    assert(new_size <= _buffer_size);

    // We need at least the largest saved write pointer.
    new_size = std::max(new_size, size);

    // Reallocate (enlarge or shrink) if necessary.
    if (reallocate && _allocated && new_size != _buffer_size) {
        size_t new_buffer_size = std::max(MINIMUM_SIZE, new_size);
        uint8_t* new_buffer = new uint8_t[new_buffer_size];
        CheckNonNull(new_buffer);
        if (_buffer != nullptr) {
            ::memcpy(new_buffer, _buffer, std::min(_buffer_size, new_size));
            delete[] _buffer;
        }
        _buffer = new_buffer;
        _buffer_size = new_buffer_size;
    }

    // We accept at most the physical buffer size.
    _buffer_max = std::min(new_size, _buffer_size);

    // Return success only if the requested size was granted.
    return size == _buffer_max;
}


//----------------------------------------------------------------------------
// Push/pop the buffer size.
//----------------------------------------------------------------------------

size_t ts::Buffer::pushSize(size_t size)
{
    _saved_max.push_back(_buffer_max);
    resize(size, false);
    return _saved_states.size() - 1;
}

bool ts::Buffer::popSize(size_t level)
{
    size_t size = 0;
    if (!_saved_max.empty() && level == NPOS) {
        size = _saved_max.back();
        _saved_max.pop_back();
    }
    else if (level >= _saved_max.size()) {
        return false;
    }
    else {
        size = _saved_max.at(level);
        _saved_max.resize(level);
    }
    return resize(size, size > _buffer_size);
}

bool ts::Buffer::dropSize(size_t level)
{
    if (!_saved_max.empty() && level == NPOS) {
        _saved_max.pop_back();
        return true;
    }
    else if (level >= _saved_max.size()) {
        return false;
    }
    else {
        _saved_max.resize(level);
        return true;
    }
}


//----------------------------------------------------------------------------
// Align the read or write pointer to the next byte boundary.
//----------------------------------------------------------------------------

bool ts::Buffer::readRealignByte()
{
    assert(_state.rbyte <= _state.wbyte);

    if (_state.rbit == 0) {
        // Already byte-aligned
        return true;
    }
    else if (_state.rbyte == _state.wbyte) {
        // Would go beyond write pointer
        _read_error = true;
        return false;
    }
    else {
        _state.rbyte++;
        _state.rbit = 0;
        return true;
    }
}

bool ts::Buffer::writeRealignByte(int stuffing)
{
    assert(_buffer != nullptr);
    assert(_state.wbyte <= _buffer_max);
    assert(_state.wbyte < _buffer_max || _state.wbit == 0);

    if (_read_only) {
        _write_error = true;
        return false;
    }
    if (_state.wbit != 0) {
        // Build a mask for partial byte ('1' in bits to overwrite).
        const uint8_t mask = _big_endian ? (0xFF >> _state.wbit) : uint8_t(0xFF << _state.wbit);
        if (stuffing == 0) {
            // Clear skipped bits.
            _buffer[_state.wbyte] &= ~mask;
        }
        else {
            // Set skipped bits.
            _buffer[_state.wbyte] |= mask;
        }
        _state.wbyte++;
        _state.wbit = 0;
    }
    return true;
}


//----------------------------------------------------------------------------
// Reset reading at the specified offset in the buffer.
//----------------------------------------------------------------------------

bool ts::Buffer::readSeek(size_t byte, size_t bit)
{
    assert(_state.rbyte <= _state.wbyte);

    // Forbid invalid values.
    if (bit > 7) {
        _read_error = true;
        return false;
    }

    // Forbid seeking beyond write pointer.
    if (byte > _state.wbyte || (byte == _state.wbyte && bit > _state.wbit)) {
        // Move to end of stream.
        _state.rbyte = _state.wbyte;
        _state.rbit = _state.wbit;
        _read_error = true;
        return false;
    }

    // Set read position.
    _state.rbyte = byte;
    _state.rbit = bit;
    return true;
}


//----------------------------------------------------------------------------
// Reset writing at the specified offset in the buffer.
//----------------------------------------------------------------------------

bool ts::Buffer::writeSeek(size_t byte, size_t bit)
{
    assert(_state.rbyte <= _state.wbyte);
    assert(_buffer_max <= _buffer_size);
    assert(_state.wbyte <= _buffer_max);
    assert(_state.wbyte < _buffer_max || _state.wbit == 0);

    // Forbid invalid values.
    if (_read_only || bit > 7) {
        _write_error = true;
        return false;
    }

    // Forbid seeking beyond read pointer.
    if (byte < _state.rbyte || (byte == _state.rbyte && bit < _state.rbit)) {
        // Move at read point, cannot got backward.
        _state.wbyte = _state.rbyte;
        _state.wbit = _state.rbit;
        _write_error = true;
        return false;
    }

    // Forbid seeking beyond end of buffer.
    if (byte > _buffer_max || (byte == _buffer_max && bit > 0)) {
        // Move to end of buffer.
        _state.wbyte = _buffer_max;
        _state.wbit = 0;
        _write_error = true;
        return false;
    }

    // Set write position.
    _state.wbyte = byte;
    _state.wbit = bit;
    return true;
}


//----------------------------------------------------------------------------
// Reset writing at the specified offset in the buffer and trash forward.
//----------------------------------------------------------------------------

bool ts::Buffer::writeSeek(size_t byte, size_t bit, uint8_t stuffing)
{
    if (_read_only) {
        _write_error = true;
        return false;
    }

    // Save current state for potential trash if moving forward.
    RWState prev(_state);

    // Seek to new position.
    const bool success = writeSeek(byte, bit);

    // If we seeked forward, trash memory with stuffing bytes.
    if (_state.wbyte == prev.wbyte && _state.wbit > prev.wbit) {
        setBits(_state.wbyte, prev.wbit, _state.wbit, stuffing);
    }
    else if (_state.wbyte > prev.wbyte) {
        if (prev.wbit > 0) {
            setBits(prev.wbyte, prev.wbit, 8, stuffing);
            prev.wbyte++;
        }
        ::memset(_buffer + prev.wbyte, stuffing, _state.wbyte - prev.wbyte);
        setBits(_state.wbyte, 0, _state.wbit, stuffing);
    }

    return success;
}


//----------------------------------------------------------------------------
// Internal method: Set range of bits [start_bit..end_bit[ in a byte.
//----------------------------------------------------------------------------

void ts::Buffer::setBits(size_t byte, size_t start_bit, size_t end_bit, uint8_t value)
{
    // Only if bit range is not empty.
    if (byte < _buffer_max && end_bit > start_bit) {

        // Build a mask where all addressed bits are 1.
        const uint8_t mask = _big_endian ?
            ((0xFF >> start_bit) & ~(0xFF >> end_bit)) :
            uint8_t((0xFF << start_bit) & ~(0xFF << end_bit));

        // Set range of bits.
        if (value == 0) {
            // Clear bits.
            _buffer[byte] &= ~mask;
        }
        else {
            // Set bits.
            _buffer[byte] |= mask;
        }
    }
}


//----------------------------------------------------------------------------
// Get positions and remaining space.
//----------------------------------------------------------------------------

size_t ts::Buffer::remainingReadBytes() const
{
    assert(_state.wbyte >= _state.rbyte);
    return _state.wbyte - _state.rbyte;
}

size_t ts::Buffer::remainingReadBits() const
{
    const size_t wpos = currentWriteBitOffset();
    const size_t rpos = currentReadBitOffset();
    assert(wpos >= rpos);
    return wpos - rpos;
}

size_t ts::Buffer::remainingWriteBytes() const
{
    assert(_buffer_max >= _state.wbyte);
    return _buffer_max - _state.wbyte; // ignore bit offset
}

size_t ts::Buffer::remainingWriteBits() const
{
    assert(_buffer_max > _state.wbyte || (_buffer_max == _state.wbyte && _state.wbit == 0));
    return 8 * (_buffer_max - _state.wbyte) - _state.wbit;
}


//----------------------------------------------------------------------------
// Skip read bits/bytes backward and forward.
//----------------------------------------------------------------------------

bool ts::Buffer::skipBytes(size_t bytes)
{
    if (_read_error) {
        // Can't skip bits and bytes if read error is already set.
        return false;
    }
    _state.rbit = 0;
    if (_state.rbyte + bytes > _state.wbyte) {
        _state.rbyte = _state.wbyte;
        _read_error = true;
        return false;
    }
    else {
        _state.rbyte += bytes;
        return true;
    }
}

bool ts::Buffer::skipBits(size_t bits)
{
    if (_read_error) {
        // Can't skip bits and bytes if read error is already set.
        return false;
    }
    const size_t rpos = 8 * _state.rbyte + _state.rbit + bits;
    const size_t wpos = 8 * _state.wbyte + _state.wbit;
    if (rpos > wpos) {
        _state.rbyte = _state.wbyte;
        _state.rbit = _state.wbit;
        _read_error = true;
        return false;
    }
    else {
        _state.rbyte = rpos >> 3;
        _state.rbit = rpos & 7;
        return true;
    }
}

bool ts::Buffer::backBytes(size_t bytes)
{
    if (_read_error) {
        // Can't skip bits and bytes if read error is already set.
        return false;
    }
    _state.rbit = 0;
    if (bytes > _state.rbyte) {
        _state.rbyte = 0;
        _read_error = true;
        return false;
    }
    else {
        _state.rbyte -= bytes;
        return true;
    }
}

bool ts::Buffer::backBits(size_t bits)
{
    if (_read_error) {
        // Can't skip bits and bytes if read error is already set.
        return false;
    }
    size_t rpos = 8 * _state.rbyte + _state.rbit;
    if (bits > rpos) {
        _state.rbyte = 0;
        _state.rbit = 0;
        _read_error = true;
        return false;
    }
    else {
        rpos -= bits;
        _state.rbyte = rpos >> 3;
        _state.rbit = rpos & 7;
        return true;
    }
}


//----------------------------------------------------------------------------
// Request some read size in bytes. Return actually possible read size.
//----------------------------------------------------------------------------

size_t ts::Buffer::requestReadBytes(size_t bytes)
{
    assert(_state.rbyte <= _state.wbyte);

    // Maximum possible bytes to read.
    const size_t max_bytes = _read_error ? 0 : remainingReadBits() / 8;

    if (bytes <= max_bytes) {
        return bytes;
    }
    else {
        _read_error = true;
        return max_bytes;
    }
}


//----------------------------------------------------------------------------
// Internal get bulk bytes, either aligned or not. Update read pointer.
//----------------------------------------------------------------------------

void ts::Buffer::readBytesInternal(uint8_t* data, size_t bytes)
{
    // Internal call: bytes is already validated by requestReadBytes().
    assert(_state.rbyte + bytes <= _state.wbyte);
    assert(_buffer != nullptr);

    if (_state.rbit == 0) {
        // Read pointer is byte aligned, bulk copy.
        ::memcpy(data, _buffer + _state.rbyte, bytes);
        _state.rbyte += bytes;
    }
    else {
        // Unaligned read pointer, copy small pieces.
        while (bytes > 0) {
            if (_big_endian) {
                *data++ = uint8_t(_buffer[_state.rbyte] << _state.rbit) | (_buffer[_state.rbyte + 1] >> (8 - _state.rbit));
            }
            else {
                *data++ = (_buffer[_state.rbyte] >> _state.rbit) | uint8_t(_buffer[_state.rbyte + 1] << (8 - _state.rbit));
            }
            _state.rbyte++;
            bytes--;
        }
    }
}


//----------------------------------------------------------------------------
// Public get bulk bytes.
//----------------------------------------------------------------------------

size_t ts::Buffer::getBytes(uint8_t* buffer, size_t bytes)
{
    if (buffer == nullptr) {
        return 0;
    }
    else {
        bytes = requestReadBytes(bytes);
        readBytesInternal(buffer, bytes);
        return bytes;
    }
}

ts::ByteBlock ts::Buffer::getByteBlock(size_t bytes)
{
    bytes = requestReadBytes(bytes);
    ByteBlock bb(bytes);
    readBytesInternal(bb.data(), bytes);
    return bb;
}

size_t ts::Buffer::getByteBlockAppend(ByteBlock& bb, size_t bytes)
{
    bytes = requestReadBytes(bytes);
    readBytesInternal(bb.enlarge(bytes), bytes);
    return bytes;
}


//----------------------------------------------------------------------------
// Put bulk bytes in the buffer.
//----------------------------------------------------------------------------

size_t ts::Buffer::putBytes(const ByteBlock& bb, size_t start, size_t count)
{
    start = std::min(start, bb.size());
    count = std::min(bb.size() - start, count);
    return putBytes(&bb[start], count);
}

size_t ts::Buffer::putBytes(const uint8_t* buffer, size_t bytes)
{
    assert(_state.wbyte <= _buffer_max);
    assert(_buffer != nullptr);
    assert(_state.wbit < 8);

    // Can't write on read-only or if write error already set.
    if (_read_only || _write_error) {
        _write_error = true;
        return 0;
    }

    // Actual size to write.
    if (_state.wbyte + bytes > _buffer_max) {
        bytes = _buffer_max - _state.wbyte;
        _write_error = true;
    }

    // Write bytes.
    if (_state.wbit == 0) {
        // Write pointer is byte aligned, bulk copy.
        ::memcpy(_buffer + _state.wbyte, buffer, bytes);
        _state.wbyte += bytes;
    }
    else {
        // Unaligned write pointer, copy small pieces.
        if (_state.wbyte + bytes == _buffer_max) {
            // One byte less because of partially written byte.
            assert(bytes > 0);
            bytes--;
            _write_error = true;
        }
        if (_big_endian) {
            // Clear unused bits in current partial write byte.
            _buffer[_state.wbyte] &= ~(0xFF >> _state.wbit);
            // Copy each byte in two parts.
            for (size_t i = 0; i < bytes; ++i) {
                _buffer[_state.wbyte] |= *buffer >> _state.wbit;
                _buffer[++_state.wbyte] = uint8_t(*buffer++ << (8 - _state.wbit));
            }
        }
        else {
            // Clear unused bits in current partial write byte.
            _buffer[_state.wbyte] &= ~uint8_t(0xFF << _state.wbit);
            // Copy each byte in two parts.
            for (size_t i = 0; i < bytes; ++i) {
                _buffer[_state.wbyte] |= uint8_t(*buffer << _state.wbit);
                _buffer[++_state.wbyte] = *buffer++ >> (8 - _state.wbit);
            }
        }
    }
    return bytes;
}


//----------------------------------------------------------------------------
// Read the next bit and advance the bitstream pointer.
//----------------------------------------------------------------------------

uint8_t ts::Buffer::getBit(uint8_t def)
{
    if (_read_error || endOfRead()) {
        _read_error = true;
        return def;
    }

    assert(_state.rbyte < _buffer_size);
    assert(_state.rbyte <= _state.wbyte);
    assert(_state.rbit < 8);

    const uint8_t bit = (_buffer[_state.rbyte] >> (_big_endian ? (7 - _state.rbit) : _state.rbit)) & 0x01;
    if (++_state.rbit > 7) {
        _state.rbyte++;
        _state.rbit = 0;
    }
    return bit;
}


//----------------------------------------------------------------------------
// Write the next bit and advance the write pointer.
//----------------------------------------------------------------------------

bool ts::Buffer::putBit(uint8_t bit)
{
    if (_read_only || _write_error || endOfWrite()) {
        _write_error = true;
        return false;
    }

    assert(_state.wbyte <= _buffer_max);
    assert(_state.wbit < 8);

    const uint8_t mask = uint8_t(1 << (_big_endian ? (7 - _state.wbit) : _state.wbit));
    if (bit == 0) {
        _buffer[_state.wbyte] &= ~mask;
    }
    else {
        _buffer[_state.wbyte] |= mask;
    }

    if (++_state.wbit > 7) {
        _state.wbyte++;
        _state.wbit = 0;
    }
    return true;
}


//----------------------------------------------------------------------------
// Internal "read bytes" method (1 to 8 bytes).
//----------------------------------------------------------------------------

const uint8_t* ts::Buffer::rdb(size_t bytes)
{
    // Internally used to read up to 8 bytes (64-bit integers).
    assert(bytes <= 8);

    // Static buffer for read error.
    static const uint8_t ff[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (_read_error) {
        // Read error already set, don't read anything more.
        return ff;
    }
    if (_state.rbit == 0) {
        // Read buffer is byte aligned. Most common case.
        if (_state.rbyte + bytes > _state.wbyte) {
            // Not enough bytes to read.
            _read_error = true;
            return ff;
        }
        else {
            const uint8_t* buf = _buffer + _state.rbyte;
            _state.rbyte += bytes;
            return buf;
        }
    }
    else {
        // Read buffer is not byte aligned, use an intermediate aligned buffer.
        if (currentReadBitOffset() + (8 * bytes) > currentWriteBitOffset()) {
            // Not enough bytes to read.
            _read_error = true;
            return ff;
        }
        else {
            for (uint8_t* p = _realigned; p < _realigned + bytes; p++) {
                if (_big_endian) {
                    *p = uint8_t(_buffer[_state.rbyte] << _state.rbit) | (_buffer[_state.rbyte + 1] >> (8 - _state.rbit));
                }
                else {
                    *p = (_buffer[_state.rbyte] >> _state.rbit) | uint8_t(_buffer[_state.rbyte + 1] << (8 - _state.rbit));
                }
                _state.rbyte++;
            }
            return _realigned;
        }
    }
}
