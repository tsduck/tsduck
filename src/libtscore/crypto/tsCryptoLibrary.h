//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definitions for the system-specific cryptographic library.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include <bcrypt.h>
    #include "tsAfterStandardHeaders.h"
    #if defined(TS_MSC)
        #pragma comment(lib, "bcrypt.lib")
    #endif
#else
    #include "tsOpenSSL.h"
#endif

namespace ts {
    //!
    //! Get the name and version of the underlying cryptographic library.
    //! @return The cryptographic library name and version.
    //! @ingroup crypto
    //!
    TSCOREDLL UString GetCryptographicLibraryVersion();
}
