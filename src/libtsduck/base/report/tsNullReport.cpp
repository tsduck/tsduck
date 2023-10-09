//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNullReport.h"

// Define singleton instance
TS_DEFINE_SINGLETON(ts::NullReport);

// Does nothing, really nothing at all.
ts::NullReport::NullReport() {}
void ts::NullReport::writeLog(int severity, const UString &msg) {}
