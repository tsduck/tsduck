//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSwitchableReport.h"


// Constructor
ts::SwitchableReport::SwitchableReport(Report& delegate, bool on) :
    Report(std::numeric_limits<int>::max()), // actual logging will be limited in delegate
    _on(on),
    _delegate(delegate)
{
}

// Message logging method.
void ts::SwitchableReport::writeLog(int severity, const UString &msg)
{
    if (_on) {
        _delegate.log(severity, msg);
    }
}
