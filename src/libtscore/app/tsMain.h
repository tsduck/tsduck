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
#include "tsVersionInfo.h"

//!
//! A function to wrap the entry point of an application.
//! The application code should use the macro TS_MAIN instead of
//! directly calling this function.
//!
//! Uncaught exceptions are displayed.
//! On Windows, the COM environment and IP networking are initialized.
//! The Windows console is set to UTF-8 mode and restored to its previous value on exit.
//!
//! @ingroup app
//! @param [in] func The actual main function with the same profile as main().
//! @param [in] argc Command line parameter count.
//! @param [in] argv Command line parameters.
//! @return The process exit code, typically EXIT_SUCCESS or EXIT_FAILURE.
//!
int TSCOREDLL MainWrapper(int (*func)(int argc, char* argv[]), int argc, char* argv[]);

//!
//! A macro which expands to a main() program.
//! An explicit reference is made to the TSDuck library version to check
//! that the compilation and runtime versions are identical.
//! @ingroup app
//! @param func The actual main function with the same profile as main().
//! @hideinitializer
//!
#define TS_MAIN(func)                                  \
    static int func(int argc, char *argv[]);           \
    int main(int argc, char *argv[])                   \
    {                                                  \
        TS_LIBCHECK();                                 \
        return MainWrapper(func, argc, argv);          \
    }                                                  \
    /** @cond nodoxygen */                             \
    using TS_UNIQUE_NAME(for_trailing_semicolon) = int \
    /** @endcond */
