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
//!
//!  @file
//!  Parser for Advanced Video Coding (AVC, ISO 14496-10, ITU H.264) data
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class TSDUCKDLL AVCParser
    {
    public:
        // Constructor: Use a memory area which must remain valid as long as
        // the AVCParser object is used.
        AVCParser (const void* data, size_t size_in_bytes) :
            _base (reinterpret_cast <const uint8_t*> (data)),
            _end (_base + size_in_bytes),
            _total_size (size_in_bytes),
            _byte (_base),
            _bit (0)
        {
        }

        // Reset with a memory area which must remain valid as long as
        // the AVCParser object is used.
        void reset (const void* data, size_t size_in_bytes)
        {
            _base = reinterpret_cast <const uint8_t*> (data);
            _end = _base + size_in_bytes;
            _total_size = size_in_bytes;
            _byte = _base;
            _bit = 0;
        }

        // Reset parsing at the specified point, byte offset and bit offset
        // inside the byte. The bit offset zero is the most significant bit.
        void reset (size_t byte_offset = 0, size_t bit_offset = 0)
        {
            _byte = _base + std::min (byte_offset + bit_offset / 8, _total_size);
            _bit = bit_offset % 8;
        }

        // Get number of remaining bytes (rounded down)
        size_t remainingBytes() const
        {
            assert (_byte >= _base);
            assert (_byte <= _end);
            assert (_byte < _end || _bit == 0);
            return _end - _byte - (_bit != 0);
        }

        // Get number of remaining bits
        size_t remainingBits() const
        {
            assert (_byte >= _base);
            assert (_byte <= _end);
            assert (_byte < _end || _bit == 0);
            assert (_bit < 8);
            return 8 * (_end - _byte) - _bit;
        }

        // Check at end of stream
        bool endOfStream() const
        {
            return _byte >= _end;
        }

        // The following operations are specified by ISO/EIC 14496-10.
        // The template methods use an integer type for extracting data
        // and returns true on success, false on error.

        // Check if the current bit pointer is on a byte boundary
        bool byteAligned() const
        {
            return _bit == 0;
        }

        // Skip an rbsp_trailing_bits() as defined by ISO/EIC 14496-10 7.3.2.11
        // Return true if one was found and skipped.
        bool rbspTrailingBits();

        // Provide the next n bits without advancing the bitstream pointer.
        template <typename INT> bool nextBits (INT& val, size_t n);

        // Read the next n bits and advance the bitstream pointer.
        template <typename INT> bool readBits (INT& val, size_t n);

        // Unsigned integer using n bits.
        template <typename INT> bool u (INT& val, size_t n)
        {
            assert (std::numeric_limits<INT>::is_integer);
            assert (!std::numeric_limits<INT>::is_signed);
            return readBits (val, n);
        }

        // Signed integer using n bits.
        template <typename INT> bool i (INT& val, size_t n)
        {
            assert (std::numeric_limits<INT>::is_integer);
            assert (std::numeric_limits<INT>::is_signed);
            return readBits (val, n);
        }

        // Unsigned integer Exp-Golomb-coded
        template <typename INT> bool ue (INT& val)
        {
            assert (std::numeric_limits<INT>::is_integer);
            assert (!std::numeric_limits<INT>::is_signed);
            return expColomb (val);
        }

        // Signed integer Exp-Golomb-coded
        template <typename INT> bool se (INT& val);

    private:
        AVCParser() = delete;
        AVCParser(const AVCParser&) = delete;
        AVCParser& operator=(const AVCParser&) = delete;

        // Private members.
        const uint8_t* _base;
        const uint8_t* _end;
        size_t         _total_size;
        const uint8_t* _byte;         // Byte pointer
        size_t         _bit;          // Bit offset into *_byte

        // Advance pointer to next byte boundary.
        void nextByte()
        {
            // Next byte boundary
            assert (_byte < _end);
            ++_byte;
            _bit = 0;

            // Process start code emulation prevention: sequences 00 00 03
            // are used when 00 00 00 or 00 00 01 would be present. In that
            // case, the 00 00 is part of the raw byte sequence payload (rbsp)
            // but the 03 shall be discarded.
            if (_byte < _end && *_byte == 0x03 && _byte > _base+1 && _byte[-1] == 0x00 && _byte[-2] == 0x00) {
                // Skip 03 after 00 00
                ++_byte;
            }
        }

        // Advance pointer by one bit and return the bit value
        uint8_t nextBit()
        {
            assert (_byte < _end);
            uint8_t b = (*_byte >> (7 - _bit)) & 0x01;
            if (++_bit > 7) {
                nextByte();
            }
            return b;
        }

        // Extract Exp-Golomb-coded value using n bits.
        template <typename INT> bool expColomb (INT&);
    };
}

#include "tsAVCParserTemplate.h"
