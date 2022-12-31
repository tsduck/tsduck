//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tspy.h"


//-----------------------------------------------------------------------------
// Convert a UTF-16 buffer in a UString.
//-----------------------------------------------------------------------------

ts::UString ts::py::ToString(const uint8_t* buffer, size_t size)
{
    UString str;
    if (buffer != nullptr) {
        str.assign(reinterpret_cast<const UChar*>(buffer), size / 2);
        str.remove(BYTE_ORDER_MARK);
    }
    return str;
}


//-----------------------------------------------------------------------------
// Convert a UTF-16 buffer in a list of UString.
//-----------------------------------------------------------------------------

ts::UStringList ts::py::ToStringList(const uint8_t* buffer, size_t size)
{
    UStringList list;
    if (buffer != nullptr) {
        const UChar* start = reinterpret_cast<const UChar*>(buffer);
        const UChar* end = start + size / 2; // number of UChar
        for (;;) {
            const UChar* cur = start;
            while (cur < end && *cur != UChar(0xFFFF)) {
                ++cur;
            }
            UString str(start, cur - start);
            str.remove(ts::BYTE_ORDER_MARK);
            list.push_back(str);
            if (cur >= end) {
                break;
            }
            start = cur + 1; // skip 0xFFFF
        }
    }
    return list;
}


//-----------------------------------------------------------------------------
// Convert a string into a UTF-16 buffer.
//-----------------------------------------------------------------------------

void ts::py::FromString(const UString& str, uint8_t* buffer, size_t* size)
{
    if (size != nullptr) {
        if (buffer == nullptr) {
            *size = 0;
        }
        else {
            *size = std::min(*size, 2 * str.length()) & ~1;
            ::memcpy(buffer, str.data(), *size);
        }
    }
}
