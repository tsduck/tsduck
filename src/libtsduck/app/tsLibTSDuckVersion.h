//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Version of the libtsduck library.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsLibTSCoreVersion.h"

extern "C" {
    //!
    //! Major version of the TSDuck library as the int value of a symbol from the library.
    //!
    extern const int TSDUCKDLL tsduckLibraryVersionMajor;
    //!
    //! Minor version of the TSDuck library as the int value of a symbol from the library.
    //!
    extern const int TSDUCKDLL tsduckLibraryVersionMinor;
    //!
    //! Commit version of the TSDuck library as the int value of a symbol from the library.
    //!
    extern const int TSDUCKDLL tsduckLibraryVersionCommit;

    // Compute the name of a symbol which describe the TSDuck library version.
    //! @cond nodoxygen
    #define LIBTSDUCK_VERSION_SYMBOL TS_SYM4(LIBTSDUCK_VERSION,TS_VERSION_MAJOR,TS_VERSION_MINOR,TS_COMMIT)
    //! @endcond

    //!
    //! Full version of the TSDuck library in the name of a symbol from the library.
    //! Same principle as LIBTSCORE_VERSION_SYMBOL
    //!
    extern const int TSDUCKDLL LIBTSDUCK_VERSION_SYMBOL;
}

//!
//! A macro to use in application code to enforce the TSDuck library version.
//!
//! When this macro is used in an executable or shared library which uses the
//! TSDuck library, it generates an external reference to a symbol name which
//! contains the TSDuck library version number, at the time of the compilation
//! of the application code. If the application is run later on a system with
//! a TSDuck library with a different version, the reference won't be resolved
//! and the application won't run.
//!
//! If we don't do this, the initialization of application automatically calls
//! complex "Register" constructors in the TSDuck library which may fail if the
//! version of the library is different.
//!
//! @hideinitializer
//!
#define TS_LIBTSDUCK_CHECK() TS_LIBTSCORE_CHECK(); TS_STATIC_REFERENCE(duck_version, &LIBTSDUCK_VERSION_SYMBOL)
