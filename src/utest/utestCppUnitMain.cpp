//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#define UTEST_CPPUNITMAIN_CPP 1
#include "utestCppUnitMain.h"
#include "utestCppUnitTest.h"
#include <cstring>
#include <cstdlib>
#include <fstream>

// Make sure that NULL in CppUnit headers does not trigger fatal warnings.
#if defined(NULL)
#undef NULL
#endif
#define NULL nullptr

#include <cppunit/TextOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>

//
// A file name which discards all output
//
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    #define UTEST_DEV_NULL "NUL:"
#else
    #define UTEST_DEV_NULL "/dev/null"
#endif


//----------------------------------------------------------------------------
// Constructor from command line
//----------------------------------------------------------------------------

utest::CppUnitMain::CppUnitMain(int argc, char* argv[], const char* outName) :
    _argv0(argv[0]),
    _outName(outName),
    _outPrefix(outName),
    _outSuffix("-Results.xml"),
    _testName(""),
    _runMode('n'), // Default option is normal basic mode
    _listMode(false),
    _debug(false),
    _exitStatus(EXIT_SUCCESS)
{
    bool ok = true;

    // Decode the command line.
    for (int arg = 1; ok && arg < argc; arg++) {
        const char* opt = argv[arg];
        if (::strlen (opt) != 2 || opt[0] != '-') {
            ok = false;
        }
        else {
            switch (opt[1]) {
            case 'a':
            case 'n':
            case 's':
            case 'v':
                _runMode = opt[1];
                break;
            case 'd':
                _debug = true;
                break;
            case 'l':
                _listMode = true;
                break;
            case 'o':
                if (++arg >= argc) {
                    ok = false;
                }
                else {
                    _outPrefix = argv[arg];
                }
                break;
            case 't':
                if (++arg >= argc) {
                    ok = false;
                }
                else {
                    _testName = argv[arg];
                }
                break;
            default:
                ok = false;
                break;
            }
        }
    }

    // Error message if incorrect line
    if (!ok) {
        usage();
    }
}


//----------------------------------------------------------------------------
// Print command line usage and return EXIT_FAILURE.
//----------------------------------------------------------------------------

int utest::CppUnitMain::usage()
{
    std::cerr << _argv0 << ": invalid command" << std::endl
              << std::endl
              << "Syntax: " << _argv0 << " [options]" << std::endl
              << std::endl
              << "The available options are:" << std::endl
              << "  -a : Automated test mode, default XML file: " << _outName << _outSuffix << std::endl
              << "  -d : Debug messages are output on standard error" << std::endl
              << "  -l : List all tests but do not execute them" << std::endl
              << "  -n : Normal basic mode (default)" << std::endl
              << "  -o name : Output file prefix with -a, " << _outSuffix << " added" << std::endl
              << "  -s : Silent mode, same as -n, for compatibility with CUnit" << std::endl
              << "  -t name : Run only one test or test suite (use -l for test list)" << std::endl
              << "  -v : Verbose mode, same as -n, for compatibility with CUnit" << std::endl;

    _exitStatus = EXIT_FAILURE;
    return EXIT_FAILURE;
}

//----------------------------------------------------------------------------
// Recursively list all tests
//----------------------------------------------------------------------------

namespace {
    void ListTests(CppUnit::Test* test, size_t indent, bool print)
    {
        if (test != nullptr) {
            if (print) {
                std::cout << std::string(indent, ' ') << test->getName() << std::endl;
                indent += 4;
            }
            int count = test->getChildTestCount();
            for (int i = 0; i < count; ++i) {
                ListTests(test->getChildTestAt(i), indent, true);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Run the tests
//----------------------------------------------------------------------------

int utest::CppUnitMain::run()
{
    // Filter previous errors
    if (_exitStatus != EXIT_SUCCESS) {
        return _exitStatus;
    }

    // Load the tests from the registry.
    // The tests were registered using CPPUNIT_TEST_SUITE_REGISTRATION macros.
    CppUnit::Test* test(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    // In list mode, only print the list of tests.
    if (_listMode) {
        ListTests(test, 0, false);
        return EXIT_SUCCESS;
    }

    // In non debug mode, redirect debug messages to nul device
    if (!_debug) {
        DebugStream().open(UTEST_DEV_NULL);
        if (!DebugStream()) {
            std::cerr << _argv0 << ": error opening " << UTEST_DEV_NULL << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Create a test executor
    CppUnit::TextTestRunner runner;

    // Output stream file
    std::ofstream out;

    // Set output
    switch (_runMode) {
        case 'n':
        case 's':
        case 'v': {
            // Normal basic mode
            CppUnit::TextOutputter* text = new CppUnit::TextOutputter(&runner.result(), std::cout);
            runner.setOutputter(text);
            break;
        }
        case 'a': {
            // Automated XML mode
            const std::string fileName(_outPrefix + _outSuffix);
            out.open (fileName.c_str());
            if (!out) {
                std::cerr << _argv0 << ": error creating " << fileName << std::endl;
                return EXIT_FAILURE;
            }
            CppUnit::XmlOutputter* xml = new CppUnit::XmlOutputter(&runner.result(), out, "UTF-8");
            runner.setOutputter(xml);
            break;
        }
        default: {
            std::cerr << _argv0 << ": CppUnitMain internal error, unexpected run mode" << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Run the tests
    runner.addTest(test);
    bool success = false;
    try {
        success = runner.run(_testName, false, true, false);
    }
    catch (const std::exception& e) {
        std::cerr << _argv0 << ": exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << _argv0 << ": unknown exception" << std::endl;
    }

    // Cleanup resources
    if (out.is_open()) {
        out.close();
    }
    if (DebugStream().is_open()) {
        DebugStream().close();
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
