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
//
//  Some utilities on integers.
//
//----------------------------------------------------------------------------

#include "tsIntegerUtils.h"
TSDUCK_SOURCE;


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
