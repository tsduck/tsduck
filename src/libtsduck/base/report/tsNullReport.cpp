//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNullReport.h"

// Get the instance of the singleton.
ts::NullReport& ts::NullReport::Instance()
{
    // Thread-safe init-safe static data pattern:
    static NullReport singleton;
    return singleton;
}

// Does nothing, really nothing at all.
void ts::NullReport::writeLog(int severity, const UString &msg)
{
}
