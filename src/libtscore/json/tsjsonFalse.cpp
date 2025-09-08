//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsjsonFalse.h"
#include "tsTextFormatter.h"


//----------------------------------------------------------------------------
// Simple virtual methods
//----------------------------------------------------------------------------

ts::json::Type ts::json::False::type() const
{
    return Type::False;
}

bool ts::json::False::isFalse() const
{
    return true;
}

void ts::json::False::print(TextFormatter& output) const
{
    output << "false";
}

bool ts::json::False::toBoolean(bool default_value) const
{
    return false;
}

int64_t ts::json::False::toInteger(int64_t default_value) const
{
    return 0;
}

double ts::json::False::toFloat(double default_value) const
{
    return 0.0;
}

ts::UString ts::json::False::toString(const UString& default_value) const
{
    return u"false";
}
