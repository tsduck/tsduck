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

#pragma once


//----------------------------------------------------------------------------
// Get the integer value of an option.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Args::getIntValue(INT& value, const UChar* name, const INT& def_value, size_t index) const
{
    const IOption& opt(getIOption(name));
    if (index >= opt.values.size() ||
        !opt.values[index].set() ||
        !opt.values[index].value().toInteger(value, THOUSANDS_SEPARATORS))
    {
        value = def_value;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::Args::intValue(const UChar* name, const INT& def_value, size_t index) const
{
    INT value = def_value;
    getIntValue(value, name, def_value, index);
    return value;
}


//----------------------------------------------------------------------------
// Get the enumeration value of an option.
//----------------------------------------------------------------------------

template <typename ENUM>
void ts::Args::getEnumValue(ENUM& value, const UChar* name, ENUM def_value, size_t index) const
{
    const IOption& opt(getIOption(name));
    int iValue = 0;
    if (index >= opt.values.size() ||
        !opt.values[index].set() ||
        !opt.values[index].value().toInteger(iValue, THOUSANDS_SEPARATORS))
    {
        value = def_value;
    }
    else {
        value = static_cast<ENUM>(iValue);
    }
}

template <typename ENUM>
ENUM ts::Args::enumValue(const UChar* name, ENUM def_value, size_t index) const
{
    ENUM value = def_value;
    getEnumValue(value, name, def_value, index);
    return value;
}


//----------------------------------------------------------------------------
// Return all occurences of this option in a vector of integers.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Args::getIntValues(std::vector<INT>& values, const UChar* name) const
{
    INT value;
    const IOption& opt(getIOption(name));

    values.clear();
    values.reserve(opt.values.size());

    for (ArgValueVector::const_iterator it = opt.values.begin(); it != opt.values.end(); ++it) {
        if (it->set() && it->value().toInteger<INT>(value, THOUSANDS_SEPARATORS)) {
            values.push_back(value);
        }
    }
}


//----------------------------------------------------------------------------
// Return all occurences of this option in a set of integers.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Args::getIntValues(std::set<INT>& values, const UChar* name) const
{
    INT value;
    const IOption& opt(getIOption(name));

    values.clear();

    for (ArgValueVector::const_iterator it = opt.values.begin(); it != opt.values.end(); ++it) {
        if (it->set() && it->value().toInteger<INT>(value, THOUSANDS_SEPARATORS)) {
            values.insert(value);
        }
    }
}


//----------------------------------------------------------------------------
// Get all occurences of an option as a bitmask of values.
//----------------------------------------------------------------------------

template <std::size_t N>
void ts::Args::getIntValues(std::bitset<N>& values, const UChar* name, bool defValue) const
{
    const IOption& opt(getIOption(name));

    if (!opt.values.empty()) {
        values.reset();
        for (auto it = opt.values.begin(); it != opt.values.end(); ++it) {
            size_t x = 0;
            if (it->set() && it->value().toInteger(x, THOUSANDS_SEPARATORS) && x < values.size()) {
                values.set(x);
            }
        }
    }
    else if (defValue) {
        values.set();
    }
    else {
        values.reset();
    }
}


//----------------------------------------------------------------------------
// Get an OR'ed of all values of an integer option.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Args::getBitMaskValue(INT& value, const UChar* name, const INT& def_value) const
{
    const IOption& opt(getIOption(name));
    if (opt.values.empty()) {
        value = def_value;
    }
    else {
        value = static_cast<INT>(0);
        for (ArgValueVector::const_iterator it = opt.values.begin(); it != opt.values.end(); ++it) {
            INT mask = static_cast<INT>(0);
            if (!it->set() || !it->value().toInteger(mask, THOUSANDS_SEPARATORS)) {
                value = def_value;
                return;
            }
            value |= mask;
        }
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::Args::bitMaskValue(const UChar* name, const INT& def_value) const
{
    INT value(def_value);
    getBitMaskValue(value, name, def_value);
    return value;
}
