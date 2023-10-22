//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsjsonNumber.h"
#include "tsTextFormatter.h"


//----------------------------------------------------------------------------
// Simple virtual methods
//----------------------------------------------------------------------------

ts::json::Type ts::json::Number::type() const
{
    return Type::Number;
}

bool ts::json::Number::isNumber() const
{
    return true;
}

void ts::json::Number::print(TextFormatter& output) const
{
    output << UString::Decimal(_value, 0, true, UString());
}

bool ts::json::Number::toBoolean(bool defaultValue) const
{
    return false;
}

int64_t ts::json::Number::toInteger(int64_t defaultValue) const
{
    return _value;
}

ts::UString ts::json::Number::toString(const UString& defaultValue) const
{
    return UString::Decimal(_value, 0, true, UString());
}

void ts::json::Number::clear()
{
    _value = 0;
}
