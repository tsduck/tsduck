//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
    #include "tsBeforeStandardHeaders.h"
    #include <openssl/opensslv.h>
    #include <openssl/evp.h>
    #include <openssl/err.h>
    #include "tsAfterStandardHeaders.h"
    #if !defined(OPENSSL_VERSION_MAJOR) // before v3
        #define OPENSSL_VERSION_MAJOR (OPENSSL_VERSION_NUMBER >> 28)
    #endif
    #if !defined(OPENSSL_VERSION_MINOR) // before v3
        #define OPENSSL_VERSION_MINOR ((OPENSSL_VERSION_NUMBER >> 20) & 0xFF)
    #endif
    #if OPENSSL_VERSION_MAJOR >= 3
        #include "tsBeforeStandardHeaders.h"
        #include <openssl/core_names.h>
        #include <openssl/provider.h>
        #include "tsAfterStandardHeaders.h"
    #endif
#endif

namespace ts {
    //!
    //! Get the name and version of the underlying cryptographic library.
    //! @return The cryptographic library name and version.
    //! @ingroup crypto
    //!
    TSDUCKDLL UString GetCryptographicLibraryVersion();
}
