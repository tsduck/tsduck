//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsjsonNull.h"
#include "tsTextFormatter.h"

// A general-purpose constant null JSON value.
ts::json::Null ts::json::NullValue;


//----------------------------------------------------------------------------
// Simple virtual methods
//----------------------------------------------------------------------------

ts::json::Type ts::json::Null::type() const
{
     return Type::Null;
}

bool ts::json::Null::isNull() const
{
     return true;
}

void ts::json::Null::print(TextFormatter& output) const
{
    output << "null";
}
