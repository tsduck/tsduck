//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!  To be included by unitary tests for the C++ language using CppUnit.
//!
//!  This header file provides all required headers from CppUnit to write
//!  a unitary test suite.
//!
//----------------------------------------------------------------------------

#pragma once
#include <string>
#include <ostream>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
#endif

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif


//!
//! @hideinitializer
//! Assert that two objects which can be converted to a string are identical.
//!
#define CPPUNIT_ASSERT_STRINGS_EQUAL(expected,actual) CPPUNIT_ASSERT_EQUAL('"'+std::string(expected)+'"','"'+std::string(actual)+'"')

//!
//! @hideinitializer
//! Assert that two objects which can be converted to a unicode string are identical.
//!
#define CPPUNIT_ASSERT_USTRINGS_EQUAL(expected,actual) CPPUNIT_ASSERT_EQUAL(u'"'+ts::UString(expected)+u'"',u'"'+ts::UString(actual)+u'"')

//!
//! Unitary tests namespace
//!
namespace utest {

    //!
    //! This static method checks if debug mode is active (ie if debug messages are displayed).
    //!
    //! @return True if debug mode is active, false otherwise.
    //!
    bool DebugMode();

    //!
    //! This static method returns a reference to an output stream
    //! which can be used by unitary tests to log messages.
    //!
    //! A unitary test typically does not display anything. It simply
    //! performs assertions. A complete set of unitary test suites
    //! reports successes or failures using CppUnit.
    //!
    //! However, there are cases where the unitary test may want to
    //! issue trace, log or debug messages. Such messages should be
    //! sent to this output stream.
    //!
    //! By default, these messages are discarded. However, when the
    //! option -d (debug) is specified on the command line of the
    //! unitary test driver, these messages are reported on the
    //! standard error stream.
    //!
    //! @return A reference to an output stream used to report debug
    //! messages.
    //!
    std::ostream& Out();

#if defined(UTEST_CPPUNITMAIN_CPP) || defined(UTEST_CPPUNITTEST_CPP)
    //!
    //! This static method returns a reference to the actual output file
    //! stream used to report debug messages. If this file is not open,
    //! debug messages will go to the standard error.
    //!
    //! @return A reference to the output file stream.
    //!
    std::ofstream& DebugStream();
#endif
}
