//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
