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

#include "tsConfigSection.h"
#include "tsStringUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ConfigSection::ConfigSection() :
    _entries()
{
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::ConfigSection::~ConfigSection()
{
    this->reset();
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

void ts::ConfigSection::getEntryNames(StringVector& names) const
{
    names.clear();
    for (EntryMap::const_iterator ent = _entries.begin(); ent != _entries.end(); ++ent) {
        names.push_back(ent->first);
    }
}

//----------------------------------------------------------------------------
// Get the number of values in an entry.
// Return 0 if section or entry does not exist.
//----------------------------------------------------------------------------

size_t ts::ConfigSection::valueCount(const std::string& entry) const
{
    const EntryMap::const_iterator ent(_entries.find(entry));
    return ent == _entries.end() ? 0 : ent->second.size();
}

//----------------------------------------------------------------------------
// Get a value in an entry.
//----------------------------------------------------------------------------

const std::string& ts::ConfigSection::value (const std::string& entry, size_t index, const std::string& defvalue) const
{
    const EntryMap::const_iterator ent (_entries.find (entry));
    return ent == _entries.end () || index >= ent->second.size () ? defvalue : ent->second[index];
}

//----------------------------------------------------------------------------
// Same as above but interpret the content as a boolean.
// Return defvalue if the value cannot be interpreted as a boolean.
// Valid boolean representations are "true" and "false".
//----------------------------------------------------------------------------

bool ts::ConfigSection::boolValue(const std::string& entry, size_t index, bool defvalue) const
{
    const std::string& val(LowerCaseValue(value(entry, index)));
    if (val == "true" || val == "yes" || val == "1") {
        return true;
    }
    else if (val == "false" || val == "no" || val == "0") {
        return false;
    }
    else {
        return defvalue;
    }
}

//----------------------------------------------------------------------------
// Set the value of an entry.
//----------------------------------------------------------------------------

void ts::ConfigSection::set(const std::string& entry, const std::string& val)
{
    Entry& ent(_entries[entry]);
    ent.clear();
    ent.push_back(val);
}

//----------------------------------------------------------------------------
// Set the value of an entry.
//----------------------------------------------------------------------------

void ts::ConfigSection::set(const std::string& entry, const StringVector& val)
{
    _entries[entry] = val;
}

//----------------------------------------------------------------------------
// Set the boolean value of an entry.
//----------------------------------------------------------------------------

void ts::ConfigSection::set(const std::string& entry, bool val)
{
    this->set(entry, val ? "true" : "false");
}

//----------------------------------------------------------------------------
// Set the boolean value of an entry.
//----------------------------------------------------------------------------

void ts::ConfigSection::set(const std::string& entry, const std::vector <bool>& val)
{
    Entry& ent(_entries[entry]);
    ent.clear();
    for (size_t i = 0; i < val.size(); ++i) {
        ent.push_back(val[i] ? "true" : "false");
    }
}

//----------------------------------------------------------------------------
// Append values in an entry
//----------------------------------------------------------------------------

void ts::ConfigSection::append(const std::string& entry, const std::string& val)
{
    _entries[entry].push_back(val);
}

//----------------------------------------------------------------------------
// Append values in an entry
//----------------------------------------------------------------------------

void ts::ConfigSection::append(const std::string& entry, const StringVector& val)
{
    Entry& ent(_entries[entry]);
    ent.insert(ent.end(), val.begin(), val.end());
}

//----------------------------------------------------------------------------
// Append values in an entry
//----------------------------------------------------------------------------

void ts::ConfigSection::append(const std::string& entry, bool val)
{
    append(entry, val ? "true" : "false");
}

//----------------------------------------------------------------------------
// Append values in an entry
//----------------------------------------------------------------------------

void ts::ConfigSection::append(const std::string& entry, const std::vector <bool>& val)
{
    Entry& ent(_entries[entry]);
    for (size_t i = 0; i < val.size(); ++i) {
        ent.push_back(val[i] ? "true" : "false");
    }
}

//----------------------------------------------------------------------------
// Set the value of an entry from a string representation:
//    entryname = value [, value ...]
//----------------------------------------------------------------------------

void ts::ConfigSection::set(const std::string& line)
{
    size_t equal(line.find('='));

    if (equal != std::string::npos) {
        std::string name(line, 0, equal);
        std::string val(line, equal + 1);
        Trim(name);
        SplitString(_entries[name], val.c_str(), ',');
    }
}

//----------------------------------------------------------------------------
// Save the content of a section in a stream
//----------------------------------------------------------------------------

std::ostream& ts::ConfigSection::save(std::ostream& strm) const
{
    for (EntryMap::const_iterator ent = _entries.begin(); ent != _entries.end(); ++ent) {
        strm << ent->first << " =";
        for (size_t i = 0; i < ent->second.size(); ++i) {
            strm << (i == 0 ? " " : ", ") << ent->second[i];
        }
        strm << std::endl;
    }
    return strm;
}
