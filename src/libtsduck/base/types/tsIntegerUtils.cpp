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

#include "tsIntegerUtils.h"


//----------------------------------------------------------------------------
// Add the size of a separator every N characters.
//----------------------------------------------------------------------------

namespace {
    size_t AddSeparatorSize(size_t width, size_t digitSeparatorSize, size_t groupSize)
    {
        if (width == 0 || digitSeparatorSize == 0 || groupSize == 0) {
            return width;
        }
        else if (width % groupSize == 0) {
            const size_t nbGroups = (width / groupSize) - 1;
            return groupSize + nbGroups * (groupSize + digitSeparatorSize);
        }
        else {
            const size_t nbGroups = width / groupSize;
            return width % groupSize + nbGroups * (groupSize + digitSeparatorSize);
        }
    }
}


//----------------------------------------------------------------------------
// Compute the maximum width of the decimal representation of an integer type.
//----------------------------------------------------------------------------

size_t ts::MaxDecimalWidth(size_t typeSize, size_t digitSeparatorSize)
{
    // 1 byte = 3 chars, 2 bytes = 5 chars, 4 bytes = 10 chars, 8 bytes = 20 chars.
    const size_t width = 2 * typeSize + (typeSize + 1) / 2;

    // Add the size of the separator every 3 characters.
    return AddSeparatorSize(width, digitSeparatorSize, 3);
}


//----------------------------------------------------------------------------
// Compute the maximum width of the hexa representation of an integer type.
//----------------------------------------------------------------------------

size_t ts::MaxHexaWidth(size_t typeSize, size_t digitSeparatorSize)
{
    // Add the size of the separator every 4 characters.
    return AddSeparatorSize(2 * typeSize, digitSeparatorSize, 4);
}


//----------------------------------------------------------------------------
// Get a power of 10 using a fast lookup table.
//----------------------------------------------------------------------------

uint64_t ts::Power10(size_t pow)
{
    // Assume that not integer type is larger than 64 bits => 10^19 is the largest unsigned value.
    static constexpr size_t MAX_POW10 = 19;
    static const uint64_t pow10[MAX_POW10 + 1] = {
        /*  0 */ TS_UCONST64(1),
        /*  1 */ TS_UCONST64(10),
        /*  2 */ TS_UCONST64(100),
        /*  3 */ TS_UCONST64(1000),
        /*  4 */ TS_UCONST64(10000),
        /*  5 */ TS_UCONST64(100000),
        /*  6 */ TS_UCONST64(1000000),
        /*  7 */ TS_UCONST64(10000000),
        /*  8 */ TS_UCONST64(100000000),
        /*  9 */ TS_UCONST64(1000000000),
        /* 10 */ TS_UCONST64(10000000000),
        /* 11 */ TS_UCONST64(100000000000),
        /* 12 */ TS_UCONST64(1000000000000),
        /* 13 */ TS_UCONST64(10000000000000),
        /* 14 */ TS_UCONST64(100000000000000),
        /* 15 */ TS_UCONST64(1000000000000000),
        /* 16 */ TS_UCONST64(10000000000000000),
        /* 17 */ TS_UCONST64(100000000000000000),
        /* 18 */ TS_UCONST64(1000000000000000000),
        /* 19 */ TS_UCONST64(10000000000000000000),
    };

    return pow <= MAX_POW10 ? pow10[pow] : 0;
}
