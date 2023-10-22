//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReportWithPrefix.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::ReportWithPrefix::ReportWithPrefix(ts::Report& report, const UString& prefix) :
    Report(report.maxSeverity()),
    _report(report),
    _prefix(prefix)
{
}


//----------------------------------------------------------------------------
// Report interface.
//----------------------------------------------------------------------------

void ts::ReportWithPrefix::writeLog(int severity, const UString& msg)
{
    _report.log(severity, _prefix + msg);
}

void ts::ReportWithPrefix::setMaxSeverity(int level)
{
    // Set in superclass.
    Report::setMaxSeverity(level);

    // Propagate to redirected report.
    _report.setMaxSeverity(level);
}
