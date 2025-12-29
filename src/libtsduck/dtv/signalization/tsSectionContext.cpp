//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSectionContext.h"

// Virtual destructor.
ts::SectionContext::~SectionContext()
{
}

// The default implementation returns the PID parameter which was given to the constructor.
ts::PID ts::SectionContext::getPID() const
{
    return _pid;
}

// The default implementation returns the standards parameter which was given to the constructor.
ts::Standards ts::SectionContext::getStandards() const
{
    return _standards;
}

// The default implementation returns the CASID parameter which was given to the constructor.
ts::CASID ts::SectionContext::getCAS() const
{
    return _casid;
}
