//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

//!
//! A macro which expands to a main() program.
//! An explicit reference is made to the TSDuck library version to check
//! that the compilation and runtime versions are identical.
//! @param func The actual main function with the same profile as main().
//! @hideinitializer
//!
#define TS_MAIN(func)                         \
    int func(int argc, char *argv[]);         \
    int main(int argc, char *argv[])          \
    {                                         \
        TS_LIBCHECK();                        \
        return MainWrapper(func, argc, argv); \
    }                                         \
    typedef int TS_UNIQUE_NAME(UnusedMainType) /* allow trailing semi-colon */
