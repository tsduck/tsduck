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
//
//  This class holds a "configuration section".
//  A configuration section contains a list of "entries". Each entry has one
//  or more values. A value can be interpreted as a string, integer or boolean.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsToInteger.h"
#include "tsDecimal.h"

//----------------------------------------------------------------------------
// Get a value in an entry.
//----------------------------------------------------------------------------

template <typename INT>
INT ts::ConfigSection::value (const std::string& entry, size_t index, const INT& defvalue) const
{
    INT result (0);
    if (ToInteger<INT> (result, this->value (entry, index))) {
        return result;
    }
    else {
        return defvalue;
    }
}

//----------------------------------------------------------------------------
// Set the value of an entry as an integer
//----------------------------------------------------------------------------

template <typename INT>
void ts::ConfigSection::set (const std::string& entry, const INT& value)
{
    this->set (entry, Decimal (value, 0, true, ""));
}

//----------------------------------------------------------------------------
// Set the value of an entry as a vector of integers
//----------------------------------------------------------------------------

template <typename INT>
void ts::ConfigSection::set (const std::string& entry, const std::vector <INT>& value)
{
    this->deleteEntry (entry);
    this->append (entry, value);
}

//----------------------------------------------------------------------------
// Append an integer value in an antry
//----------------------------------------------------------------------------

template <typename INT>
void ts::ConfigSection::append (const std::string& entry, const INT& value)
{
    this->append (entry, Decimal (value, 0, true, ""));
}

//----------------------------------------------------------------------------
// Append a vector of integer values in an antry
//----------------------------------------------------------------------------

template <typename INT>
void ts::ConfigSection::append (const std::string& entry, const std::vector <INT>& value)
{
    for (size_t i = 0; i < value.size (); ++i) {
        this->append (entry, std::string (value[i]));
    }
}
