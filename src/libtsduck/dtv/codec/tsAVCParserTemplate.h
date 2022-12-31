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


//----------------------------------------------------------------------------
// Provide the next n bits without advancing the bitstream pointer.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::AVCParser::nextBits(INT& val, size_t n)
{
    ts_avcparser_assert_consistent();

    const uint8_t* saved_byte = _byte;
    size_t saved_bit = _bit;

    bool result = readBits(val, n);
    _byte = saved_byte;
    _bit = saved_bit;

    return result;
}


//----------------------------------------------------------------------------
// Read the next n bits and advance the bitstream pointer.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::AVCParser::readBits(INT& val, size_t n)
{
    ts_avcparser_assert_consistent();

    val = 0;

    // Check that there are enough bits
    if (remainingBits() < n) {
        return false;
    }

    // Read leading bits up to byte boundary
    while (n > 0 && _bit != 0) {
        val = INT(val << 1) | nextBit();
        --n;
    }

    // Read complete bytes
    while (n > 7) {
        val = INT(val << 8) | *_byte;
        nextByte();
        n -= 8;
    }

    // Read trailing bits
    while (n > 0) {
        val = INT(val << 1) | nextBit();
        --n;
    }

    return true;
}


//----------------------------------------------------------------------------
// Extract Exp-Golomb-coded value using n bits.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::AVCParser::expColomb(INT& val)
{
    ts_avcparser_assert_consistent();

    // See ISO/IEC 14496-10 section 9.1
    val = 0;
    int leading_zero_bits = -1;
    for (uint8_t b = 0; b == 0; leading_zero_bits++) {
        if (_byte >= _end) {
            return false;
        }
        b = nextBit();
    }
    if (!readBits(val, leading_zero_bits)) {
        return false;
    }

    val += (INT (1) << leading_zero_bits) - 1;
    return true;
}


//----------------------------------------------------------------------------
// Signed integer Exp-Golomb-coded using n bits.
//----------------------------------------------------------------------------

template <typename INT,
          typename std::enable_if<std::is_integral<INT>::value>::type*,
          typename std::enable_if<std::is_signed<INT>::value>::type*>
bool ts::AVCParser::se(INT& val)
{
    // See ISO/IEC 14496-10 section 9.1.1
    if (expColomb(val)) {
        val = ((val % 2) == 0 ? -1 : 1) * (val + 1) / 2;
        return true;
    }
    else {
        return false;
    }
}
