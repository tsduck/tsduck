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
//
//  Binary Coded Decimal utilities
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    // Return the decimal value of a BCD-encoded byte.

    TSDUCKDLL inline int DecodeBCD (uint8_t b)
    {
        return 10 * (b >> 4) + (b & 0x0F);
    }

    // Return a one-byte BCD representation of an integer (must be in 0..99)

    TSDUCKDLL inline uint8_t EncodeBCD (int i)
    {
        return (((i / 10) % 10) << 4) | (i % 10);
    }

    // Return the decimal value of a BCD-encoded string, on bcd_count digits
    // (bcd_count/2 bytes). Note that bcd_count can be even.

    TSDUCKDLL uint32_t DecodeBCD (const uint8_t* bcd, size_t bcd_count);

    // Decode a variable-length BCD-encoded integer.
    // Return a string representation in str.
    // The BCD-encoded data start at bcd, on bcd_count digits
    // (bcd_count/2 bytes).
    // The 'decimal' value indicates the position of the virtual decimal
    // point (-1: none, 0: before first digit, 1: after first digit, etc.)

    TSDUCKDLL void BCDToString (std::string &str, const uint8_t* bcd, size_t bcd_count, int decimal);
}
