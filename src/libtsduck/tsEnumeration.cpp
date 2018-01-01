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
//!
//! @file tsEnumeration.h
//!
//! Declare the ts::Enumeration class.
//!
//----------------------------------------------------------------------------

#include "tsEnumeration.h"
TSDUCK_SOURCE;

//
// Values for "not found"
//
const int ts::Enumeration::UNKNOWN = std::numeric_limits<int>::max();


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::Enumeration::Enumeration() :
    _map()
{
}

ts::Enumeration::Enumeration(const std::initializer_list<NameValue> values) :
    _map()
{
    for (std::initializer_list<NameValue>::const_iterator it = values.begin(); it != values.end(); ++it) {
        _map.insert(std::make_pair(it->value, it->name));
    }
}


//----------------------------------------------------------------------------
// Get the value from a name, abbreviation allowed.
//----------------------------------------------------------------------------

int ts::Enumeration::value(const UString& name, bool caseSensitive) const
{
    const UString lcName(name.toLower());
    size_t previousCount = 0;
    int previous = UNKNOWN;

    for (EnumMap::const_iterator it = _map.begin(); it != _map.end(); ++it) {
        if ((caseSensitive && it->second == name) || (!caseSensitive && it->second.toLower() == lcName)) {
            // Found an exact match
            return it->first;
        }
        else if (it->second.startWith(name, caseSensitive ? CASE_SENSITIVE : CASE_INSENSITIVE)) {
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
    else if (name.toInteger(previous, u",")) {
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

ts::UString ts::Enumeration::name(int value, bool hexa, size_t hexDigitCount) const
{
    const EnumMap::const_iterator it = _map.find(value);
    if (it != _map.end()) {
        return it->second;
    }
    else if (hexa) {
        return UString::Format(u"0x%0*X", {hexDigitCount, value});
    }
    else {
        return UString::Decimal(value, 0, true, UString());
    }
}


//----------------------------------------------------------------------------
// Return a comma-separated list of all possible names.
//----------------------------------------------------------------------------

ts::UString ts::Enumeration::nameList(const UString& separator) const
{
    UString list;
    for (EnumMap::const_iterator it = _map.begin(); it != _map.end(); ++it) {
        if (!list.empty()) {
            list += separator;
        }
        list += it->second;
    }
    return list;
}
