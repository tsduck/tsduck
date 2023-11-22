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
