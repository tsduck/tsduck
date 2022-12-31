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

#include "tsEnumeration.h"

// Values for "not found"
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
    for (const auto& it : values) {
        _map.insert(std::make_pair(it.value, it.name));
    }
}


//----------------------------------------------------------------------------
// Operators
//----------------------------------------------------------------------------

bool ts::Enumeration::operator==(const Enumeration& other) const
{
    return _map == other._map;
}

#if defined(TS_NEED_UNEQUAL_OPERATOR)
bool ts::Enumeration::operator!=(const Enumeration& other) const
{
    return _map != other._map;
}
#endif


//----------------------------------------------------------------------------
// Get the value from a name, abbreviation allowed.
//----------------------------------------------------------------------------

TS_PUSH_WARNING()
TS_GCC_NOWARNING(shadow) // workaround for a bug in GCC 7.5

int ts::Enumeration::value(const UString& name, bool caseSensitive, bool abbreviated) const
{
    const UString lcName(name.toLower());
    size_t previousCount = 0;
    int previous = UNKNOWN;

    for (const auto& it : _map) {
        if ((caseSensitive && it.second == name) || (!caseSensitive && it.second.toLower() == lcName)) {
            // Found an exact match
            return it.first;
        }
        else if (abbreviated && it.second.startWith(name, caseSensitive ? CASE_SENSITIVE : CASE_INSENSITIVE)) {
            // Found an abbreviated version
            if (++previousCount == 1) {
                // First abbreviation, remember it and continue searching
                previous = it.first;
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

TS_POP_WARNING()


//----------------------------------------------------------------------------
// Get the error message about a name failing to match a value.
//----------------------------------------------------------------------------

ts::UString ts::Enumeration::error(const UString& name1, bool caseSensitive, bool abbreviated, const UString& designator, const UString& prefix) const
{
    const UString lcName(name1.toLower());
    UStringList maybe;

    for (const auto& it : _map) {
        if ((caseSensitive && it.second == name1) || (!caseSensitive && it.second.toLower() == lcName)) {
            // Found an exact match, there is no error.
            return UString();
        }
        else if (abbreviated && it.second.startWith(name1, caseSensitive ? CASE_SENSITIVE : CASE_INSENSITIVE)) {
            // Found an abbreviated version.
            maybe.push_back(prefix + it.second);
        }
    }

    if (maybe.empty()) {
        return UString::Format(u"unknown %s \"%s%s\"", {designator, prefix, name1});
    }
    else if (maybe.size() == 1) {
        // Only one possibility, there is no error.
        return UString();
    }
    else {
        return UString::Format(u"ambiguous %s \"%s%s\", could be one of %s", {designator, prefix, name1, UString::Join(maybe)});
    }
}


//----------------------------------------------------------------------------
// Get the name from a value.
//----------------------------------------------------------------------------

ts::UString ts::Enumeration::intToName(int value, bool hexa, size_t hexDigitCount) const
{
    const auto it = _map.find(value);
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
// Get the names from a bit-mask value.
//----------------------------------------------------------------------------

ts::UString ts::Enumeration::bitMaskNames(int value, const ts::UString& separator, bool hexa, size_t hexDigitCount) const
{
    UString list;
    int done = 0; // Bitmask of all values which are already added in the list.

    // Insert all known names.
    for (const auto& it : _map) {
        if ((value & it.first) == it.first) {
            // This bit pattern is present.
            done |= it.first;
            if (!list.empty()) {
                list += separator;
            }
            list += it.second;
        }
    }

    // Now loop on bits which were not already printed.
    value &= ~done;
    for (int mask = 1; value != 0 && mask != 0; mask <<= 1) {
        value &= ~mask;
        if (!list.empty()) {
            list += separator;
        }
        if (hexa) {
            list += UString::Format(u"0x%0*X", {hexDigitCount, mask});
        }
        else {
            list += UString::Decimal(mask, 0, true, UString());
        }
    }

    return list;
}


//----------------------------------------------------------------------------
// Return a comma-separated list of all possible names.
//----------------------------------------------------------------------------

ts::UString ts::Enumeration::nameList(const UString& separator, const UString& inQuote, const UString& outQuote) const
{
    UStringVector sl;
    sl.reserve(_map.size());
    for (const auto& it : _map) {
        sl.push_back(inQuote + it.second + outQuote);
    }
    std::sort(sl.begin(), sl.end());
    return UString::Join(sl, separator);
}
