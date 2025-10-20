//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Define a standard main() function with appropriate checks.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsLibTSCoreVersion.h"

//! @cond nodoxygen
// See comments below.
#if defined(TS_WINDOWS)
    #define _TS_MAIN_NAME wmain
    #define _TS_ARGV_TYPE wchar_t
#else
    #define _TS_MAIN_NAME main
    #define _TS_ARGV_TYPE char
#endif
//! @endcond

//!
//! A function to wrap the entry point of an application.
//! @ingroup app
//!
//! The application code should use the macro TS_MAIN instead of directly calling this function.
//!
//! Uncaught exceptions are displayed.
//!
//! Windows specificities
//! ---------------------
//! The COM environment and IP networking are initialized.
//!
//! The Windows console is set to UTF-8 mode and restored to its previous value on exit.
//!
//! The entry point is defined as wmain() instead of main() in macro TS_MAIN to capture
//! UTF-16 characters from the command line. This is a workaround for another Windows oddity.
//! On Windows, unlike any modern operating system, when main() is used, any non-ASCII
//! character from the command line is converted to '?' instead of converting all strings
//! to UTF-8. We need to use wmain() to get UTF-16 characters. In MainWrapper(), we convert
//! UTF-16 characters from the command line into UTF-8 to be compatible with the classical
//! main() UTF-8 profile. In that case, there is a double conversion, first from UTF-16 to
//! UTF-8 in MainWrapper(), then back from UTF-8 to UTF-16 inside the application, typically
//! in class Args. This is silly but an operating not working in UTF-8 is silly...
//!
//! @param [in] func The actual main function with the same profile as main().
//! @param [in] argc Command line parameter count.
//! @param [in] argv Command line parameters.
//! @return The process exit code, typically EXIT_SUCCESS or EXIT_FAILURE.
//!
int TSCOREDLL MainWrapper(int (*func)(int argc, char* argv[]), int argc, _TS_ARGV_TYPE* argv[]);

//!
//! A macro which expands to a main() program.
//! An explicit reference is made to the TSDuck library version to check
//! that the compilation and runtime versions are identical.
//! @ingroup app
//! @param func The actual main function with the same profile as main().
//! @hideinitializer
//!
#define TS_MAIN(func)                                  \
    static int func(int argc, char* argv[]);           \
    int _TS_MAIN_NAME(int argc, _TS_ARGV_TYPE* argv[]) \
    {                                                  \
        TS_LIBTSCORE_CHECK();                          \
        return MainWrapper(func, argc, argv);          \
    }                                                  \
    /** @cond nodoxygen */                             \
    using TS_UNIQUE_NAME(for_trailing_semicolon) = int \
    /** @endcond */
