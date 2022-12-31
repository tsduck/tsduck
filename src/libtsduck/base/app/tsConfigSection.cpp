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

#include "tsConfigSection.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ConfigSection::ConfigSection() :
    _entries()
{
}


//----------------------------------------------------------------------------
// Reset content of the configuration section
//----------------------------------------------------------------------------

void ts::ConfigSection::reset()
{
    _entries.clear();
}


//----------------------------------------------------------------------------
// Get the names of all entries in a section
//----------------------------------------------------------------------------

void ts::ConfigSection::getEntryNames(UStringVector& names) const
{
    names.clear();
    for (const auto& ent : _entries) {
        names.push_back(ent.first);
    }
}


//----------------------------------------------------------------------------
// Get the number of values in an entry.
//----------------------------------------------------------------------------

size_t ts::ConfigSection::valueCount(const UString& entry) const
{
    const auto ent = _entries.find(entry);
    return ent == _entries.end() ? 0 : ent->second.size();
}


//----------------------------------------------------------------------------
// Get a value in an entry.
//----------------------------------------------------------------------------

ts::UString ts::ConfigSection::value(const UString& entry, size_t index, const UString& defvalue) const
{
    const auto ent = _entries.find(entry);
    return ent == _entries.end() || index >= ent->second.size() ? defvalue : ent->second[index];
}


//----------------------------------------------------------------------------
// Same as above but interpret the content as a boolean.
// Return defvalue if the value cannot be interpreted as a boolean.
// Valid boolean representations are "true" and "false".
//----------------------------------------------------------------------------

bool ts::ConfigSection::boolValue(const UString& entry, size_t index, bool defvalue) const
{
    bool val = false;
    return value(entry, index).toBool(val) ? val : defvalue;
}


//----------------------------------------------------------------------------
// Set the value of an entry.
//----------------------------------------------------------------------------

void ts::ConfigSection::set(const UString& entry, const UString& val)
{
    UStringVector& ent(_entries[entry]);
    ent.clear();
    ent.push_back(val);
}


//----------------------------------------------------------------------------
// Set the value of an entry.
//----------------------------------------------------------------------------

void ts::ConfigSection::set(const UString& entry, const UStringVector& val)
{
    _entries[entry] = val;
}


//----------------------------------------------------------------------------
// Set the boolean value of an entry.
//----------------------------------------------------------------------------

void ts::ConfigSection::set(const UString& entry, bool val)
{
    this->set(entry, val ? u"true" : u"false");
}


//----------------------------------------------------------------------------
// Set the boolean value of an entry.
//----------------------------------------------------------------------------

void ts::ConfigSection::set(const UString& entry, const std::vector<bool>& val)
{
    UStringVector& ent(_entries[entry]);
    ent.clear();
    for (size_t i = 0; i < val.size(); ++i) {
        ent.push_back(val[i] ? u"true" : u"false");
    }
}

//----------------------------------------------------------------------------
// Append values in an entry
//----------------------------------------------------------------------------

void ts::ConfigSection::append(const UString& entry, const UString& val)
{
    _entries[entry].push_back(val);
}


//----------------------------------------------------------------------------
// Append values in an entry
//----------------------------------------------------------------------------

void ts::ConfigSection::append(const UString& entry, const UStringVector& val)
{
    UStringVector& ent(_entries[entry]);
    ent.insert(ent.end(), val.begin(), val.end());
}


//----------------------------------------------------------------------------
// Append values in an entry
//----------------------------------------------------------------------------

void ts::ConfigSection::append(const UString& entry, bool val)
{
    append(entry, val ? u"true" : u"false");
}


//----------------------------------------------------------------------------
// Append values in an entry
//----------------------------------------------------------------------------

void ts::ConfigSection::append(const UString& entry, const std::vector<bool>& val)
{
    UStringVector& ent(_entries[entry]);
    for (size_t i = 0; i < val.size(); ++i) {
        ent.push_back(val[i] ? u"true" : u"false");
    }
}


//----------------------------------------------------------------------------
// Save the content of a section in a stream
//----------------------------------------------------------------------------

std::ostream& ts::ConfigSection::save(std::ostream& strm) const
{
    for (const auto& ent : _entries) {
        for (const auto& conf : ent.second) {
            strm << ent.first << " = " << conf << std::endl;
        }
    }
    return strm;
}
