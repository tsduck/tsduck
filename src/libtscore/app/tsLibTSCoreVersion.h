//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Version of the libtscore library.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsVersion.h"

//!
//! Define the TSDuck version as an 8-bit string literal.
//! @ingroup app
//!
#define TS_VERSION_STRING TS_STRINGIFY(TS_VERSION_MAJOR) "." TS_STRINGIFY(TS_VERSION_MINOR) "-" TS_STRINGIFY(TS_COMMIT)

//!
//! Define the TSDuck version as a 16-bit string literal.
//! @ingroup app
//!
#define TS_VERSION_USTRING TS_USTRINGIFY(TS_VERSION_MAJOR) u"." TS_USTRINGIFY(TS_VERSION_MINOR) u"-" TS_USTRINGIFY(TS_COMMIT)

//!
//! Define the TSDuck version as an integer, suitable for comparisons.
//! @ingroup app
//!
#define TS_VERSION_INTEGER ((TS_VERSION_MAJOR * 10000000) + (TS_VERSION_MINOR * 100000) + TS_COMMIT)

extern "C" {
    //!
    //! Major version of the TSCore library as the int value of a symbol from the library.
    //!
    extern const int TSCOREDLL tscoreLibraryVersionMajor;
    //!
    //! Minor version of the TSCore library as the int value of a symbol from the library.
    //!
    extern const int TSCOREDLL tscoreLibraryVersionMinor;
    //!
    //! Commit version of the TSCore library as the int value of a symbol from the library.
    //!
    extern const int TSCOREDLL tscoreLibraryVersionCommit;

    // Compute the name of a symbol which describe the TSCore library version.
    //! @cond nodoxygen
    #define TS_SYM4(a,b,c,d) TS_SYM4a(a,b,c,d)
    #define TS_SYM4a(a,b,c,d) a##_##b##_##c##_##d
    #define LIBTSCORE_VERSION_SYMBOL TS_SYM4(LIBTSCORE_VERSION,TS_VERSION_MAJOR,TS_VERSION_MINOR,TS_COMMIT)
    //! @endcond

    //!
    //! Full version of the TSCore library in the name of a symbol from the library.
    //!
    //! Can be used to force an undefined reference at run-time in case of version mismatch.
    //! The C++ application uses the name LIBTSCORE_VERSION_SYMBOL but this
    //! generates a reference to a symbol containing the actual version number.
    //!
    //! Example:
    //! @code
    //!   $ nm -C tsLibTSCoreVersion.o | grep LIBTSCORE_VERSION
    //!   000000000000000c S LIBTSCORE_VERSION_3_40_4153
    //! @endcode
    //!
    extern const int TSCOREDLL LIBTSCORE_VERSION_SYMBOL;
}

//!
//! A macro to use in application code to enforce the TSCore library version.
//!
//! When this macro is used in an executable or shared library which uses the
//! TSCore library, it generates an external reference to a symbol name which
//! contains the TSCore library version number, at the time of the compilation
//! of the application code. If the application is run later on a system with
//! a TSCore library with a different version, the reference won't be resolved
//! and the application won't run.
//!
//! If we don't do this, the initialization of application automatically calls
//! complex "Register" constructors in the TSCore library which may fail if the
//! version of the library is different.
//!
//! @hideinitializer
//!
#define TS_LIBTSCORE_CHECK() TS_STATIC_REFERENCE(version, &LIBTSCORE_VERSION_SYMBOL)
