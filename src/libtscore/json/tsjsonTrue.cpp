//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsjsonTrue.h"
#include "tsTextFormatter.h"


//----------------------------------------------------------------------------
// Simple virtual methods
//----------------------------------------------------------------------------

ts::json::Type ts::json::True::type() const
{
    return Type::True;
}

bool ts::json::True::isTrue() const
{
    return true;
}

void ts::json::True::print(TextFormatter& output) const
{
    output << "true";
}

bool ts::json::True::toBoolean(bool default_value) const
{
    return true;
}

int64_t ts::json::True::toInteger(int64_t default_value) const
{
    return 1;
}

double ts::json::True::toFloat(double default_value) const
{
    return 1.0;
}

ts::UString ts::json::True::toString(const UString& default_value) const
{
    return u"true";
}
