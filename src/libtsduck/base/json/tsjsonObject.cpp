//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsjsonObject.h"
#include "tsjsonNull.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Simple virtual methods
//----------------------------------------------------------------------------

ts::json::Type ts::json::Object::type() const
{
     return TypeObject;
}
bool ts::json::Object::isObject() const
{
     return true;
}
size_t ts::json::Object::size() const
{
     return _fields.size();
}
void ts::json::Object::clear()
{
     _fields.clear();
}


//----------------------------------------------------------------------------
// Manage object fields.
//----------------------------------------------------------------------------

const ts::json::Value& ts::json::Object::value(const UString& name) const
{
    std::map<UString, ValuePtr>::const_iterator it = _fields.find(name);
    if (it == _fields.end() || it->second.isNull()) {
        return NullValue;
    }
    else {
        return *it->second;
    }
}

void ts::json::Object::remove(const UString& name)
{
    _fields.erase(name);
}

ts::json::ValuePtr ts::json::Object::extract(const UString& name)
{
    ValuePtr result;
    std::map<UString, ValuePtr>::const_iterator it = _fields.find(name);
    if (it != _fields.end()) {
        result = it->second;
        _fields.erase(name);
    }
    return result;
}

void ts::json::Object::add(const UString& name, const ValuePtr& value)
{
    // If the pointer is null, explicitly create a "null" value.
    _fields[name] = value.isNull() ? ValuePtr(new Null) : value;
}

void ts::json::Object::getNames(UStringList& names) const
{
    names.clear();
    for (std::map<UString, ValuePtr>::const_iterator it = _fields.begin(); it != _fields.end(); ++it) {
        names.push_back(it->first);
    }
}


//----------------------------------------------------------------------------
// Format a JSON object.
//----------------------------------------------------------------------------

void ts::json::Object::print(TextFormatter& output) const
{
    // Opening sequence, then indent.
    output << "{" << ts::indent;

    // Format all fields.
    for (std::map<UString, ValuePtr>::const_iterator it = _fields.begin(); it != _fields.end(); ++it) {
        if (it != _fields.begin()) {
            output << ",";
        }
        output << std::endl << ts::margin << '"' << it->first.toJSON() << "\": ";
        it->second->print(output);
    }

    // Unindent and closing sequence.
    output << std::endl << ts::unindent << ts::margin << "}";
}
