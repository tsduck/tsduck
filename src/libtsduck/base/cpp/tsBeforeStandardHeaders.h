//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
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
