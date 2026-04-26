//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReporterBase.h"


//----------------------------------------------------------------------------
// Virtual destructor.
//----------------------------------------------------------------------------

ts::ReporterBase::~ReporterBase()
{
}


//----------------------------------------------------------------------------
// Associate this object with another Report to log errors.
//----------------------------------------------------------------------------

ts::Report* ts::ReporterBase::setReport(Report* report)
{
    Report* previous = _report;
    _report = report;
    return previous;
}


//----------------------------------------------------------------------------
// Temporarily mute the associated report.
//----------------------------------------------------------------------------

bool ts::ReporterBase::muteReport(bool mute)
{
    const bool previous = _mute;
    _mute = mute;
    return previous;
}
