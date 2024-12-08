//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDescriptorContext.h"

// Virtual destructor.
ts::DescriptorContext::~DescriptorContext()
{
}

// The default implementation returns the TID parameter which was given to the constructor.
ts::TID ts::DescriptorContext::getTableId() const
{
    return _tid;
}

// The default implementation returns the standards parameter which was given to the constructor.
ts::Standards ts::DescriptorContext::getStandards() const
{
    return _standards;
}

// The default implementation returns the CASID parameter which was given to the constructor.
ts::CASID ts::DescriptorContext::getCAS() const
{
    return _casid;
}

// The default implementation searches nothing and returns the REGID and PDS values which were given to the constructor.
bool ts::DescriptorContext::getPrivateIds(REGID* regid, PDS* pds) const
{
    if (regid != nullptr) {
        *regid = _regid;
    }
    if (pds != nullptr) {
        *pds = _pds;
    }
    return false; // PDS takes precedence over REGID if both are returned.
}
