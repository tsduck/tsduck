//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tspyAsyncReport.h"

//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::py::AsyncReport::AsyncReport(LogCallback log_callback, int max_severity, const AsyncReportArgs& args) :
    ts::AsyncReport(max_severity, args),
    _log_callback(log_callback)
{
}

ts::py::AsyncReport::~AsyncReport()
{
}

//----------------------------------------------------------------------------
// Message logging method.
//----------------------------------------------------------------------------

void ts::py::AsyncReport::asyncThreadLog(int severity, const UString& message)
{
    if (_log_callback != nullptr) {
        _log_callback(severity, message.data(), message.size() * sizeof(UChar));
    }
}
