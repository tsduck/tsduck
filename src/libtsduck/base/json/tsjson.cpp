//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsjson.h"
#include "tsjsonNull.h"
#include "tsjsonTrue.h"
#include "tsjsonFalse.h"
#include "tsjsonNumber.h"
#include "tsjsonString.h"
#include "tsjsonObject.h"
#include "tsjsonArray.h"
#include "tsTextParser.h"


//----------------------------------------------------------------------------
// Enumeration description of ts::json::Type.
//----------------------------------------------------------------------------

const ts::Enumeration& ts::json::TypeEnum()
{
    // Thread-safe init-safe static data pattern:
    static const Enumeration data {
        {u"Null literal",  ts::json::Type::Null},
        {u"True literal",  ts::json::Type::True},
        {u"False literal", ts::json::Type::False},
        {u"string",        ts::json::Type::String},
        {u"number",        ts::json::Type::Number},
        {u"object",        ts::json::Type::Object},
        {u"array",         ts::json::Type::Array},
    };
    return data;
}


//----------------------------------------------------------------------------
// Create a JSON value by type.
//----------------------------------------------------------------------------

ts::json::ValuePtr ts::json::Bool(bool value)
{
    if (value) {
        return std::make_shared<True>();
    }
    else {
        return std::make_shared<False>();
    }
}

ts::json::ValuePtr ts::json::Factory(Type type, const UString& value)
{
    switch (type) {
        case Type::True:
            return std::make_shared<True>();
        case Type::False:
            return std::make_shared<False>();
        case Type::String:
            return std::make_shared<String>(value);
        case Type::Number: {
            int64_t ivalue = 0;
            double fvalue = 0.0;
            if (value.toInteger(ivalue, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
                return std::make_shared<Number>(ivalue);
            }
            else if (value.toFloat(fvalue)) {
                return std::make_shared<Number>(fvalue);
            }
            else {
                return std::make_shared<Null>();
            }
        }
        case Type::Object:
            return std::make_shared<Object>();
        case Type::Array:
            return std::make_shared<Array>();
        case Type::Null:
        default:
            return std::make_shared<Null>();
    }
}


//----------------------------------------------------------------------------
// Check if a "file name" is in fact inline JSON content.
//----------------------------------------------------------------------------

bool ts::json::IsInlineJSON(const UString& name)
{
    const size_t len = name.length();
    size_t start = 0;
    while (start < len && IsSpace(name[start])) {
        ++start;
    }
    return start < len && (name[start] == '{' || name[start] == '[');
}


//----------------------------------------------------------------------------
// Load a JSON value (typically an object or array) from a text file.
//----------------------------------------------------------------------------

bool ts::json::LoadFile(ValuePtr& value, const UString& filename, Report& report)
{
    TextParser parser(report);
    bool ok = true;
    if (filename.empty() || filename == u"-") {
        ok = parser.loadStream(std::cin);
    }
    else if (IsInlineJSON(filename)) {
        parser.loadDocument(filename);
    }
    else {
        ok = parser.loadFile(filename);
    }
    return ok && Parse(value, parser, true, report);
}

bool ts::json::LoadStream(ValuePtr& value, std::istream& strm, Report& report)
{
    TextParser parser(report);
    return parser.loadStream(strm) && Parse(value, parser, true, report);
}


//----------------------------------------------------------------------------
// Parse a JSON value (typically an object or array).
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
    value.reset();

    UString str;
    int64_t intVal = 0;
    double floatVal= 0.0;

    // Leading spaces are ignored.
    parser.skipWhiteSpace();

    // Look for one of the seven possible forms or JSON value.
    if (parser.match(u"null", true)) {
        value = std::make_shared<Null>();
    }
    else if (parser.match(u"true", true)) {
        value = std::make_shared<True>();
    }
    else if (parser.match(u"false", true)) {
        value = std::make_shared<False>();
    }
    else if (parser.parseJSONStringLiteral(str)) {
        value = std::make_shared<String>(str);
    }
    else if (parser.parseNumericLiteral(str, false, true)) {
        if (str.toInteger(intVal, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
            value = std::make_shared<Number>(intVal);
        }
        else if (str.toFloat(floatVal)) {
            value = std::make_shared<Number>(floatVal);
        }
        else {
            // Invalid integer,
            report.error(u"line %d: JSON floating-point numbers not yet supported, using \"null\" instead", parser.lineNumber());
            value = std::make_shared<Null>();
        }
    }
    else if (parser.match(u"{", true)) {
        // Parse an object.
        value = std::make_shared<Object>();
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
                report.error(u"line %d: syntax error in JSON object, missing ','", parser.lineNumber());
                return false;
            }
        }
    }
    else if (parser.match(u"[", true)) {
        // Parse an array.
        value = std::make_shared<Array>();
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
                report.error(u"line %d: syntax error in JSON array, missing ','", parser.lineNumber());
                return false;
            }
        }
    }
    else {
        report.error(u"line %d: not a valid JSON value", parser.lineNumber());
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
            report.error(u"line %d: extraneous text after JSON value", parser.lineNumber());
            return false;
        }
    }
    else {
        // Do not parse further.
        return true;
    }
}
