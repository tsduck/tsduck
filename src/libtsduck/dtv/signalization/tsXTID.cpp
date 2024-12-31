//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsXTID.h"


// Convert to a string object.
ts::UString ts::XTID::toString() const
{
    return isLongSection() ? UString::Format(u"%X:%X", tid(), tidExt()) : UString::Format(u"%X", tid());
}
