//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup cpp
//!  Technical header to include before including standard and system headers.
//!
//!  This header file cleans up the compilation state to prepare for system
//!  headers. These system headers may generate spurious warnings or require
//!  additional definitions.
//!
//!  The following example illustrates the mandatory way to include system headers:
//!
//!  @code
//!  #include "tsBeforeStandardHeaders.h"
//!  #include <system-header-1.h>
//!  #include <system-header-2.h>
//!  ...
//!  #include "tsAfterStandardHeaders.h"
//!  @endcode
//!
//!  @see tsAfterStandardHeaders.h
//!
//----------------------------------------------------------------------------

// Do not use "#pragma once", can be used multiple times on purpose.

// Enforce inclusion tsPlatform.h but avoid recursion.
#if !defined(TS_ADDRESS_BITS)
    #include "tsPlatform.h"
#endif

// Windows specific settings
#if defined(TS_WINDOWS) && !defined(DOXYGEN)
    #if !defined(_CRT_SECURE_NO_DEPRECATE)
        #define _CRT_SECURE_NO_DEPRECATE 1
    #endif
    #if !defined(_CRT_NONSTDC_NO_DEPRECATE)
        #define _CRT_NONSTDC_NO_DEPRECATE 1
    #endif
    #if !defined(WIN32_LEAN_AND_MEAN)
        #define WIN32_LEAN_AND_MEAN 1  // Exclude rarely-used stuff from Windows headers
    #endif
#endif

// Large file system (LFS) support on Linux.
#if defined(TS_LINUX) && !defined(DOXYGEN)
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(reserved-id-macro)
    #if !defined(_LARGEFILE_SOURCE)
        #define _LARGEFILE_SOURCE 1
    #endif
    #if !defined(_LARGEFILE64_SOURCE)
        #define _LARGEFILE64_SOURCE 1
    #endif
    #if !defined(_FILE_OFFSET_BITS)
        #define _FILE_OFFSET_BITS 64
    #endif
    TS_POP_WARNING()
#endif

// Before including system headers, we must temporarily suspend some compilation warnings.
// This is especially true for Windows which trigger tons of warnings.
// The normal warning reporting is restored after inclusion.
// See the restoration section in tsAfterStandardHeaders.h.
TS_PUSH_WARNING()
TS_MSC_NOWARNING(4193 4244 4263 4264 4668 4774 5026 5027 5031 5032 5054 5204 5262 5266)
TS_GCC_NOWARNING(pedantic)
TS_LLVM_NOWARNING(reserved-id-macro)
TS_LLVM_NOWARNING(zero-length-array)

#if !defined(DOXYGEN)
    #define TS_INSIDE_SYSTEM_HEADERS 1
#endif
