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
// Check if an integer value is in range for an option.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_same<INT, uint64_t>::value>::type*>
bool ts::Args::IOption::inRange(INT value) const
{
    return value < TS_UCONST64(0x8000000000000000) && static_cast<int64_t>(value) >= min_value && static_cast<int64_t>(value) <= max_value;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::Args::IOption::inRange(INT value) const
{
    return static_cast<int64_t>(value) >= min_value && static_cast<int64_t>(value) <= max_value;
}


//----------------------------------------------------------------------------
// Get the integer value of an option.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Args::getIntValue(INT& value, const UChar* name, const INT& def_value, size_t index) const
{
    const IOption& opt(getIOption(name));
    if (opt.type != INTEGER || index >= opt.value_count) {
        // Invalid index.
        value = def_value;
    }
    else if (opt.value_count == opt.values.size()) {
        // No range, one integer per option, direct lookup.
        assert(index < opt.values.size());
        value = opt.values[index].int_count == 0 ? def_value : static_cast<INT>(opt.values[index].int_base);
    }
    else {
        // There is at least one range, iterate.
        bool found = false;
        for (size_t i = 0; !found && i < opt.values.size(); ++i) {
            if (index > 0 && index >= opt.values[i].int_count) {
                // Not in this range.
                index -= opt.values[i].int_count == 0 ? 1 : opt.values[i].int_count;
            }
            else {
                found = true;
                value = opt.values[i].int_count == 0 ? def_value : static_cast<INT>(opt.values[i].int_base + index);
            }
        }
        assert(found);
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
    value = static_cast<ENUM>(intValue<int64_t>(name, static_cast<int64_t>(def_value), index));
}

template <typename ENUM>
ENUM ts::Args::enumValue(const UChar* name, ENUM def_value, size_t index) const
{
    return static_cast<ENUM>(intValue<int64_t>(name, static_cast<int64_t>(def_value), index));
}


//----------------------------------------------------------------------------
// Return all occurences of this option in a vector of integers.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Args::getIntValues(std::vector<INT>& values, const UChar* name) const
{
    const IOption& opt(getIOption(name));
    values.clear();
    values.reserve(opt.value_count);
    for (auto it = opt.values.begin(); it != opt.values.end(); ++it) {
        for (int64_t v = it->int_base; v < it->int_base + int64_t(it->int_count); ++v) {
            if (opt.inRange<int64_t>(v)) {
                values.push_back(static_cast<INT>(v));
            }
        }
    }
}


//----------------------------------------------------------------------------
// Return all occurences of this option in a set of integers.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Args::getIntValues(std::set<INT>& values, const UChar* name) const
{
    const IOption& opt(getIOption(name));
    values.clear();
    for (auto it = opt.values.begin(); it != opt.values.end(); ++it) {
        for (int64_t v = it->int_base; v < it->int_base + int64_t(it->int_count); ++v) {
            if (opt.inRange<int64_t>(v)) {
                values.insert(static_cast<INT>(v));
            }
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
    if (opt.value_count > 0) {
        values.reset();
        for (auto it = opt.values.begin(); it != opt.values.end(); ++it) {
            for (int64_t v = it->int_base; v < it->int_base + int64_t(it->int_count); ++v) {
                if (v >= 0 && size_t(v) < values.size()) {
                    values.set(size_t(v));
                }
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
    if (opt.value_count == 0) {
        value = def_value;
    }
    else {
        value = static_cast<INT>(0);
        for (auto it = opt.values.begin(); it != opt.values.end(); ++it) {
            for (int64_t v = it->int_base; v < it->int_base + int64_t(it->int_count); ++v) {
                if (opt.inRange<int64_t>(v)) {
                    value |= static_cast<INT>(v);
                }
            }
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
