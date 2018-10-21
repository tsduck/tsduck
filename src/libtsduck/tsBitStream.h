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
//!
//!  @file
//!  Read a bitstream in MSB order.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    //!
    //! Class to analyze a bit-stream in memory.
    //! @ingroup cpp
    //!
    //! An instance of this class is used to analyze a continuous
    //! bit stream in memory, ignoring byte boundaries.
    //! The bit-stream can be read bit by bit.
    //! Integer values of any size can be read, regardless of alignment.
    //!
    //! The order of which the bits are read is the following:
    //! The bytes are read in increasing order of address.
    //! The bits within each byte are read from the most significant one
    //! to the least significant one.
    //!
    //! When multi-bit integer values are extracted, they are interpreted
    //! in most significant bit first order (MSB first). But the integer
    //! value is returned in native byte order.
    //!
    //! An instance of @c BitStream is an @e analyzer,
    //! it does not @e contain the bit stream data. An instance must
    //! be associated to a memory area. This association can be performed
    //! in a constructor or using the @c reset() method.
    //!
    class TSDUCKDLL BitStream
    {
    private:
        const uint8_t* _base;       // First byte
        size_t         _start_bit;  // Start bit offset in first byte
        size_t         _end_bit;    // End bit offset in stream (one after last bit)
        size_t         _next_bit;   // Next bit to read

    public:
        //!
        //! Default constructor.
        //! Object is unusable as long as reset is not invoked.
        //!
        BitStream() :
            _base(nullptr),
            _start_bit(0),
            _end_bit(0),
            _next_bit(0)
        {
        }

        //!
        //! Constructor using a memory area which must remain valid as long as the BitStream object is used.
        //! @param [in] data Address of data area to analyze.
        //! @param [in] size_in_bits Size in bits of the data area.
        //! @param [in] bit_offset_in_first_byte The number of initial bits to ignore in the first byte. Default: zero.
        //!
        BitStream(const void* data, size_t size_in_bits, size_t bit_offset_in_first_byte = 0) :
            _base(nullptr),
            _start_bit(0),
            _end_bit(0),
            _next_bit(0)
        {
            reset(data, size_in_bits, bit_offset_in_first_byte);
        }

        //!
        //! Copy constructor.
        //! The same data area will be analyzed, starting at the current point.
        //! @param [in] bs Other bitstream to copy.
        //!
        BitStream(const BitStream& bs) :
            _base(bs._base),
            _start_bit(bs._start_bit),
            _end_bit(bs._end_bit),
            _next_bit(bs._next_bit)
        {
        }

        //!
        //! Assignment (use the same buffer).
        //! The same data area will be analyzed, starting at the current point.
        //! @param [in] bs Other bitstream to copy.
        //! @return A reference to this object.
        //!
        BitStream& operator=(const BitStream& bs)
        {
            _base = bs._base;
            _start_bit = bs._start_bit;
            _end_bit = bs._end_bit;
            _next_bit = bs._next_bit;
            return *this;
        }

        //!
        //! Check if this object is currently associated with a memory area.
        //! @return True if this object is currently associated with a memory area.
        //!
        bool isAssociated() const
        {
            return _base != nullptr;
        }

        //!
        //! Reset with a memory area which must remain valid as long as the BitStream object is used.
        //! @param [in] data Address of data area to analyze.
        //! @param [in] size_in_bits Size in bits of the data area.
        //! @param [in] bit_offset_in_first_byte The number of initial bits to ignore in the first byte. Default: zero.
        //!
        void reset(const void* data, size_t size_in_bits, size_t bit_offset_in_first_byte = 0)
        {
            _base = reinterpret_cast<const uint8_t*>(data) + (bit_offset_in_first_byte >> 3);
            _start_bit = bit_offset_in_first_byte & 0x07;
            _end_bit = _start_bit + size_in_bits;
            _next_bit = _start_bit;
        }

        //!
        //! Reset parsing at the specified bit offset.
        //! @param [in] bit_offset Offset of next bit to analyze.
        //!
        void seek(size_t bit_offset)
        {
            _next_bit = std::min(_end_bit, _start_bit + bit_offset);
        }

        //!
        //! Get current bit position.
        //! @return The offset of the current bit from the starting one.
        //!
        size_t currentBitOffset() const
        {
            assert(_next_bit >= _start_bit);
            assert(_next_bit <= _end_bit);
            return _next_bit - _start_bit;
        }

        //!
        //! Get number of remaining bits.
        //! @return The number of remaining bits.
        //!
        size_t remainingBitCount() const
        {
            assert(_next_bit >= _start_bit);
            assert(_next_bit <= _end_bit);
            return _end_bit - _next_bit;
        }

        //!
        //! Check end of stream.
        //! @return True if the end of stream is reached.
        //!
        bool endOfStream() const
        {
            return _next_bit >= _end_bit;
        }

        //!
        //! Check if the current bit pointer is on a byte boundary.
        //! @return True if the next byte to read is at the beginning of a byte.
        //!
        bool byteAligned() const
        {
            return (_next_bit & 0x07) == 0;
        }

        //!
        //! Skip bits.
        //! @param [in] n Number of bits to skip.
        //!
        void skip(size_t n) {
            _next_bit = std::min(_end_bit, _next_bit + n);
        }

        //!
        //! Back n bits.
        //! @param [in] n Number of bits to skip back.
        //!
        void back(size_t n) {
            _next_bit = _start_bit + n >= _next_bit ? _start_bit : _next_bit - n;
        }

        //!
        //! Advance pointer to next byte boundary.
        //!
        void skipToNextByte()
        {
            _next_bit = std::min(_end_bit, (_next_bit + 8) & ~0x07);
        }

        //!
        //! Read the next bit and advance the bitstream pointer.
        //! @param [in] def Default value to return if already at end of stream.
        //! @return The value of the next bit.
        //!
        uint8_t readBit(uint8_t def = 0)
        {
            if (_next_bit >= _end_bit) {
                return def;
            }
            else {
                const uint8_t b = (_base[_next_bit >> 3] >> (7 - (_next_bit & 0x07))) & 0x01;
                _next_bit++;
                return b;
            }
        }

        //!
        //! Read the next n bits as an integer value and advance the bitstream pointer.
        //! @tparam INT An integer type for the result.
        //! @param [in] n Number of bits to read.
        //! @param [in] def Default value to return if less than @a n bits before end of stream.
        //! @return The value of the next @a n bits.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        INT read(size_t n, INT def = 0)
        {
            if (_next_bit + n > _end_bit) {
                return def;
            }
            INT val = 0;
            // Read leading bits up to byte boundary
            while (n > 0 && (_next_bit & 0x07) != 0) {
                val = (val << 1) | INT (readBit());
                --n;
            }
            // Read complete bytes
            const uint8_t* byte = _base + (_next_bit >> 3);
            while (n > 7) {
                val = (val << 8) | INT(*byte++);
                _next_bit += 8;
                n -= 8;
            }
            // Read trailing bits
            while (n > 0) {
                val = (val << 1) | INT(readBit());
                --n;
            }
            return val;
        }
    };
}
