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

#define TS_AVCPARSER_CPP 1 // used in tsAVCParser.h
#include "tsAVCParser.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::AVCParser::AVCParser(const void* data, size_t size_in_bytes) :
    _base(reinterpret_cast<const uint8_t*>(data)),
    _end(_base + size_in_bytes),
    _total_size(size_in_bytes),
    _byte(_base),
    _bit(0)
{
    ts_avcparser_assert_consistent();
}


//----------------------------------------------------------------------------
// Reset with a new memory area.
//----------------------------------------------------------------------------

void ts::AVCParser::reset(const void* data, size_t size_in_bytes)
{
    _base = reinterpret_cast<const uint8_t*>(data);
    _end = _base + size_in_bytes;
    _total_size = size_in_bytes;
    _byte = _base;
    _bit = 0;

    ts_avcparser_assert_consistent();
}


//----------------------------------------------------------------------------
// Reset parsing at the specified point.
//----------------------------------------------------------------------------

void ts::AVCParser::reset(size_t byte_offset, size_t bit_offset)
{
    _byte = _base + std::min(byte_offset + bit_offset / 8, _total_size);
    _bit = _byte == _end ? 0 : bit_offset % 8;

    ts_avcparser_assert_consistent();
}


//----------------------------------------------------------------------------
// Get number of remaining bytes (rounded down).
//----------------------------------------------------------------------------

size_t ts::AVCParser::remainingBytes() const
{
    ts_avcparser_assert_consistent();
    return _end - _byte - (_bit != 0);
}


//----------------------------------------------------------------------------
// Get number of remaining bits.
//----------------------------------------------------------------------------

size_t ts::AVCParser::remainingBits() const
{
    ts_avcparser_assert_consistent();
    return 8 * (_end - _byte) - _bit;
}


//----------------------------------------------------------------------------
// Skip an rbsp_trailing_bits() as defined by ISO/EIC 14496-10 7.3.2.11
// Return true if one was found and skipped.
//----------------------------------------------------------------------------

bool ts::AVCParser::rbspTrailingBits()
{
    ts_avcparser_assert_consistent();

    const uint8_t* saved_byte = _byte;
    size_t saved_bit = _bit;
    uint8_t bit = 0;

    bool valid = readBits(bit, 1) && bit == 1;
    while (valid && !byteAligned()) {
        valid = readBits(bit, 1) && bit == 0;
    }
    if (!valid) {
        _byte = saved_byte;
        _bit = saved_bit;
    }
    return valid;
}


//----------------------------------------------------------------------------
// Advance pointer to next byte boundary.
//----------------------------------------------------------------------------

void ts::AVCParser::nextByte()
{
    // Internal method, must be called when next byte is available.
    assert(_byte >= _base);
    assert(_byte < _end);
    assert(_bit <= 8); // can be called internally when _bit == 8

    // Next byte boundary
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


//----------------------------------------------------------------------------
// Advance pointer by one bit and return the bit value
//----------------------------------------------------------------------------

uint8_t ts::AVCParser::nextBit()
{
    ts_avcparser_assert_consistent();

    uint8_t b = (*_byte >> (7 - _bit)) & 0x01;
    if (++_bit > 7) {
        nextByte();
    }
    return b;
}
