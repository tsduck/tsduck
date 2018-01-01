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
//
//  Binary Coded Decimal utilities
//
//----------------------------------------------------------------------------

#include "tsBCD.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Return the decimal value of a BCD-encoded string, on bcd_count digits
// (bcd_count/2 bytes). Note that bcd_count can be even.
//----------------------------------------------------------------------------

uint32_t ts::DecodeBCD(const uint8_t* bcd, size_t bcd_count)
{
    uint32_t result = 0;

    for (size_t index = 0; index < bcd_count; index++) {
        if (index % 2 == 0) {
            result = 10 * result + (*bcd >> 4);
        }
        else {
            result = 10 * result + (*bcd++ & 0x0F);
        }
    }

    return result;
}


//----------------------------------------------------------------------------
// Encode a Binary Coded Decimal (BCD) string.
//----------------------------------------------------------------------------

void ts::EncodeBCD(uint8_t* bcd, size_t bcd_count, uint32_t value)
{
    if (bcd_count > 0) {
        bcd += (bcd_count - 1) / 2;
        for (size_t index = bcd_count; index > 0; --index) {
            if (index % 2 == 0) {
                *bcd = (*bcd & 0xF0) | (value % 10);
            }
            else {
                *bcd = (*bcd & 0x0F) | ((value % 10) << 4);
                bcd--;
            }
            value = value / 10;
        }
    }
}


//----------------------------------------------------------------------------
// Decode a variable-length BCD-encoded integer.
// Return a string representation in str.
// The BCD-encoded data start at bcd, on bcd_count digits
// (bcd_count/2 bytes).
// The 'decimal' value indicates the position of the virtual decimal
// point (-1: none, 0: before first digit, 1: after first digit, etc.)
//----------------------------------------------------------------------------

void ts::BCDToString(std::string &str, const uint8_t* bcd, size_t bcd_count, int decimal)
{
    // Cleanup result string and over-pre-allocate
    str.clear();
    str.reserve(bcd_count + 2);

    // Insert Decimal point in first position
    if (decimal == 0) {
        str.push_back('0');
    }

    // Decode the BCD
    for (size_t i = 0; i < bcd_count; i++) {
        if (decimal == int (i)) {
            str.push_back('.');
        }
        int digit = (i % 2 == 0) ? (*bcd >> 4) : (*bcd++ & 0x0F);
        if (digit != 0 || !str.empty()) {
            str.push_back(char('0' + digit));
        }
    }
}
