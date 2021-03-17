//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup app
//!  Define a standard main() function with appropriate checks.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"

//!
//! A function to wrap the entry point of an application.
//! The application code should use the macro TS_MAIN instead of
//! directly calling this function.
//!
//! Uncaught exceptions are displayed.
//! On Windows, the COM environment and IP networking are initialized.
//! The Windows console is set to UTF-8 mode and restored to its previous value on exit.
//!
//! @param [in] func The actual main function with the same profile as main().
//! @param [in] argc Command line parameter count.
//! @param [in] argv Command line parameters.
//! @return The process exit code, typically EXIT_SUCCESS or EXIT_FAILURE.
//!
int TSDUCKDLL MainWrapper(int (*func)(int argc, char* argv[]), int argc, char* argv[]);

//! @cond nodoxygen
// On Windows, verify that the DLL has the same version number as the application.
#if defined (TS_WINDOWS)
#include "tsVersionString.h"
#include "tsVersionInfo.h"
#define TS_CHECK_TSDUCKLIB_VERSION                                       \
    if (tsduckLibraryVersionMajor != TS_VERSION_MAJOR ||                 \
        tsduckLibraryVersionMinor != TS_VERSION_MINOR ||                 \
        tsduckLibraryVersionCommit != TS_COMMIT)                         \
    {                                                                    \
        std::cerr << "**** TSDuck library version mismatch, library is " \
                  << tsduckLibraryVersionMajor << "."                    \
                  << tsduckLibraryVersionMinor << "-"                    \
                  << tsduckLibraryVersionCommit                          \
                  << ", this command needs " TS_VERSION_STRING " ****"   \
                  << std::endl << std::flush;                            \
        return EXIT_FAILURE;                                             \
    }
#else
#define TS_CHECK_TSDUCKLIB_VERSION
#endif
//! @endcond

//!
//! A macro which expands to a main() program.
//!
//! On Windows, the version of the tsduck DLL is checked before the first call to it.
//! It has been noted that using a tsduck DLL with an incompatible version sometimes makes
//! the application silently exit on Windows. This is why we check the version of the DLL.
//!
//! @param func The actual main function with the same profile as main().
//! @hideinitializer
//!
#define TS_MAIN(func)                         \
    int func(int argc, char *argv[]);         \
    int main(int argc, char *argv[])          \
    {                                         \
        TS_CHECK_TSDUCKLIB_VERSION            \
        return MainWrapper(func, argc, argv); \
    }                                         \
    typedef int TS_UNIQUE_NAME(UnusedMainType) /* allow trailing semi-colon */
