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
//
//  TSUnit test suite for tsFatal.h
//
//  Since the purpose of this test is to crash the application, we don't do
//  it blindly! The crash is effective only if the environment variable
//  UTEST_FATAL_CRASH_ALLOWED is defined.
//
//----------------------------------------------------------------------------

#include "tsFatal.h"
#include "tsSysUtils.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class FatalTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testWithoutCrash();
    void testCrash();

    TSUNIT_TEST_BEGIN(FatalTest);
    TSUNIT_TEST(testWithoutCrash);
    TSUNIT_TEST(testCrash);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(FatalTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void FatalTest::beforeTest()
{
}

// Test suite cleanup method.
void FatalTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void FatalTest::testWithoutCrash()
{
    int i = 0;

    // Shall not crash with a non_null address.
    ts::CheckNonNull(&i);
}

void FatalTest::testCrash()
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
