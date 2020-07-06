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

#pragma once


//----------------------------------------------------------------------------
// Read the next n bits as an integer value and advance the read pointer.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::Buffer::getBits(size_t bits, INT def)
{
    // No read if read error is already set or not enough bits to read.
    if (_read_error || currentReadBitOffset() + bits > currentWriteBitOffset()) {
        _read_error = true;
        return def;
    }

    INT val = 0;

    if (_big_endian) {
        // Read leading bits up to byte boundary
        while (bits > 0 && _state.rbit != 0) {
            val = INT(val << 1) | INT(getBit());
            --bits;
        }

        // Read complete bytes
        while (bits > 7) {
            val = INT(val << 8) | INT(_buffer[_state.rbyte++]);
            bits -= 8;
        }

        // Read trailing bits
        while (bits > 0) {
            val = INT(val << 1) | INT(getBit());
            --bits;
        }
    }
    else {
        // Little endian decoding
        int shift = 0;

        // Read leading bits up to byte boundary
        while (bits > 0 && _state.rbit != 0) {
            val |= INT(getBit()) << shift;
            --bits;
            shift++;
        }

        // Read complete bytes
        while (bits > 7) {
            val |= INT(_buffer[_state.rbyte++]) << shift;
            bits -= 8;
            shift += 8;
        }

        // Read trailing bits
        while (bits > 0) {
            val |= INT(getBit()) << shift;
            --bits;
            shift++;
        }
    }

    return val;
}


//----------------------------------------------------------------------------
// Put the next n bits from an integer value and advance the write pointer.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::Buffer::putBits(INT value, size_t bits)
{
    // No write if write error is already set or read-only or not enough bits to write.
    if (_write_error || _read_only || remainingWriteBits() < bits) {
        _write_error = true;
        return false;
    }

    if (_big_endian) {
        // Write leading bits up to byte boundary
        while (bits > 0 && _state.wbit != 0) {
            putBit(uint8_t((value >> --bits) & 1));
        }

        // Write complete bytes.
        while (bits > 7) {
            bits -= 8;
            _buffer[_state.wbyte++] = uint8_t(value >> bits);
        }

        // Write trailing bits
        while (bits > 0) {
            putBit(uint8_t((value >> --bits) & 1));
        }
    }
    else {
        // Little endian encoding. Write leading bits up to byte boundary.
        while (bits > 0 && _state.wbit != 0) {
            putBit(uint8_t(value & 1));
            value >>= 1;
            --bits;
        }

        // Write complete bytes.
        while (bits > 7) {
            _buffer[_state.wbyte++] = uint8_t(value);
            TS_PUSH_WARNING()
            TS_LLVM_NOWARNING(shift-count-overflow) // "shift count >= width of type" for 8-bit ints
            value >>= 8;
            TS_POP_WARNING()
            bits -= 8;
        }

        // Write trailing bits
        while (bits > 0) {
            putBit(uint8_t(value & 1));
            value >>= 1;
            --bits;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Internal put integer method.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::Buffer::putint(INT value, size_t bytes, void (*putBE)(void*,INT), void (*putLE)(void*,INT))
{
    // Internally used to write up to 8 bytes (64-bit integers).
    assert(bytes <= 8);

    // No write if write error is already set or read-only.
    if (_write_error || _read_only) {
        _write_error = true;
        return false;
    }

    // Hypothetical new write pointer (bit pointer won't change).
    const size_t new_wbyte = _state.wbyte + bytes;

    if (new_wbyte > _buffer_max || (new_wbyte == _buffer_max && _state.wbit > 0)) {
        // Not enough bytes to write.
        _write_error = true;
        return false;
    }
    else if (_state.wbit == 0) {
        // Write pointer is byte aligned. Most common case.
        if (_big_endian) {
            putBE(_buffer + _state.wbyte, value);
        }
        else {
            putLE(_buffer + _state.wbyte, value);
        }
        _state.wbyte = new_wbyte;
        return true;
    }
    else {
        // Write pointer is not byte aligned. Use an intermediate buffer.
        uint8_t buf[8];
        if (_big_endian) {
            putBE(buf, value);
        }
        else {
            putLE(buf, value);
        }
        putBytes(buf, bytes);
        assert(_state.wbyte == new_wbyte);
        return true;
    }
}
