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

#include "tsjsonObject.h"
#include "tsjsonString.h"
#include "tsjsonNumber.h"
#include "tsjsonNull.h"
#include "tsTextFormatter.h"


//----------------------------------------------------------------------------
// Simple virtual methods
//----------------------------------------------------------------------------

ts::json::Type ts::json::Object::type() const
{
     return Type::Object;
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

ts::json::ValuePtr ts::json::Object::valuePtr(const UString& name)
{
    const auto it = _fields.find(name);
    return it == _fields.end() || it->second.isNull() ? ValuePtr() : it->second;
}

const ts::json::Value& ts::json::Object::value(const UString& name) const
{
    const auto it = _fields.find(name);
    if (it == _fields.end() || it->second.isNull()) {
        return NullValue;
    }
    else {
        return *it->second;
    }
}

ts::json::Value& ts::json::Object::value(const UString& name, bool create, Type type)
{
    const auto it = _fields.find(name);
    if (it != _fields.end() && !it->second.isNull()) {
        return *it->second;
    }
    else if (create) {
        ValuePtr val(Factory(type));
        _fields[name] = val;
        return *val;
    }
    else {
        return NullValue;
    }
}

void ts::json::Object::remove(const UString& name)
{
    _fields.erase(name);
}

ts::json::ValuePtr ts::json::Object::extract(const UString& name)
{
    ValuePtr result;
    const auto it = _fields.find(name);
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

void ts::json::Object::add(const UString& name, int64_t value)
{
    add(name, ValuePtr(new Number(value)));
}

void ts::json::Object::add(const UString& name, const UString& value)
{
    add(name, ValuePtr(new String(value)));
}

void ts::json::Object::getNames(UStringList& names) const
{
    names.clear();
    for (const auto& it : _fields) {
        names.push_back(it.first);
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
    bool first = true;
    for (const auto& it : _fields) {
        if (!first) {
            output << ",";
        }
        output << ts::endl << ts::margin << '"' << it.first.toJSON() << "\": ";
        it.second->print(output);
        first = false;
    }

    // Unindent and closing sequence.
    output << ts::endl << ts::unindent << ts::margin << "}";
}


//----------------------------------------------------------------------------
// Split and validate a query path.
//----------------------------------------------------------------------------

bool ts::json::Object::splitPath(const UString& path, UString& field, UString& next)
{
    field.clear();
    next.clear();

    if (path.empty()) {
        return true; // root object.
    }
    else if (path.front() == u'[') {
        return false; // array syntax => error.
    }
    else {
        // Extract first field name.
        size_t end = std::min(path.size(), std::min(path.find(u'.'), path.find(u'[')));
        field = path.substr(0, end);

        // Skip separators, point to next field name or array index.
        while (end < path.size() && path[end] == u'.') {
            ++end;
        }
        next = path.substr(end);
        return true;
    }
}


//----------------------------------------------------------------------------
// Deep query of an object, constant version.
//----------------------------------------------------------------------------

const ts::json::Value& ts::json::Object::query(const UString& path) const
{
    UString field, next;

    if (!splitPath(path, field, next)) {
        return NullValue; // error
    }
    else if (field.empty()) {
        return *this; // root object.
    }
    else {
        // Search first field.
        const auto it = _fields.find(field);
        if (it == _fields.end() || it->second.isNull()) {
            return NullValue; // field does not exist
        }
        else {
            return it->second->query(next); // recursive query
        }
    }
}


//----------------------------------------------------------------------------
// Deep query of an object, modifiable version.
//----------------------------------------------------------------------------

ts::json::Value& ts::json::Object::query(const UString& path, bool create, Type type)
{
    UString field, next;

    if (!splitPath(path, field, next)) {
        return NullValue; // error
    }
    else if (field.empty()) {
        return *this; // root object.
    }
    else {
        // Search first field.
        const auto it = _fields.find(field);
        if (it != _fields.end() && !it->second.isNull()) {
            return it->second->query(next, create, type); // recursive query
        }
        else if (create) {
            // Determine next field type
            ValuePtr val(Factory(next.empty() ? type : (next.startWith(u"[") ? Type::Array : Type::Object)));
            _fields[field] = val;
            return val->query(next, create, type); // recursive query
        }
        else {
            // Non existent field and don't create it.
            return NullValue;
        }
    }
}
