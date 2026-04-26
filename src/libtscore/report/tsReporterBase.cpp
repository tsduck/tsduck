//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReporterBase.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Virtual destructor.
//----------------------------------------------------------------------------

ts::ReporterBase::~ReporterBase()
{
}


//----------------------------------------------------------------------------
// Access the Report which is associated with this object.
//----------------------------------------------------------------------------

ts::Report& ts::ReporterBase::report() const
{
    if (_mute) {
        return NULLREP;
    }
    else if (_delegate != nullptr) {
        return _delegate->report();
    }
    else if (_report != nullptr) {
        return *_report;
    }
    else {
        return NULLREP;
    }
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

ts::ReporterBase* ts::ReporterBase::setReport(ReporterBase* delegate)
{
    ReporterBase* previous = _delegate;
    _delegate = delegate;
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
