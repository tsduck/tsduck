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

#if defined(TS_WINDOWS)
#include "tsUString.h"
#endif


//----------------------------------------------------------------------------
// A function to wrap the entry point of an application.
//----------------------------------------------------------------------------

int MainWrapper(int (*func)(int argc, char* argv[]), int argc, _TS_ARGV_TYPE* argv[])
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

        // Convert UTF-16 command line arguments to UTF-8 and rebuild a 8-bit version of argv.
        std::vector<std::string> arg8(argc);
        std::vector<char*> argv8(argc);
        for (int i = 0; i < argc; i++) {
            ts::UString(argv[i]).toUTF8(arg8[i]);
            argv8[i] = arg8[i].data();
        }

        // Argv is traditionally followed by a null pointer.
        argv8.push_back(nullptr);

        // Actual application code.
        return func(argc, argv8.data());
#else
        // Actual application code.
        return func(argc, argv);
#endif
    }
    catch (const std::exception& e) {
        std::cerr << "Program aborted: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
