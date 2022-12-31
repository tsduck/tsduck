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

#include "tsjsonArray.h"
#include "tsjsonString.h"
#include "tsjsonNumber.h"
#include "tsjsonNull.h"
#include "tsTextFormatter.h"


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

ts::json::Type ts::json::Array::type() const
{
     return Type::Array;
}
bool ts::json::Array::isArray() const
{
     return true;
}
size_t ts::json::Array::size() const
{
     return _value.size();
}
void ts::json::Array::clear()
{
    _value.clear();
}

//----------------------------------------------------------------------------
// Access to an array element.
//----------------------------------------------------------------------------

const ts::json::Value& ts::json::Array::at(size_t index) const
{
    if (index >= _value.size() || _value[index].isNull()) {
        return NullValue;
    }
    else {
        return *_value[index];
    }
}

ts::json::Value& ts::json::Array::at(size_t index)
{
    if (index >= _value.size() || _value[index].isNull()) {
        return NullValue;
    }
    else {
        return *_value[index];
    }
}

size_t ts::json::Array::set(const ValuePtr& value, size_t index)
{
    // If the pointer is null, explicitly create a "null" value.
    const ValuePtr actualValue(value.isNull() ? ValuePtr(new Null) : value);

    if (index < _value.size()) {
        _value[index] = actualValue;
        return index;
    }
    else {
        _value.push_back(actualValue);
        return _value.size() - 1;
    }
}

size_t ts::json::Array::set(int64_t value, size_t index)
{
    return set(new Number(value), index);
}

size_t ts::json::Array::set(const UString& value, size_t index)
{
    return set(new String(value), index);
}

void ts::json::Array::erase(size_t index, size_t count)
{
    if (index < _value.size() && count > 0) {
        _value.erase(_value.begin() + index, _value.begin() + std::min(index + count, _value.size()));
    }
}

ts::json::ValuePtr ts::json::Array::extractAt(size_t index)
{
    ValuePtr result;
    if (index < _value.size()) {
        result = _value[index];
        _value.erase(_value.begin() + index, _value.begin() + index + 1);
    }
    return result;
}


//----------------------------------------------------------------------------
// Format a JSON array.
//----------------------------------------------------------------------------

void ts::json::Array::print(TextFormatter& output) const
{
    // Opening sequence, then indent.
    output << "[" << ts::indent;

    // Format all fields.
    bool first = true;
    for (const auto& it : _value) {
        if (!first) {
            output << ",";
        }
        output << ts::endl << ts::margin;
        it->print(output);
        first = false;
    }

    // Unindent and closing sequence.
    output << ts::endl << ts::unindent << ts::margin << "]";
}


//----------------------------------------------------------------------------
// Split and validate a query path.
//----------------------------------------------------------------------------

bool ts::json::Array::splitPath(const UString& path, size_t& index, UString& next)
{
    index = 0;
    next.clear();

    if (path.empty()) {
        return true; // root object.
    }
    else if (path.front() != u'[') {
        return false; // not an array index syntax => error.
    }
    else {
        // Extract index.
        size_t end = path.find(u']', 1);
        if (end >= path.size()) {
            return false; // no closing ']', invalid index syntax
        }
        else if (end == 1) {
            index = NPOS; // syntax '[]' means add at end of array
        }
        else if (!path.substr(1, end - 1).toInteger(index, u",")) {
            return false; // invalid index syntax
        }

        // Skip separators, point to next field name or array index.
        while (++end < path.size() && path[end] == u'.') {
        }
        next = path.substr(end);
        return true;
    }
}


//----------------------------------------------------------------------------
// Deep query of an object, constant version.
//----------------------------------------------------------------------------

const ts::json::Value& ts::json::Array::query(const UString& path) const
{
    size_t index = 0;
    UString next;

    if (path.empty()) {
        return *this; // root object.
    }
    else if (!splitPath(path, index, next)) {
        return NullValue; // error
    }
    else if (index >= _value.size() || _value[index].isNull()) {
        return NullValue; // non existent element.
    }
    else {
        return _value[index]->query(next); // recursive query
    }
}


//----------------------------------------------------------------------------
// Deep query of an object, modifiable version.
//----------------------------------------------------------------------------

ts::json::Value& ts::json::Array::query(const UString& path, bool create, Type type)
{
    size_t index = 0;
    UString next;

    if (path.empty()) {
        return *this; // root object.
    }
    else if (!splitPath(path, index, next)) {
        return NullValue; // error
    }
    else if (index < _value.size() && !_value[index].isNull()) {
        return _value[index]->query(next, create, type); // recursive query
    }
    else if (create) {
        // Determine next field type
        ValuePtr val(Factory(next.empty() ? type : (next.startWith(u"[") ? Type::Array : Type::Object)));
        set(val, index);
        return val->query(next, create, type); // recursive query
    }
    else {
        // Non existent field and don't create it.
        return NullValue;
    }
}
