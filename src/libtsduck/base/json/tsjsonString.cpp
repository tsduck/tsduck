//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsjsonString.h"
#include "tsTextFormatter.h"


//----------------------------------------------------------------------------
// Simple virtual methods
//----------------------------------------------------------------------------

ts::json::Type ts::json::String::type() const
{
    return Type::String;
}

bool ts::json::String::isString() const
{
    return true;
}

void ts::json::String::print(TextFormatter& output) const
{
    output << '"' << _value.toJSON() << '"';
}

int64_t ts::json::String::toInteger(int64_t defaultValue) const
{
    int64_t i = 0;
    return _value.toInteger(i) ? i : defaultValue;
}

double ts::json::String::toFloat(double defaultValue) const
{
    double f = 0.0;
    return _value.toFloat(f) ? f : defaultValue;
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

ts::UString ts::json::String::toString(const UString& defaultValue) const
{
    return _value;
}

size_t ts::json::String::size() const
{
    return _value.size();
}

void ts::json::String::clear()
{
    _value.clear();
}
