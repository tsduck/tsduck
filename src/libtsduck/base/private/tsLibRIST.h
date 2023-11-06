//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Safely include the librist definitions.
//!
//!  The librist library has a double versioning mechanism. The software
//!  itself has a version number. Then, the API definition has a distinct
//!  version number. When using conditional compilation for a new librist
//!  feature, we need to check the API version in symbol LIBRIST_API_VERSION.
//!
//!  librist version | librist API version
//!  --------------- | -------------------
//!  0.2.0           | 4.0.0
//!  0.2.2           | 4.1.1
//!  0.2.3           | 4.1.1
//!  0.2.4           | 4.1.1
//!  0.2.5           | 4.1.1
//!  0.2.7           | 4.2.0
//!  0.2.8           | 4.3.0
//!  0.2.9           | 4.3.0
//!  0.2.10          | 4.4.0
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

#if !defined(TS_NO_RIST)
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(documentation)

    // Common headers.
    #include <librist/librist.h>
    #include <librist/version.h>

    // SRP header was not protected for C++ before librist 0.2.7 (API version 4.2.0).
    #if LIBRIST_API_VERSION < LIBRIST_MAKE_API_VERSION(4, 2, 0)
    extern "C" {
    #endif
    #include <librist/librist_srp.h>
    #if LIBRIST_API_VERSION < LIBRIST_MAKE_API_VERSION(4, 2, 0)
    }
    #endif

    TS_POP_WARNING()
#endif
