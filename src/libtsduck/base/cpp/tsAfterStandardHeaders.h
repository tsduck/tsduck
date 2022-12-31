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
//!  Technical header to include after including standard and system headers.
//!  @see tsBeforeStandardHeaders.h
//!
//----------------------------------------------------------------------------

// Do not use "#pragma once", can be used multiple times on purpose.

// Must close a system header inclusion sequence.
#if defined(TS_INSIDE_SYSTEM_HEADERS)
    #undef TS_INSIDE_SYSTEM_HEADERS
#else
    #error "tsBeforeStandardHeaders.h must be included before tsAfterStandardHeaders.h"
#endif

// Restore warnings
TS_POP_WARNING()

// Some standard Windows headers have the very-very bad idea to define common
// words as macros. Also, common function names, used by TSDuck, are defined
// as macros, breaking C++ visibility rules.
#if defined(min)
    #undef min
#endif
#if defined(max)
    #undef max
#endif
#if defined(MIN)
    #undef MIN
#endif
#if defined(MAX)
    #undef MAX
#endif
#if defined(IGNORE)
    #undef IGNORE
#endif
#if defined(CHECK)
    #undef CHECK
#endif
#if defined(COMPUTE)
    #undef COMPUTE
#endif
#if defined(INFO)
    #undef INFO
#endif
#if defined(ERROR)
    #undef ERROR
#endif
#if defined(SHORT)
    #undef SHORT
#endif
#if defined(LONG)
    #undef LONG
#endif
#if defined(INTEGER)
    #undef INTEGER
#endif
#if defined(Yield)
    #undef Yield
#endif
#if defined(CreateDirectory)
    #undef CreateDirectory
#endif
#if defined(DeleteFile)
    #undef DeleteFile
#endif
#if defined(ALTERNATE)
    #undef ALTERNATE
#endif

// Similar common-words macros in Mach kernel (macOS).
#if defined(MAX_TRAILER_SIZE)
    #undef MAX_TRAILER_SIZE
#endif

// For platforms not supporting large files:
#if !defined(TS_WINDOWS) && !defined(O_LARGEFILE) && !defined(DOXYGEN)
    #define O_LARGEFILE 0
#endif
