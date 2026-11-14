//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractRegulator.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::AbstractRegulator::AbstractRegulator(Report* report, int log_level) :
    ReporterBase(report),
    _log_level(log_level)
{
}

ts::AbstractRegulator::AbstractRegulator(ReporterBase* delegate, int log_level) :
    ReporterBase(delegate),
    _log_level(log_level)
{
}

ts::AbstractRegulator::~AbstractRegulator()
{
}
