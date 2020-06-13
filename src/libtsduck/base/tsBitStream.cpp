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

#include "tsBitStream.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::BitStream::BitStream() :
    _base(nullptr),
    _start_bit(0),
    _end_bit(0),
    _next_bit(0)
{
}

ts::BitStream::BitStream(const void* data, size_t size_in_bits, size_t bit_offset_in_first_byte) :
    _base(nullptr),
    _start_bit(0),
    _end_bit(0),
    _next_bit(0)
{
    reset(data, size_in_bits, bit_offset_in_first_byte);
}

ts::BitStream::BitStream(const BitStream& bs) :
    _base(bs._base),
    _start_bit(bs._start_bit),
    _end_bit(bs._end_bit),
    _next_bit(bs._next_bit)
{
}


//----------------------------------------------------------------------------
// Assignment (use the same buffer).
//----------------------------------------------------------------------------

ts::BitStream& ts::BitStream::operator=(const BitStream& bs)
{
    _base = bs._base;
    _start_bit = bs._start_bit;
    _end_bit = bs._end_bit;
    _next_bit = bs._next_bit;
    return *this;
}


//----------------------------------------------------------------------------
// Reset with a memory area which must remain valid as long as the BitStream object is used.
//----------------------------------------------------------------------------

void ts::BitStream::reset(const void* data, size_t size_in_bits, size_t bit_offset_in_first_byte)
{
    _base = reinterpret_cast<const uint8_t*>(data) + (bit_offset_in_first_byte >> 3);
    _start_bit = bit_offset_in_first_byte & 0x07;
    _end_bit = _start_bit + size_in_bits;
    _next_bit = _start_bit;
}


//----------------------------------------------------------------------------
// Get current bit position.
//----------------------------------------------------------------------------

size_t ts::BitStream::currentBitOffset() const
{
    assert(_next_bit >= _start_bit);
    assert(_next_bit <= _end_bit);
    return _next_bit - _start_bit;
}


//----------------------------------------------------------------------------
// Get number of remaining bits.
//----------------------------------------------------------------------------

size_t ts::BitStream::remainingBitCount() const
{
    assert(_next_bit >= _start_bit);
    assert(_next_bit <= _end_bit);
    return _end_bit - _next_bit;
}


//----------------------------------------------------------------------------
// Read the next bit and advance the bitstream pointer.
//----------------------------------------------------------------------------

uint8_t ts::BitStream::readBit(uint8_t def)
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
