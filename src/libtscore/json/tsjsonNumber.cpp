//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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

bool ts::json::Number::toBoolean(bool default_value) const
{
    return _valid_integer ? _integer != 0 : default_value;
}

int64_t ts::json::Number::toInteger(int64_t default_value) const
{
    return _valid_integer ? _integer : default_value;
}

double ts::json::Number::toFloat(double default_value) const
{
    return _float;
}

bool ts::json::Number::isInteger() const
{
    return _valid_integer && equal_float(double(_integer), _float);
}

ts::UString ts::json::Number::toString(const UString& default_value) const
{
    return isInteger() ? UString::Decimal(_integer, 0, true, UString()) : UString::Float(_float);
}

void ts::json::Number::clear()
{
    _valid_integer = false;
    _integer = 0;
    _float = 0.0;
}
