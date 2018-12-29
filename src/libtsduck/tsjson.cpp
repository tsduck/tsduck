//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsjson.h"
TSDUCK_SOURCE;

// A general-purpose constant null JSON value.
const ts::json::Null ts::json::NullValue;


//----------------------------------------------------------------------------
// Default access to sub-component.
//----------------------------------------------------------------------------

const ts::json::Value& ts::json::Value::at(size_t index) const
{
    return NullValue;
}

const ts::json::Value& ts::json::Value::value(const UString& name) const
{
    return NullValue;
}


//----------------------------------------------------------------------------
// Conversion methods for strings.
//----------------------------------------------------------------------------

int64_t ts::json::String::toInteger(int64_t defaultValue) const
{
    int64_t i = 0;
    return _value.toInteger(i) ? i : defaultValue;
}

bool ts::json::String::toBoolean(bool defaultValue) const
{
    int i = 0;
    if (_value.similar(u"true") || _value.similar(u"yes") || _value.similar(u"on") || (_value.toInteger(i) && i != 0)) {
        return true;
    }
    else if (_value.similar(u"false") || _value.similar(u"no") || _value.similar(u"off") || (_value.toInteger(i) && i == 0)) {
        return false;
    }
    else {
        return defaultValue;
    }
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


//----------------------------------------------------------------------------
// Parse a JSON value (typically an object or array.
//----------------------------------------------------------------------------

bool ts::json::Parse(ValuePtr& value, const UStringList& lines, Report& report)
{
    TextParser parser(lines, report);
    return Parse(value, parser, true, report);
}

bool ts::json::Parse(ValuePtr& value, const UString& text, Report& report)
{
    TextParser parser(text, report);
    return Parse(value, parser, true, report);
}

bool ts::json::Parse(ValuePtr& value, TextParser& parser, bool jsonOnly, Report& report)
{
    value.clear();

    UString str;
    int64_t intVal;

    // Leading spaces are ignored.
    parser.skipWhiteSpace();

    // Look for one of the seven possible forms or JSON value.
    if (parser.match(u"null", true)) {
        value = new Null;
    }
    else if (parser.match(u"true", true)) {
        value = new True;
    }
    else if (parser.match(u"false", true)) {
        value = new False;
    }
    else if (parser.parseJSONStringLiteral(str)) {
        value = new String(str);
    }
    else if (parser.parseNumericLiteral(str, false, true)) {
        if (str.toInteger(intVal)) {
            value = new Number(intVal);
        }
        else {
            // Invalid integer,
            report.error(u"line %d: JSON floating-point numbers not yet supported, using \"null\" instead", {parser.lineNumber()});
            value = new Null;
        }
    }
    else if (parser.match(u"{", true)) {
        // Parse an object.
        value = new Object;
        // Loop on all fields of the object.
        for (;;) {
            parser.skipWhiteSpace();
            // Exit at end of object
            if (parser.match(u"}", true)) {
                break;
            }
            ValuePtr element;
            if (!parser.parseJSONStringLiteral(str) ||
                !parser.skipWhiteSpace() ||
                !parser.match(u":", true) ||
                !parser.skipWhiteSpace() ||
                !Parse(element, parser, false, report))
            {
                return false;
            }
            // Found field.
            value->add(str, element);
            parser.skipWhiteSpace();
            // Exit at end of object
            if (parser.match(u"}", true)) {
                break;
            }
            // Expect a comma before next field.
            if (!parser.match(u",", true)) {
                report.error(u"line %d: syntax error in JSON object, missing ','", {parser.lineNumber()});
                return false;
            }
        }
    }
    else if (parser.match(u"[", true)) {
        // Parse an array.
        value = new Array;
        // Loop on all elements of the array.
        for (;;) {
            parser.skipWhiteSpace();
            // Exit at end of array.
            if (parser.match(u"]", true)) {
                break;
            }
            ValuePtr element;
            if (!Parse(element, parser, false, report)) {
                return false;
            }
            // Found an element.
            value->set(element);
            parser.skipWhiteSpace();
            // Exit at end of array.
            if (parser.match(u"]", true)) {
                break;
            }
            // Expect a comma before next element
            if (!parser.match(u",", true)) {
                report.error(u"line %d: syntax error in JSON array, missing ','", {parser.lineNumber()});
                return false;
            }
        }
    }
    else {
        report.error(u"line %d: not a valid JSON value", {parser.lineNumber()});
        return false;
    }

    // Process text after the JSON value.
    if (jsonOnly) {
        // Nothing is allowed after the JSON value.
        parser.skipWhiteSpace();
        if (parser.eof()) {
            return true;
        }
        else {
            report.error(u"line %d: extraneous text after JSON value", {parser.lineNumber()});
            return false;
        }
    }
    else {
        // Do not parse further.
        return true;
    }
}
