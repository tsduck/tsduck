//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2019, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A class implementing the unitary tests main program using CppUnit.
//!
//----------------------------------------------------------------------------

#pragma once
#include <string>

//!
//! Unitary tests namespace
//!
namespace utest {

    //!
    //! This class drives all unitary tests in a project.
    //!
    //! There must be one instance in the main program of the unitary
    //! test driver of the project.
    //!
    //! The layout of the unitary test driver main program is as simple as:
    //! @code
    //! #include "utestCppUnitMain.h"
    //! int main(int argc, char* argv[])
    //! {
    //!     utest::CppUnitMain ctx(argc, argv, "MyProjectName");
    //!     return ctx.run();
    //! }
    //! @endcode
    //!
    //! The command line arguments @c argc and @c argv are analyzed to setup
    //! the unitary tests. The accepted command line arguments are:
    //!
    //! @li -a : Automated test mode. Produces an XML report. The default XML
    //!          output file name is <i>MyProjectName</i>-Results.xml.
    //! @li -d : Debug messages from the various unitary tests are output on
    //!          standard error. By default, they are dropped.
    //! @li -l : List all tests but do not execute them.
    //! @li -n : Normal basic mode (default).
    //! @li -o name : Specify an alternate output file prefix with -a instead
    //!          of <i>MyProjectName</i>. The suffix -Results.xml will be added.
    //! @li -s : Silent mode, same as -n, for compatibility with CUnit.
    //! @li -t name : Run only one test or test suite (use -l for test list).
    //! @li -v : Verbose mode, same as -n, for compatibility with CUnit.
    //!
    class CppUnitMain
    {
    public:

        //!
        //! Constructor from command line arguments.
        //!
        //! The command line arguments are analyzed and the object
        //! is setup accordingly.
        //!
        //! @param [in] argc Number of arguments from command line.
        //! @param [in] argv Arguments from command line.
        //! @param [in] outName Prefix of the output XML file name
        //! in automated (-a) mode. The actual file name will be
        //! <code><i>outName</i>-Results.xml</code>.
        //!
        CppUnitMain(int argc, char* argv[], const char* outName);

        //!
        //! Run the unitary tests.
        //!
        //! @return EXIT_SUCCESS if all tests passed, EXIT_FAILURE otherwise.
        //! Thus, the result can be used as exit status in the unitary test driver.
        //!
        int run();

    private:
        // Print command line usage and return EXIT_FAILURE.
        int usage();

        // Private fields
        const std::string _argv0;   // program name
        const std::string _outName; // default output file prefix
        std::string _outPrefix;     // output file prefix
        std::string _outSuffix;     // output file suffix
        std::string _testName;      // name of test to run
        char _runMode;              // output type (option character)
        bool _listMode;             // list tests, do not execute
        bool _debug;                // enable debug messages
        int _exitStatus;            // EXIT_SUCCESS or EXIT_FAILURE

        // Forbidden operations
        CppUnitMain() = delete;
        CppUnitMain(const CppUnitMain&) = delete;
        CppUnitMain& operator=(const CppUnitMain&) = delete;
    };
}
