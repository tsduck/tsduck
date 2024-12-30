//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCerrReport.h"
#include "tsEnvironment.h"

// Get the instance of the singleton.
ts::CerrReport& ts::CerrReport::Instance()
{
    // Thread-safe init-safe static data pattern:
    static CerrReport singleton;
    return singleton;
}

// Constructor.
ts::CerrReport::CerrReport()
{
    int severity = 0;
    if (GetEnvironment(u"TS_CERR_DEBUG_LEVEL").toInteger(severity)) {
        setMaxSeverity(severity);
    }
}

// Message logging method.
void ts::CerrReport::writeLog(int severity, const UString &msg)
{
    std::cerr << "* " << Severity::Header(severity) << msg << std::endl;
    std::cerr.flush();
}
