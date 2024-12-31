//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for tsFatal.h
//
//  Since the purpose of this test is to crash the application, we don't do
//  it blindly! The crash is effective only if the environment variable
//  UTEST_FATAL_CRASH_ALLOWED is defined.
//
//----------------------------------------------------------------------------

#include "tsFatal.h"
#include "tsEnvironment.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class FatalTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(WithoutCrash);
    TSUNIT_DECLARE_TEST(Crash);
};

TSUNIT_REGISTER(FatalTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(WithoutCrash)
{
    int i = 0;

    // Shall not crash with a non_null address.
    ts::CheckNonNull(&i);
}

TSUNIT_DEFINE_TEST(Crash)
{
    if (ts::EnvironmentExists(u"UTEST_FATAL_CRASH_ALLOWED")) {
        std::cerr << "FatalTest: CheckNonNull(0) : should fail !" << std::endl
                  << "Unset UTEST_FATAL_CRASH_ALLOWED to skip the crash test" << std::endl;
        ts::CheckNonNull(nullptr);
        TSUNIT_FAIL("Should not get there, should have crashed");
    }
    else {
        debug() << "FatalTest: crash test skipped, define UTEST_FATAL_CRASH_ALLOWED to force it" << std::endl;
    }
}
