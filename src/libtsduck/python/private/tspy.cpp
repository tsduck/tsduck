//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tspy.h"
#include "tsMemory.h"


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
            MemCopy(buffer, str.data(), *size);
        }
    }
}
