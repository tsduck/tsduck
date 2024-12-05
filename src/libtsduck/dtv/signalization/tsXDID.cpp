//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsXDID.h"


// Convert to a string object.
ts::UString ts::XDID::toString() const
{
    return isExtension() && xdid() != DID_NULL ? UString::Format(u"%X:%X", did(), xdid()) : UString::Format(u"%X", did());
}
