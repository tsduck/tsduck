//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
#include "tsVersionInfo.h"
#include "tsConsoleState.h"
#include "tsIPUtils.h"
#include "tsCOM.h"

#if defined(TS_WINDOWS) && !defined(TS_NO_BUILD_VERSION)
#include "tsVersion.h"
#endif

//!
//! A macro which expands to a main() program.
//! Uncaught exceptions are displayed.
//!
//! On Windows, the IP networking is initialized and the version of the
//! tsduck DLL is checked. It has been noted that using a tsduck DLL with
//! an incompatible version makes the application silently exit on Windows.
//! This is why we check the version of the DLL.
//!
//! @param func The actual main function with the same profile as main().
//! @hideinitializer
//!
#if !defined(TS_WINDOWS)
#define TS_MAIN(func)                                                         \
    static ts::ConsoleState _consoleState;                                    \
    int func(int argc, char *argv[]);                                         \
    int main(int argc, char *argv[])                                          \
    {                                                                         \
        try {                                                                 \
            return func(argc, argv);                                          \
        }                                                                     \
        catch (const std::exception& e) {                                     \
            std::cerr << "Program aborted: " << e.what() << std::endl;        \
            return EXIT_FAILURE;                                              \
        }                                                                     \
    }                                                                         \
    typedef int UnusedMainType /* allow trailing semi-colon */
#elif defined(TS_NO_BUILD_VERSION)
#define TS_MAIN(func)                                                         \
    static ts::ConsoleState _consoleState;                                    \
    int func(int argc, char *argv[]);                                         \
    int main(int argc, char *argv[])                                          \
    {                                                                         \
        try {                                                                 \
            ts::COM com;                                                      \
            if (!com.isInitialized() || !ts::IPInitialize()) {                \
                return EXIT_FAILURE;                                          \
            }                                                                 \
            return func(argc, argv);                                          \
        }                                                                     \
        catch (const std::exception& e) {                                     \
            std::cerr << "Program aborted: " << e.what() << std::endl;        \
            return EXIT_FAILURE;                                              \
        }                                                                     \
    }                                                                         \
    typedef int UnusedMainType /* allow trailing semi-colon */
#else
#define TS_MAIN(func)                                                         \
    static ts::ConsoleState _consoleState;                                    \
    int func(int argc, char *argv[]);                                         \
    int main(int argc, char *argv[])                                          \
    {                                                                         \
        if (tsduckLibraryVersionMajor != TS_VERSION_MAJOR ||                  \
            tsduckLibraryVersionMinor != TS_VERSION_MINOR ||                  \
            tsduckLibraryVersionCommit != TS_COMMIT)                          \
        {                                                                     \
            std::cerr << "**** TSDuck library version mismatch, library is "  \
                      << tsduckLibraryVersionMajor << "."                     \
                      << tsduckLibraryVersionMinor << "-"                     \
                      << tsduckLibraryVersionCommit                           \
                      << ", this command needs " TS_VERSION_USTRING " ****"   \
                      << std::flush << std::endl;                             \
            return EXIT_FAILURE;                                              \
        }                                                                     \
        try {                                                                 \
            ts::COM com;                                                      \
            if (!com.isInitialized() || !ts::IPInitialize()) {                \
                return EXIT_FAILURE;                                          \
            }                                                                 \
            return func(argc, argv);                                          \
        }                                                                     \
        catch (const std::exception& e) {                                     \
            std::cerr << "Program aborted: " << e.what() << std::endl;        \
            return EXIT_FAILURE;                                              \
        }                                                                     \
    }                                                                         \
    typedef int UnusedMainType /* allow trailing semi-colon */
#endif
