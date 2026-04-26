//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReporterGuard.h"


//----------------------------------------------------------------------------
// Constructor: replace the Report.
//----------------------------------------------------------------------------

ts::ReporterGuard::ReporterGuard(ReporterBase& base, Report* replacement) :
    _base(base),
    _previous(base.setReport(replacement))
{
}


//----------------------------------------------------------------------------
// Destructor: restore the previous report.
//----------------------------------------------------------------------------

ts::ReporterGuard::~ReporterGuard()
{
    _base.setReport(_previous);
}
