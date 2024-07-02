//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tspySyncReport.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::py::SyncReport::SyncReport(LogCallback log_callback, int max_severity) :
    ts::Report(max_severity),
    _log_callback(log_callback)
{
}

ts::py::SyncReport::~SyncReport()
{
}

//----------------------------------------------------------------------------
// Message logging method.
//----------------------------------------------------------------------------

void ts::py::SyncReport::writeLog(int severity, const UString&  message)
{
    if (_log_callback != nullptr) {
        _log_callback(severity, message.data(), message.size() * sizeof(UChar));
    }
}
