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

#include "tsMain.h"
#include "tsConsoleState.h"
#include "tsIPUtils.h"
#include "tsCOM.h"


//----------------------------------------------------------------------------
// A function to wrap the entry point of an application.
//----------------------------------------------------------------------------

int MainWrapper(int (*func)(int argc, char* argv[]), int argc, char* argv[])
{
    // Save console state, set UTF-8 output, restore state on exit.
    ts::ConsoleState _consoleState;

    try {

#if defined(TS_WINDOWS)
        // Initialize COM and networking.
        ts::COM com;
        if (!com.isInitialized() || !ts::IPInitialize()) {
            return EXIT_FAILURE;
        }
#endif

        // Actual application code.
        return func(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << "Program aborted: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
