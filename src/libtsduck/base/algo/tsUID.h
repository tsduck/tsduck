//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A function which generates 64-bit UID, unique integer
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Generate a new 64-bit UID, unique integer.
    //! @ingroup cpp
    //! @return A new unique 64-bit value.
    //!
    uint64_t UID();
}
