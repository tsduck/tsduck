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

#pragma once
#include "tsStringUtils.h"

//
// In this module, we work on formal integer types INT. We use std::numeric_limits<INT> to test the
// capability of the type (is_signed, etc.) But, for each instantiation of INT, the corresponding
// expression is constant and the Microsoft compiler complains about that. Disable this specific
// warning in the module. Note the #pragma warning(pop) at the end of file.
//
// warning C4127: conditional expression is constant
// 
#if defined(__msc)
    #pragma warning(push)
    #pragma warning(disable:4127)
#endif

//----------------------------------------------------------------------------
// Convert a string into an integer.
//----------------------------------------------------------------------------

template <typename INT>
bool ts::ToInteger(INT& value,
                     const char* from,
                     size_t length,
                     const char* thousandSeparators)
{
    typedef typename std::numeric_limits<INT> limits;

    // Initial value, up to decode error
    value = static_cast<INT>(0);

    // Reject non-integer type (floating points, etc.) and invalid parameters
    if (!limits::is_integer || from == 0 || thousandSeparators == 0) {
        return false;
    }

    // Locate actual begin and end of integer value
    const char* start = from;
    const char* end = from + length;
    while (start < end && IsSpace(*start)) {
        ++start;
    }
    while (start < end && IsSpace(*(end-1))) {
        --end;
    }

    // Skip optional sign
    bool negative = false;
    if (start < end) {
        if (*start == '+') {
            ++start;
        }
        else if (*start == '-') {
            if (!limits::is_signed) {
                // INT type is unsigned, invalid signed value
                return false;
            }
            ++start;
            negative = true;
        }
    }

    // Look for hexadecimal prefix
    int base = 10;
    if (start + 1 < end && *start == '0' && (start[1] == 'x' || start[1] == 'X')) {
        start += 2;
        base = 16;
    }

    // Filter empty string
    if (start == end) {
        return false;
    }
    assert(start < end);

    // Decode the string
    while (start < end) {
        const int digit = ToIntegerDigit(*start, base);
        if (digit >= 0) {
            // Character is a valid digit
            value = value * static_cast<INT>(base) + static_cast<INT>(digit);
        }
        else if (::strchr(thousandSeparators, *start) == 0) {
            // Character is not a possible thousands separator
            break;
        }
        ++start;
    }

    // Apply sign
    if (negative) {
        // If the type is unsigned, "value = -value" will never be executed
        // but Visual C++ complains. Suppress the warning.
        #if defined(__msc)
            #pragma warning(push)
            #pragma warning(disable:4146)
        #endif
        value = -value;
        #if defined(__msc)
            #pragma warning (pop)
        #endif
    }

    // Success only if we went down to the end of string
    return start == end;
}


//----------------------------------------------------------------------------
// Convert a string containing a list of integers into a container of integers.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::ToIntegers(CONTAINER& container,
                      const char* from,
                      size_t length,
                      const char* thousandSeparators,
                      const char* listSeparators)
{
    // Let's name INT the integer type.
    // In all STL standard containers, value_type is a typedef for the element type.
    typedef typename CONTAINER::value_type INT;

    // Reset the content of the container
    container.clear();

    // Reject invalid parameters
    if (from == 0 || thousandSeparators == 0 || listSeparators == 0) {
        return false;
    }

    // Locate segments in the string
    const char* start = from;
    const char* farEnd = from + length;

    // Loop on segments
    while (start < farEnd) {
        // Skip spaces and list separators
        while (start < farEnd && (IsSpace(*start) || ::strchr(listSeparators, *start) != 0)) {
            ++start;
        }
        // Locate end of segment
        const char* end = start;
        while (end < farEnd && ::strchr(listSeparators, *end) == 0) {
            ++end;
        }
        // Exit at end of string
        if (start >= farEnd) {
            break;
        }
        // Decode segment
        INT value;
        const bool success = ToInteger(value, start, end - start, thousandSeparators);
        if (success || value != 0) {
            container.push_back(value);
        }
        if (!success) {
            return false;
        }
        // Move to next segment
        start = end;
    }

    return true;
}

#if defined(__msc)
    #pragma warning(pop)
#endif
