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
#include "tsToInteger.h"


//----------------------------------------------------------------------------
// Get the integer value of an option.
//----------------------------------------------------------------------------

template <typename INT>
void ts::Args::getIntValue(INT& value, const char* name, const INT& def_value, size_t index) const
{
    const IOption& opt (getIOption(name));
    if (index >= opt.values.size() ||
        !opt.values[index].set() ||
        !ToInteger(value, opt.values[index].value(), THOUSANDS_SEPARATORS)) {
        value = def_value;
    }
}

template <typename INT>
INT ts::Args::intValue(const char* name, const INT& def_value, size_t index) const
{
    INT value = def_value;
    getIntValue (value, name, def_value, index);
    return value;
}


//----------------------------------------------------------------------------
// Return all occurences of this option in a vector of integers.
//----------------------------------------------------------------------------

template <typename INT>
void ts::Args::getIntValues(std::vector<INT>& values, const char* name) const
{
    INT value;
    const IOption& opt (getIOption(name));

    values.clear();
    values.reserve (opt.values.size());

    for (ArgValueVector::const_iterator it = opt.values.begin(); it != opt.values.end(); ++it) {
        if (it->set() && ToInteger<INT>(value, it->value(), THOUSANDS_SEPARATORS)) {
            values.push_back (value);
        }
    }
}


//----------------------------------------------------------------------------
// Return all occurences of this option in a set of integers.
//----------------------------------------------------------------------------

template <typename INT>
void ts::Args::getIntValues(std::set<INT>& values, const char* name) const
{
    INT value;
    const IOption& opt(getIOption(name));

    values.clear();

    for (ArgValueVector::const_iterator it = opt.values.begin(); it != opt.values.end(); ++it) {
        if (it->set() && ToInteger<INT>(value, it->value(), THOUSANDS_SEPARATORS)) {
            values.insert (value);
        }
    }
}


//----------------------------------------------------------------------------
// Get an OR'ed of all values of an integer option.
//----------------------------------------------------------------------------

template <typename INT>
void ts::Args::getBitMaskValue(INT& value, const char* name, const INT& def_value) const
{
    const IOption& opt(getIOption(name));
    if (opt.values.empty()) {
        value = def_value;
    }
    else {
        value = static_cast<INT>(0);
        for (ArgValueVector::const_iterator it = opt.values.begin(); it != opt.values.end(); ++it) {
            INT mask = static_cast<INT>(0);
            if (!it->set() || !ToInteger(mask, it->value(), THOUSANDS_SEPARATORS)) {
                value = def_value;
                return;
            }
            value |= mask;
        }
    }
}

template <typename INT>
INT ts::Args::bitMaskValue(const char* name, const INT& def_value) const
{
    INT value(def_value);
    getBitMaskValue(value, name, def_value);
    return value;
}
