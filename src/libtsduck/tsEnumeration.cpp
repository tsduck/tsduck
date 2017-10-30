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
//!
//! @file tsEnumeration.h
//!
//! Declare the ts::Enumeration class.
//!
//----------------------------------------------------------------------------

#include "tsEnumeration.h"
#include "tsToInteger.h"
#include "tsStringUtils.h"
#include "tsDecimal.h"
#include "tsFormat.h"
TSDUCK_SOURCE;

//
// Values for "not found"
//
const int ts::Enumeration::UNKNOWN = std::numeric_limits<int>::max();


//----------------------------------------------------------------------------
// Constructor from a variable list of string/int pairs.
//----------------------------------------------------------------------------

ts::Enumeration::Enumeration(const char* name, int value, ...) :
    _map()
{
    // Do not do anything if the null pointer is at the beginning
    if (name == 0) {
        return;
    }

    // Insert first element
    _map.insert(std::make_pair(value, std::string(name)));

    // Loop on variable argument list
    va_list ap;
    va_start(ap, value);
    for (;;) {
        const char* n = va_arg(ap, const char*);
        // There is a trick here. Normally, we documented that the last argument
        // must be a TS_NULL pointer, ie. the symbol "TS_NULL", not the literal "0".
        // This is important because a variable argument list is untyped and "0"
        // is interpreted as "int literal zero". On some platforms, int and char*
        // may have different sizes.
        bool isNull = n == 0;
        // In the following test, we know that the expression "sizeof(int) < sizeof(char*)" is constant.
        // warning C4127: conditional expression is constant
        #if defined(TS_MSC)
            #pragma warning (push)
            #pragma warning (disable:4127)
        #endif
        if (!isNull && sizeof(int) < sizeof(char*)) {
            #if defined(TS_MSC)
                #pragma warning (pop)
            #endif
            // Try to detect buggy "0" literal. This is not bullet-proof and
            // may even lead to false positive in rare occasion (valid pointer
            // appearing as a zero). But this prevent a crash in most buggy
            // applications.
            const uintptr_t ns = reinterpret_cast<uintptr_t>(n);
            const uintptr_t intMask = uintptr_t((unsigned int)(~0));
            isNull = (ns & intMask) == 0;
        }
        if (isNull) {
            break;
        }
        int v = va_arg(ap, int);
        _map.insert(std::make_pair(v, std::string(n)));
    }
    va_end(ap);
}


//----------------------------------------------------------------------------
// Get the value from a name, abbreviation allowed.
//----------------------------------------------------------------------------

int ts::Enumeration::value(const std::string& name, bool caseSensitive) const
{
    const std::string lcName(LowerCaseValue(name));
    size_t previousCount = 0;
    int previous = UNKNOWN;

    for (EnumMap::const_iterator it = _map.begin(); it != _map.end(); ++it) {
        if ((caseSensitive && it->second == name) || (!caseSensitive && LowerCaseValue(it->second) == lcName)) {
            // Found an exact match
            return it->first;
        }
        else if ((caseSensitive && it->second.find(name) == 0) || (!caseSensitive && LowerCaseValue(it->second).find(lcName) == 0)) {
            // Found an abbreviated version
            if (++previousCount == 1) {
                // First abbreviation, remember it and continue searching
                previous = it->first;
            }
            else {
                // Another abbreviation already found, name is ambiguous
                break;
            }
        }
    }

    if (previousCount == 1) {
        // Only one solution for abbreviation
        return previous;
    }
    else if (ToInteger(previous, name)) {
        // Name evaluates to an integer
        return previous;
    }
    else {
        // Ambiguous or not found, not an integer, cannot resolve
        return UNKNOWN;
    }
}


//----------------------------------------------------------------------------
// Get the name from a value.
//----------------------------------------------------------------------------

std::string ts::Enumeration::name(int value, bool hexa, size_t hexDigitCount) const
{
    const EnumMap::const_iterator it = _map.find(value);
    if (it != _map.end()) {
        return it->second;
    }
    else if (hexa) {
        return Format("0x%0*X", int(hexDigitCount), value);
    }
    else {
        return Decimal(value, 0, true, "");
    }
}


//----------------------------------------------------------------------------
// Return a comma-separated list of all possible names.
//----------------------------------------------------------------------------

std::string ts::Enumeration::nameList(const char* separator) const
{
    std::string list;
    for (EnumMap::const_iterator it = _map.begin(); it != _map.end(); ++it) {
        if (!list.empty()) {
            list += separator;
        }
        list += it->second;
    }

    return list;
}
