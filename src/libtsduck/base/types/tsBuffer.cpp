//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBuffer.h"
#include "tsFatal.h"


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
    _state.clear();
}


//----------------------------------------------------------------------------
// Check the validity of a buffer.
//----------------------------------------------------------------------------

bool ts::Buffer::isValid() const
{
    assert(_state.rbyte <= _state.wbyte);
    assert(_state.end <= _buffer_size);
    assert(_state.wbyte <= _state.end);
    assert(_state.wbyte < _state.end || _state.wbit == 0);
    assert(8 * _state.rbyte + _state.rbit <= 8 * _state.wbyte + _state.wbit);
    return _buffer != nullptr;
}


//----------------------------------------------------------------------------
// Read/write state constructor.
//----------------------------------------------------------------------------

ts::Buffer::State::State(bool rdonly, size_t size) :
    read_only(rdonly),
    end(size)
{
}

void ts::Buffer::State::clear()
{
    read_only = false;
    end = rbyte = wbyte = rbit = wbit = len_bits = 0;
}


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::Buffer::Buffer(size_t size) :
    _buffer(nullptr), // adjusted later
    _buffer_size(std::max(MINIMUM_SIZE, size)),
    _allocated(true),
    _state(false, size)
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
    }

    // Allocate the new buffer.
    if (!_allocated || _buffer == nullptr) {
        _buffer_size = std::max(MINIMUM_SIZE, size);
        _buffer = new uint8_t[_buffer_size];
        CheckNonNull(_buffer);
    }

    // Reset other properties.
    _allocated = true;
    _read_error = false;
    _write_error = false;
    _user_error = false;
    _state.read_only = false;
    _state.rbyte = 0;
    _state.rbit = 0;
    _state.wbyte = 0;
    _state.wbit = 0;
    _state.end = size;
    _saved_states.clear();
    _reserved_bits_errors.clear();
}


//----------------------------------------------------------------------------
// Constructor using an external memory area.
//----------------------------------------------------------------------------

ts::Buffer::Buffer(void* data, size_t size, bool read_only) :
    _buffer(reinterpret_cast<uint8_t*>(data)),
    _buffer_size(size),
    _allocated(false),
    _state(read_only, size)
{
    if (_state.read_only) {
        _state.wbyte = _state.end;
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
    _allocated = false;

    // Reset other properties.
    _read_error = false;
    _write_error = false;
    _user_error = false;
    _state.read_only = read_only;
    _state.end = size;
    _state.rbyte = 0;
    _state.rbit = 0;
    _state.wbyte = _state.read_only ? _state.end : 0;
    _state.wbit = 0;
    _saved_states.clear();
    _reserved_bits_errors.clear();
}


//----------------------------------------------------------------------------
// Constructor using a read-only external memory area.
//----------------------------------------------------------------------------

ts::Buffer::Buffer(const void* data, size_t size) :
    _buffer(reinterpret_cast<uint8_t*>(const_cast<void*>(data))),
    _buffer_size(size),
    _allocated(false),
    _state(true, size)
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
    _allocated = false;

    // Reset other properties.
    _read_error = false;
    _write_error = false;
    _user_error = false;
    _state.read_only = true;
    _state.rbyte = 0;
    _state.rbit = 0;
    _state.end = _state.wbyte = _buffer_size;
    _state.wbit = 0;
    _saved_states.clear();
    _reserved_bits_errors.clear();
}


//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------

ts::Buffer::Buffer(const Buffer& other) :
    _buffer(other._buffer), // adjusted later
    _buffer_size(other._buffer_size),
    _allocated(other._allocated),
    _big_endian(other._big_endian),
    _read_error(other._read_error),
    _write_error(other._write_error),
    _user_error(other._user_error),
    _state(other._state),
    _saved_states(other._saved_states),
    _realigned(),
    _reserved_bits_errors(other._reserved_bits_errors)
{
    if (_buffer != nullptr && _allocated) {
        // Private internal buffer, copy resources.
        _buffer = new uint8_t[_buffer_size];
        CheckNonNull(_buffer);
        std::memcpy(_buffer, other._buffer, _buffer_size);
    }
}


//----------------------------------------------------------------------------
// Move constructor
//----------------------------------------------------------------------------

ts::Buffer::Buffer(Buffer&& other) :
    _buffer(other._buffer),
    _buffer_size(other._buffer_size),
    _allocated(other._allocated),
    _big_endian(other._big_endian),
    _read_error(other._read_error),
    _write_error(other._write_error),
    _user_error(other._user_error),
    _state(other._state),
    _saved_states(std::move(other._saved_states)),
    _realigned(),
    _reserved_bits_errors(std::move(other._reserved_bits_errors))
{
    // Clear state of moved buffer.
    other._buffer = nullptr;
    other._buffer_size = 0;
    other._state.clear();
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
        _allocated = other._allocated;
        _big_endian = other._big_endian;
        _read_error = other._read_error;
        _write_error = other._write_error;
        _user_error = other._user_error;
        _state = other._state;
        _saved_states = other._saved_states;
        _reserved_bits_errors = other._reserved_bits_errors;

        // Process buffer content.
        if (_buffer != nullptr && _allocated) {
            // Private internal buffer, copy resources.
            _buffer = new uint8_t[_buffer_size];
            CheckNonNull(_buffer);
            std::memcpy(_buffer, other._buffer, _buffer_size);
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
        _allocated = other._allocated;
        _big_endian = other._big_endian;
        _read_error = other._read_error;
        _user_error = other._user_error;
        _write_error = other._write_error;
        _state = other._state;
        _saved_states = std::move(other._saved_states);
        _reserved_bits_errors = std::move(other._reserved_bits_errors);

        // Clear state of moved buffer.
        other._buffer = nullptr;
        other._buffer_size = 0;
        other._state.clear();
    }
    return *this;
}


//----------------------------------------------------------------------------
// Push the current state of the read/write streams.
//----------------------------------------------------------------------------

size_t ts::Buffer::pushState()
{
    _saved_states.push_back(_state);
    _saved_states.back().reason = Reason::FULL;
    return _saved_states.size() - 1;
}


//----------------------------------------------------------------------------
// Temporary reduce the new readable size of the buffer.
//----------------------------------------------------------------------------

size_t ts::Buffer::pushReadSize(size_t size)
{
    // Save current state.
    _saved_states.push_back(_state);
    _saved_states.back().reason = Reason::READ_SIZE;

    // Adjust the new write pointer and make the buffer read-only.
    _state.wbyte = std::max(_state.rbyte, std::min(size, _state.wbyte));
    _state.read_only = true;

    return _saved_states.size() - 1;
}


//----------------------------------------------------------------------------
// Temporary reduce the writable size of the buffer.
//----------------------------------------------------------------------------

size_t ts::Buffer::pushWriteSize(size_t size)
{
    // Save current state.
    _saved_states.push_back(_state);
    _saved_states.back().reason = Reason::WRITE_SIZE;

    // Adjust the new end of buffer.
    _state.end = std::max(_state.wbyte, std::min(size, _state.end));

    return _saved_states.size() - 1;
}


//----------------------------------------------------------------------------
// Start a write sequence with a leading length field.
//----------------------------------------------------------------------------

size_t ts::Buffer::pushWriteSequenceWithLeadingLength(size_t length_bits)
{
    // Check parameters. Must be byte-aligned after the length field.
    if (_state.read_only || _write_error || length_bits == 0 || length_bits > 64 || (_state.wbit + length_bits) % 8 != 0) {
        return NPOS;
    }

    // Save current state.
    _saved_states.push_back(_state);
    _saved_states.back().reason = Reason::WRITE_LEN_SEQ;
    _saved_states.back().len_bits = length_bits;

    // Write a zero place holder for the length field.
    putBits(0, length_bits);

    return _saved_states.size() - 1;
}



//----------------------------------------------------------------------------
// Pop the current state from the stack and perform appropriate actions.
//----------------------------------------------------------------------------

bool ts::Buffer::popState(size_t level)
{
    if (_saved_states.empty()) {
        // Nothing to restore.
        return false;
    }
    else if (level == NPOS) {
        // Default level is last level.
        level = _saved_states.size() - 1;
    }
    else if (level >= _saved_states.size()) {
        // Non-existent level.
        return false;
    }

    // We need to pop the states one by one since some actions may be performed at each level.
    while (_saved_states.size() > level) {
        // Reference to last saved state.
        const State& saved(_saved_states.back());

        // Apply restoration of the state.
        switch (saved.reason) {
            case Reason::FULL: {
                // Full restore.
                _state = _saved_states.back();
                break;
            }
            case Reason::READ_SIZE: {
                // Skip potentially unread data in pushed read area.
                // Restore write pointer and read-only indicator.
                assert(_state.wbyte <= saved.wbyte);
                _state.rbyte = _state.wbyte;
                _state.wbyte = saved.wbyte;
                _state.read_only = saved.read_only;
                break;
            }
            case Reason::WRITE_SIZE: {
                // Restore end of buffer.
                assert(_state.end <= saved.end);
                _state.end = saved.end;
                break;
            }
            case Reason::WRITE_LEN_SEQ: {
                // Save current state.
                State current(_state);
                // Compute the number bytes in the written sequence.
                const size_t bytes = current.wbyte - (8 * saved.wbyte + saved.wbit + saved.len_bits) / 8;
                // Back to state at the beginning of the length field and write length field.
                _state = saved;
                putBits(bytes, saved.len_bits);
                // Restore current state.
                _state = current;
                break;
            }
            default: {
                // Corrupted saved state.
                assert(false);
                return false;
            }
        }

        // Drop last saved state.
        _saved_states.pop_back();
    }

    return true;
}


//----------------------------------------------------------------------------
// Temporary reduce the new readable size using a length field.
//----------------------------------------------------------------------------

size_t ts::Buffer::pushReadSizeFromLength(size_t length_bits)
{
    // Read the length field.
    size_t length = getBits<size_t>(length_bits);
    if (_read_error || _state.rbit != 0) {
        // Length not read or not byte-aligned.
        _read_error = true;
        return NPOS;
    }
    else {
        // Reduce the readable size. Will be maximized by the write pointer.
        return pushReadSize(_state.rbyte + length);
    }
}


//----------------------------------------------------------------------------
// Swap the current state with the one on top of the stack of saved states.
//----------------------------------------------------------------------------

size_t ts::Buffer::swapState()
{
    if (_saved_states.empty()) {
        // Empty stack, cannot swap, simply push current state.
        _saved_states.push_back(_state);
    }
    else if (_saved_states.back().reason != Reason::FULL) {
        // Cannot swap when with a temporary state.
        _read_error = _write_error = true;
        return NPOS;
    }
    else {
        // Actually swap states.
        std::swap(_state, _saved_states.back());
    }
    return _saved_states.size() - 1;
}


//----------------------------------------------------------------------------
// Drop the last saved state from the stack of saved states.
//----------------------------------------------------------------------------

bool ts::Buffer::dropState(size_t level)
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
// Change the buffer size.
//----------------------------------------------------------------------------

bool ts::Buffer::resize(size_t size, bool reallocate)
{
    // Get the max write pointer in saved values.
    size_t new_size = _state.wbyte + (_state.wbit + 7) / 8;
    for (const auto& it : _saved_states) {
        new_size = std::max<size_t>(new_size, it.wbyte + (it.wbit + 7) / 8);
    }
    assert(new_size <= _buffer_size);

    // We need at least the largest saved write pointer.
    new_size = std::max(new_size, size);

    // Reallocate (enlarge or shrink) if necessary.
    if (reallocate && _allocated && new_size != _buffer_size) {

        // Allocate new buffer.
        const size_t new_buffer_size = std::max(MINIMUM_SIZE, new_size);
        uint8_t* const new_buffer = new uint8_t[new_buffer_size];
        CheckNonNull(new_buffer);

        // Copy previous buffer and deallocate it.
        if (_buffer != nullptr) {
            std::memcpy(new_buffer, _buffer, std::min(_buffer_size, new_size));
            delete[] _buffer;
        }

        // Switch to new buffer.
        _buffer = new_buffer;
        _buffer_size = new_buffer_size;

        // Make sure that all saved state don't allow more than new size.
        for (auto& it : _saved_states) {
            it.end = std::min(it.end, new_size);
        }
    }

    // We accept at most the physical buffer size.
    _state.end = std::min(new_size, _buffer_size);

    // Return success only if the requested size was granted.
    return size == _state.end;
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
    assert(_state.wbyte <= _state.end);
    assert(_state.wbyte < _state.end || _state.wbit == 0);

    if (_state.read_only) {
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
    assert(_state.end <= _buffer_size);
    assert(_state.wbyte <= _state.end);
    assert(_state.wbyte < _state.end || _state.wbit == 0);

    // Forbid invalid values.
    if (_state.read_only || bit > 7) {
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
    if (byte > _state.end || (byte == _state.end && bit > 0)) {
        // Move to end of buffer.
        _state.wbyte = _state.end;
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
    if (_state.read_only) {
        _write_error = true;
        return false;
    }

    // Save current state for potential trash if moving forward.
    State prev(_state);

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
        std::memset(_buffer + prev.wbyte, stuffing, _state.wbyte - prev.wbyte);
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
    if (byte < _state.end && end_bit > start_bit) {

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
    assert(_state.end >= _state.wbyte);
    return _state.end - _state.wbyte; // ignore bit offset
}

size_t ts::Buffer::remainingWriteBits() const
{
    assert(_state.end > _state.wbyte || (_state.end == _state.wbyte && _state.wbit == 0));
    return 8 * (_state.end - _state.wbyte) - _state.wbit;
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
// Skip read reserved bits forward.
//----------------------------------------------------------------------------

bool ts::Buffer::skipReservedBits(size_t bits, int expected)
{
    expected &= 1;  // force 0 or 1
    while (!_read_error && bits-- > 0) {
        if (getBit() != expected && !_read_error) {
            // Invalid reserved bit.
            _reserved_bits_errors.push_back((currentReadBitOffset() << 1) | expected);
        }
    }
    return !_read_error;
}


//----------------------------------------------------------------------------
// This static method returns a string describing "reserved bits errors".
//----------------------------------------------------------------------------

ts::UString ts::Buffer::ReservedBitsErrorString(std::vector<size_t>& errors, size_t base_offset, const UString& margin)
{
    UString message;
    std::sort(errors.begin(), errors.end());
    for (size_t value : errors) {
        if (!message.empty()) {
            message.push_back(LINE_FEED);
        }
        message.format(u"%sByte %d, bit #%d should be '%d'", {margin, (value >> 4) + base_offset, (value >> 1) & 0x07, value & 0x01});
    }
    return message;
}


//----------------------------------------------------------------------------
// Serialize the number of reserved '1' bits
//----------------------------------------------------------------------------

bool ts::Buffer::putReserved(size_t bits)
{
    while (bits >= 32 && putUInt32(~0u)) {
        bits -= 32;
    }
    return putBits<int>(~0, bits);
}


//----------------------------------------------------------------------------
// Request some read size in bytes. Return actually possible read size.
//----------------------------------------------------------------------------

size_t ts::Buffer::requestReadBytes(size_t bytes)
{
    assert(_state.rbyte <= _state.wbyte);

    // Maximum possible bytes to read.
    const size_t max_bytes = _read_error ? 0 : remainingReadBits() / 8;

    if (bytes == NPOS) {
        return max_bytes;
    }
    else if (bytes <= max_bytes) {
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
        std::memcpy(data, _buffer + _state.rbyte, bytes);
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

void ts::Buffer::getBytes(ByteBlock& bb, size_t bytes)
{
    bytes = requestReadBytes(bytes);
    bb.resize(bytes);
    readBytesInternal(bb.data(), bytes);
}

ts::ByteBlock ts::Buffer::getBytes(size_t bytes)
{
    bytes = requestReadBytes(bytes);
    ByteBlock bb(bytes);
    readBytesInternal(bb.data(), bytes);
    return bb;
}

size_t ts::Buffer::getBytesAppend(ByteBlock& bb, size_t bytes)
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
    return putBytes(bb.data() + start, count);
}

size_t ts::Buffer::putBytes(const uint8_t* buffer, size_t bytes)
{
    assert(_state.wbyte <= _state.end);
    assert(_buffer != nullptr);
    assert(_state.wbit < 8);

    // Can't write on read-only or if write error already set.
    if (_state.read_only || _write_error) {
        _write_error = true;
        return 0;
    }

    // Actual size to write.
    if (_state.wbyte + bytes > _state.end) {
        bytes = _state.end - _state.wbyte;
        _write_error = true;
    }

    // Write bytes.
    if (_state.wbit == 0) {
        // Write pointer is byte aligned, bulk copy.
        std::memcpy(_buffer + _state.wbyte, buffer, bytes);
        _state.wbyte += bytes;
    }
    else {
        // Unaligned write pointer, copy small pieces.
        if (_state.wbyte + bytes == _state.end) {
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

uint8_t ts::Buffer::getBit()
{
    if (_read_error || endOfRead()) {
        _read_error = true;
        return 0;
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
    if (_state.read_only || _write_error || endOfWrite()) {
        _write_error = true;
        return false;
    }

    assert(_state.wbyte <= _state.end);
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


//----------------------------------------------------------------------------
// Try to get an ASCII string.
//----------------------------------------------------------------------------

ts::UString ts::Buffer::tryGetASCII(size_t bytes)
{
    UString str;
    tryGetASCII(str, bytes);
    return str;
}

bool ts::Buffer::tryGetASCII(UString& result, size_t bytes)
{
    // If size is unspecified, use the rest of the buffer.
    if (bytes == NPOS) {
        bytes = remainingReadBytes();
    }

    // Parameter validation.
    if (_read_error || _state.rbit != 0 || bytes > remainingReadBytes()) {
        _read_error = true;
        result.clear();
        return false;
    }

    // Validate each character in the are.
    bool valid = true;
    for (size_t i = 0; valid && i < bytes; ++i) {
        const uint8_t c = _buffer[_state.rbyte + i];
        if (c >= 0x20 && c <= 0x7E) {
            // This is an ASCII character.
            if (i == result.size()) {
                result.push_back(UChar(c));
            }
            else {
                // But come after trailing zero.
                valid = false;
            }
        }
        else if (c != 0) {
            // Not ASCII, not trailing zero, unusable string.
            valid = false;
        }
    }

    if (valid) {
        _state.rbyte += bytes;
    }
    else {
        result.clear();
    }
    return valid;
}


//----------------------------------------------------------------------------
// Transform an output string parameter into a returned value.
//----------------------------------------------------------------------------

ts::UString ts::Buffer::outStringToResult(size_t param, bool (Buffer::*method)(UString&, size_t))
{
    UString result;
    (this->*method)(result, param);
    return result;
}


//----------------------------------------------------------------------------
// Get a UTF-8 or UTF-16 string.
//----------------------------------------------------------------------------

bool ts::Buffer::getUTFInternal(UString& result, size_t bytes, bool utf8)
{
    // If size is unspecified, use the rest of the buffer.
    if (bytes == NPOS) {
        bytes = remainingReadBytes();
    }

    // Parameter validation.
    if (_read_error || _state.rbit != 0 || bytes > remainingReadBytes()) {
        _read_error = true;
        return false;
    }

    // Decode UTF-8 or UTF-16 characters.
    if (utf8) {
        result.assignFromUTF8(reinterpret_cast<const char*>(_buffer + _state.rbyte), bytes);
        _state.rbyte += bytes;
    }
    else if (isNativeEndian()) {
        // Decode UTF-16 in native endian => direct mapping.
        result.assign(reinterpret_cast<const UChar*>(_buffer + _state.rbyte), bytes / 2);
        _state.rbyte += bytes;
    }
    else {
        // Decode UTF-16 in opposite endian => decode characters one by one.
        result.resize(bytes / 2);
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] = UChar(getUInt16());
        }
        if (bytes % 2 != 0) {
            // Odd number of bytes, last one is ignored.
            skipBytes(1);
        }
    }

    // Remove trailing zeroes, considered as padding/stuffing from fixed-size strings.
    while (!result.empty() && result.back() == CHAR_NULL) {
        result.pop_back();
    }

    return true;
}


//----------------------------------------------------------------------------
// Get a UTF-8 or UTF-16 string (preceded by its length).
//----------------------------------------------------------------------------

bool ts::Buffer::getUTFWithLengthInternal(UString& result, size_t length_bits, bool utf8)
{
    // The size of the length field must be representable as a size_t.
    if (_read_error || length_bits == 0 || length_bits > 8 * sizeof(size_t)) {
        _read_error = true;
        return false;
    }

    // Attempt to read the length.
    const State saved(_state);
    const size_t length = getBits<size_t>(length_bits);
    if (_read_error || _state.rbit != 0 || length > remainingReadBytes()) {
        _state = saved; // revert the length field in case of error
        _read_error = true;
        return false;
    }

    // Read the characters as a fixed string (length is already validated).
    return getUTFInternal(result, length, utf8);
}


//----------------------------------------------------------------------------
// Put a string using UTF format.
//----------------------------------------------------------------------------

size_t ts::Buffer::putUTFInternal(const UString& str, size_t start, size_t count, bool partial, size_t fixed_size, int pad, bool utf8)
{
    // Normalize start and count within allowed bounds.
    start = std::min(start, str.size());
    count = std::min(count, str.size() - start);

    if (_state.read_only || _write_error || _state.wbit != 0) {
        _write_error = true;
        return 0; // 0 byte with partial, 0 as false otherwise
    }

    if (fixed_size != NPOS && remainingWriteBytes() < fixed_size) {
        _write_error = true;
        return 0; // as false
    }

    // Save the position before attempting to serialize.
    const State saved(_state);

    // Serialize as many characters as possible.
    const UChar* const in_start = &str[start];
    const UChar* in = in_start;
    const UChar* const in_end = in + count;
    char* const cbuffer = reinterpret_cast<char*>(_buffer);
    char* out = cbuffer + _state.wbyte;
    char* const out_end = cbuffer + (fixed_size == NPOS ? _state.end : std::min(_state.end, _state.wbyte + fixed_size));

    if (utf8) {
        // Convert to UTF-8 and include the written data in the buffer.
        UString::ConvertUTF16ToUTF8(in, in_end, out, out_end);
        _state.wbyte = out - cbuffer;
    }
    else {
        // Encode UTF-16 characters one by one.
        while (in < in_end && out + 1 < out_end) {
            putUInt16(uint16_t(*in++));
            out += 2;
        }
    }

    assert(in >= in_start);
    assert(in <= in_end);
    assert(out >= cbuffer + _state.wbyte);
    assert(out <= out_end);

    if (partial) {
        // Always accept the conversion, return the number of written characters.
        return in - in_start;
    }
    else if (fixed_size != NPOS) {
        // Fixed-size serialization, pad if necessary and return "true".
        if (utf8) {
            // Pad with 8-bit values.
            std::memset(out, pad, out_end - out);
            _state.wbyte = out_end - cbuffer;
        }
        else {
            // Pad with 16-bit values.
            while (cbuffer + _state.wbyte + 1 < out_end) {
                putUInt16(uint16_t(pad));
            }
            if (cbuffer + _state.wbyte < out_end) {
                // Pad an even number of bytes, use LSB of pad value as last byte.
                putUInt8(uint8_t(pad));
            }
            assert(cbuffer + _state.wbyte == out_end);
        }
        return 1;
    }
    else if (in == in_end) {
        // Full conversion completed, return "true".
        return 1;
    }
    else {
        // Full conversion failed, restore state and return "false".
        _state = saved;
        _write_error = true;
        return 0;
    }
}


//----------------------------------------------------------------------------
// Put a string (preceded by its length) using UTF format.
//----------------------------------------------------------------------------

size_t ts::Buffer::putUTFWithLengthInternal(const UString& str, size_t start, size_t count, size_t length_bits, bool partial, bool utf8)
{
    // Normalize start and count within allowed bounds.
    start = std::min(start, str.size());
    count = std::min(count, str.size() - start);

    // The size of the length field must be representable as a size_t.
    // The write pointer must be byte-aligned after writing the length field.
    if (_state.read_only || _write_error || length_bits == 0 || length_bits > 8 * sizeof(size_t) || (_state.wbit + length_bits) % 8 != 0) {
        _write_error = true;
        return 0; // 0 byte with partial, 0 as false otherwise
    }

    // Cannot write more bytes than representable in the length field.
    const size_t max_bytes = length_bits == 8 * sizeof(size_t) ? std::numeric_limits<size_t>::max() : (size_t(1) << length_bits) - 1;

    // Save the position for the length field and write a zero place-holder for the length.
    const State saved(_state);
    putBits(0, length_bits);
    assert(!_write_error);
    assert(_state.wbit == 0);

    // Now we can attempt the conversion.
    const UChar* const in_start = &str[start];
    const UChar* in = in_start;
    const UChar* const in_end = in + count;
    char* const cbuffer = reinterpret_cast<char*>(_buffer);
    char* const out_start = cbuffer + _state.wbyte;
    char* out = out_start;
    char* const out_end = out_start + std::min(_state.end - _state.wbyte, max_bytes);

    if (utf8) {
        // Encode UTF-8.
        UString::ConvertUTF16ToUTF8(in, in_end, out, out_end);
    }
    else if (isNativeEndian()) {
        // Encode UTF-16 using native endian => direct copy.
        // Warning: UChar pointer arithmetics uses 2-byte unit.
        // Always copy an even number of bytes (& ~1).
        const size_t size = std::min(2 * (in_end - in), out_end - out) & ~1;
        std::memcpy(out, in, size);
        in += size / 2;
        out += size;
    }
    else {
        // Encode UTF-16 in opposite endian => decode characters one by one.
        while (in < in_end && out + 1 < out_end) {
            putUInt16(uint16_t(*in++));
            out += 2;
        }
    }

    assert(in >= in_start);
    assert(in <= in_end);
    assert(out >= out_start);
    assert(out <= out_end);

    // Restore state before zero-length place-holder
    _state = saved;

    if (partial || in == in_end) {
        // Accept the conversion.
        // Write actual length in length field.
        putBits(out - out_start, length_bits);
        assert(!_write_error);
        assert(_state.wbit == 0);
        // The write pointer is now after the written bytes.
        _state.wbyte = out - cbuffer;
        // Return a number of characters for a partial write and a "boolean" for a full write.
        return partial ? in - in_start : 1;
    }
    else {
        // Full conversion failed.
        _write_error = true;
        return 0;
    }
}
