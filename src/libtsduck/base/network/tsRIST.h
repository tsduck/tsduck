//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Reliable Internet Stream Transport (RIST) definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Get the version of the RIST library.
    //! @return A string describing the RIST library version (or the lack of RIST support).
    //!
    TSDUCKDLL UString GetRISTLibraryVersion();
}
