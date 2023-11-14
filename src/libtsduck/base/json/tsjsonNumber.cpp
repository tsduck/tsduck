//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsjsonNumber.h"
#include "tsTextFormatter.h"
#include "tsFloatUtils.h"


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
    output << toString();
}

bool ts::json::Number::toBoolean(bool defaultValue) const
{
    return false;
}

int64_t ts::json::Number::toInteger(int64_t defaultValue) const
{
    return _integer;
}

double ts::json::Number::toFloat(double defaultValue) const
{
    return _float;
}

bool ts::json::Number::isInteger() const
{
    return equal_float(double(_integer), _float);
}

ts::UString ts::json::Number::toString(const UString& defaultValue) const
{
    return isInteger() ? UString::Decimal(_integer, 0, true, UString()) : UString::Float(_float);
}

void ts::json::Number::clear()
{
    _integer = 0;
}
