//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    static const uint64_t pow10[MAX_POWER_10 + 1] = {
        /*  0 */ 1,
        /*  1 */ 10,
        /*  2 */ 100,
        /*  3 */ 1'000,
        /*  4 */ 10'000,
        /*  5 */ 100'000,
        /*  6 */ 1'000'000,
        /*  7 */ 10'000'000,
        /*  8 */ 100'000'000,
        /*  9 */ 1'000'000'000,
        /* 10 */ 10'000'000'000,
        /* 11 */ 100'000'000'000,
        /* 12 */ 1'000'000'000'000,
        /* 13 */ 10'000'000'000'000,
        /* 14 */ 100'000'000'000'000,
        /* 15 */ 1'000'000'000'000'000,
        /* 16 */ 10'000'000'000'000'000,
        /* 17 */ 100'000'000'000'000'000,
        /* 18 */ 1'000'000'000'000'000'000,
        /* 19 */ 10'000'000'000'000'000'000ull,
    };

    return pow <= MAX_POWER_10 ? pow10[pow] : 0;
}
