//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2022, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
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

#pragma once

// Enforce inclusion tsPlatform.h but avoid recursion.
#if !defined(TS_ADDRESS_BITS)
    #include "tsPlatform.h"
#endif

// Windows specific settings
#if defined(TS_WINDOWS) && !defined(DOXYGEN)
    #if !defined(WINVER)
        #define WINVER 0x0601            // Allow use of features specific to Windows 7 or later.
    #endif
    #if !defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0601      // Allow use of features specific to Windows 7 or later.
    #endif
    #if defined(UNICODE)
        #undef UNICODE                   // No unicode in TSDuck, use single byte char
    #endif
    #if !defined(_CRT_SECURE_NO_DEPRECATE)
        #define _CRT_SECURE_NO_DEPRECATE 1
    #endif
    #if !defined(_CRT_NONSTDC_NO_DEPRECATE)
        #define _CRT_NONSTDC_NO_DEPRECATE 1
    #endif
    #if !defined(WIN32_LEAN_AND_MEAN)
        #define WIN32_LEAN_AND_MEAN 1        // Exclude rarely-used stuff from Windows headers
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

// [BUG.1] This one is nasty and is a bug in winioctl.h, already reported, never fixed, as usual with MSVC...
// It must be set before pushing warnings.
// tsPlatform.h(840, 1) : error C2220 : warning treated as error - no 'object' file generated
// tsPlatform.h(840, 1) : warning C5031 : #pragma warning(pop) : likely mismatch, popping warning state pushed in different file
// winioctl.h(161, 17) : message:  #pragma warning(push)
// tsPlatform.h(719, 1) : warning C5032 : detected #pragma warning(push) with no corresponding #pragma warning(pop)
// different warnings for older versions of MSVC:  C4193:  #pragma warning(pop): no matching '#pragma warning(push)'
TS_MSC_NOWARNING(5031)
TS_MSC_NOWARNING(5032)
TS_MSC_NOWARNING(4193)

// Warnings to disable for system headers.
TS_PUSH_WARNING()
TS_MSC_NOWARNING(4263)
TS_MSC_NOWARNING(4264)
TS_MSC_NOWARNING(4668)
TS_MSC_NOWARNING(4774)
TS_MSC_NOWARNING(5026)
TS_MSC_NOWARNING(5027)
TS_MSC_NOWARNING(5054)
TS_MSC_NOWARNING(5204)
TS_GCC_NOWARNING(pedantic)
TS_LLVM_NOWARNING(reserved-id-macro)
TS_LLVM_NOWARNING(zero-length-array)

#if !defined(DOXYGEN)
    #define TS_INSIDE_SYSTEM_HEADERS 1
#endif
