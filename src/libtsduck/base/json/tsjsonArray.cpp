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

#include "tsjsonArray.h"
#include "tsjsonNull.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

ts::json::Type ts::json::Array::type() const
{
     return TypeArray;
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
    for (std::vector<ValuePtr>::const_iterator it = _value.begin(); it != _value.end(); ++it) {
        if (it != _value.begin()) {
            output << ",";
        }
        output << std::endl << ts::margin;
        (*it)->print(output);
    }

    // Unindent and closing sequence.
    output << std::endl << ts::unindent << ts::margin << "]";
}
