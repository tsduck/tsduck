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

#include "tsjsonValue.h"
#include "tsjsonNull.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Simple virtual methods
//----------------------------------------------------------------------------

bool ts::json::Value::isNull()   const { return false; }
bool ts::json::Value::isTrue()   const { return false; }
bool ts::json::Value::isFalse()  const { return false; }
bool ts::json::Value::isNumber() const { return false; }
bool ts::json::Value::isString() const { return false; }
bool ts::json::Value::isObject() const { return false; }
bool ts::json::Value::isArray()  const { return false; }

bool ts::json::Value::toBoolean(bool defaultValue) const
{
    return defaultValue;
}

int64_t ts::json::Value::toInteger(int64_t defaultValue) const
{
    return defaultValue;
}

ts::UString ts::json::Value::toString(const UString& defaultValue) const
{
    return defaultValue;
}

void ts::json::Value::clear()
{
}

size_t ts::json::Value::size() const
{
    return 0;
}

void ts::json::Value::getNames(UStringList& names) const
{
    names.clear();
}

void ts::json::Value::remove(const UString& name)
{
}

ts::json::ValuePtr ts::json::Value::extract(const UString& name)
{
    return ValuePtr();
}

void ts::json::Value::add(const UString& name, const ValuePtr& value)
{
}

size_t ts::json::Value::set(const ValuePtr& value, size_t index)
{
    return 0;
}

void ts::json::Value::erase(size_t index, size_t count)
{
}

ts::json::ValuePtr ts::json::Value::extractAt(size_t index)
{
    return ValuePtr();
}

const ts::json::Value& ts::json::Value::at(size_t index) const
{
    return NullValue;
}

const ts::json::Value& ts::json::Value::value(const UString& name) const
{
    return NullValue;
}


//----------------------------------------------------------------------------
// Format the value as JSON text.
//----------------------------------------------------------------------------

ts::UString ts::json::Value::printed(size_t indent, Report& report) const
{
    TextFormatter out(report);
    out.setIndentSize(indent);
    out.setString();
    print(out);
    UString str;
    out.getString(str);
    return str;
}
