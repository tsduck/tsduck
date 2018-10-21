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
//  Utility routines for memory operations.
//
//----------------------------------------------------------------------------

#include "tsMemoryUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Check if a memory area starts with the specified prefix
//----------------------------------------------------------------------------

bool ts::StartsWith (const void* area, size_t area_size, const void* prefix, size_t prefix_size)
{
    if (prefix_size == 0 || area_size < prefix_size) {
        return false;
    }
    else {
        return ::memcmp (area, prefix, prefix_size) == 0;
    }
}


//----------------------------------------------------------------------------
// Locate a pattern into a memory area. Return 0 if not found
//----------------------------------------------------------------------------

const void* ts::LocatePattern (const void* area, size_t area_size, const void* pattern, size_t pattern_size)
{
    if (pattern_size > 0) {
        const uint8_t* a = reinterpret_cast <const uint8_t*> (area);
        const uint8_t* p = reinterpret_cast <const uint8_t*> (pattern);
        while (area_size >= pattern_size) {
            if (*a == *p && ::memcmp (a, p, pattern_size) == 0) {
                return a;
            }
            ++a;
            --area_size;
        }
    }
    return nullptr; // not found
}

//----------------------------------------------------------------------------
// Check if a memory area contains all identical byte values.
//----------------------------------------------------------------------------

bool ts::IdenticalBytes(const void * area, size_t area_size)
{
    if (area_size < 2) {
        return false;
    }
    else {
        const uint8_t* d = reinterpret_cast<const uint8_t*>(area);
        for (size_t i = 0; i < area_size - 1; ++i) {
            if (d[i] != d[i + 1]) {
                return false;
            }
        }
        return true;
    }
}
