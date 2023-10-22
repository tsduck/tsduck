//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    _byte(_base)
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
