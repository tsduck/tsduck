//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsjsonValue.h"
#include "tsjsonNull.h"
#include "tsTextFormatter.h"


//----------------------------------------------------------------------------
// Default (empty) implementation of simple virtual methods
//----------------------------------------------------------------------------

bool ts::json::Value::isNull()    const { return false; }
bool ts::json::Value::isTrue()    const { return false; }
bool ts::json::Value::isFalse()   const { return false; }
bool ts::json::Value::isNumber()  const { return false; }
bool ts::json::Value::isInteger() const { return false; }
bool ts::json::Value::isString()  const { return false; }
bool ts::json::Value::isObject()  const { return false; }
bool ts::json::Value::isArray()   const { return false; }

bool ts::json::Value::toBoolean(bool defaultValue) const { return defaultValue; }
int64_t ts::json::Value::toInteger(int64_t defaultValue) const { return defaultValue; }
double ts::json::Value::toFloat(double defaultValue) const { return defaultValue; }
ts::UString ts::json::Value::toString(const UString& defaultValue) const { return defaultValue; }

void ts::json::Value::clear() {}
void ts::json::Value::erase(size_t index, size_t count) {}
void ts::json::Value::remove(const UString& name) {}

size_t ts::json::Value::size() const { return 0; }
void ts::json::Value::getNames(UStringList& names) const { names.clear(); }
ts::json::ValuePtr ts::json::Value::extract(const UString& name) { return ValuePtr(); }

void ts::json::Value::addValue(const UString& name, const ValuePtr& value) {}
void ts::json::Value::addInteger(const UString& name, int64_t value) {}
void ts::json::Value::addFloat(const UString& name, double value) {}
void ts::json::Value::addString(const UString& name, const UString& value) {}

size_t ts::json::Value::setValue(const ValuePtr& value, size_t index) { return 0; }
size_t ts::json::Value::setInteger(int64_t value, size_t index) { return 0; }
size_t ts::json::Value::setFloat(double value, size_t index) { return 0; }
size_t ts::json::Value::setString(const UString& value, size_t index) { return 0; }

ts::json::ValuePtr ts::json::Value::extractAt(size_t index) { return ValuePtr(); }
const ts::json::Value& ts::json::Value::at(size_t index) const { return NullValue; }
ts::json::Value& ts::json::Value::at(size_t index) { return NullValue; }
const ts::json::Value& ts::json::Value::value(const UString& name) const { return NullValue; }
ts::json::Value& ts::json::Value::value(const UString& name, bool create, Type type) { return NullValue; }
ts::json::ValuePtr ts::json::Value::valuePtr(const UString& name) { return ValuePtr(); }

const ts::json::Value& ts::json::Value::query(const UString& path) const
{
    return path.empty() ? *this : NullValue;
}

ts::json::Value& ts::json::Value::query(const UString& path, bool create, Type type)
{
    return path.empty() ? *this : NullValue;
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
    return out.toString();
}

ts::UString ts::json::Value::oneLiner(Report& report) const
{
    TextFormatter out(report);
    out.setString();
    out.setEndOfLineMode(TextFormatter::EndOfLineMode::SPACING);
    print(out);
    return out.toString();
}


//----------------------------------------------------------------------------
// Save the value as a JSON file.
//----------------------------------------------------------------------------

bool ts::json::Value::save(const fs::path& fileName, size_t indent, bool stdOutputIfEmpty, Report& report)
{
    TextFormatter out(report);
    out.setIndentSize(indent);

    if (stdOutputIfEmpty && (fileName.empty() || fileName == u"-")) {
        out.setStream(std::cout);
    }
    else if (!out.setFile(fileName)) {
        return false;
    }

    print(out);

    // All JSON objects print their value without end-of-line.
    out << std::endl;
    out.close();
    return true;
}
