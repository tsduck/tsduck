//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#include "tsIntegerUtils.h"


//----------------------------------------------------------------------------
// Read the next n bits as an integer value and advance the read pointer.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::Buffer::getBits(size_t bits)
{
    typedef typename std::make_unsigned<INT>::type UNSINT;
    const INT value = static_cast<INT>(getBits<UNSINT>(bits));
    return SignExtend(value, bits);
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Buffer::getBits(Variable<INT>& value, size_t bits)
{
    if (_read_error || currentReadBitOffset() + bits > currentWriteBitOffset()) {
        _read_error = true;
        value.clear();
    }
    else {
        value = getBits<INT>(bits);
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
INT ts::Buffer::getBits(size_t bits)
{
    // No read if read error is already set or not enough bits to read.
    if (_read_error || currentReadBitOffset() + bits > currentWriteBitOffset()) {
        _read_error = true;
        return 0;
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
    if (_write_error || _state.read_only || remainingWriteBits() < bits) {
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

template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_floating_point<INT>::value, int>::type>
bool ts::Buffer::putint(INT value, size_t bytes, void (*putBE)(void*,INT), void (*putLE)(void*,INT))
{
    // Internally used to write up to 8 bytes (64-bit integers).
    assert(bytes <= 8);

    // No write if write error is already set or read-only.
    if (_write_error || _state.read_only) {
        _write_error = true;
        return false;
    }

    // Hypothetical new write pointer (bit pointer won't change).
    const size_t new_wbyte = _state.wbyte + bytes;

    if (new_wbyte > _state.end || (new_wbyte == _state.end && _state.wbit > 0)) {
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


//----------------------------------------------------------------------------
// Read the next 4*n bits as a BCD value.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::Buffer::getBCD(size_t bcd_count)
{
    INT value = 0;
    getBCD(value, bcd_count);
    return value;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::Buffer::getBCD(INT& value, size_t bcd_count)
{
    typedef typename std::make_unsigned<INT>::type UNSINT;
    UNSINT uvalue = 0;

    if (_read_error || currentReadBitOffset() + 4 * bcd_count > currentWriteBitOffset()) {
        _read_error = true;
        value = 0;
        return false;
    }
    else {
        while (bcd_count-- > 0) {
            UNSINT nibble = getBits<UNSINT>(4);
            if (nibble > 9) {
                _read_error = true;
                nibble = 0;
            }
            uvalue = 10 * uvalue + nibble;
        }
        value = static_cast<INT>(uvalue);
        return true;
    }
}


//----------------------------------------------------------------------------
// Put the next 4*n bits as a BCD value.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::Buffer::putBCD(INT value, size_t bcd_count)
{
    // No write if write error is already set or read-only or not enough bits to write.
    if (_write_error || _state.read_only || remainingWriteBits() < 4 * bcd_count) {
        _write_error = true;
        return false;
    }

    if (bcd_count > 0) {
        typedef typename std::make_unsigned<INT>::type UNSINT;
        UNSINT uvalue = static_cast<UNSINT>(value);
        UNSINT factor = static_cast<UNSINT>(Power10(bcd_count));
        while (bcd_count-- > 0) {
            uvalue %= factor;
            factor /= 10;
            putBits(uvalue / factor, 4);
        }
    }
    return true;
}
