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
//  TSUnit test suite for tsDirectShowUtils.h (Windows only)
//
//----------------------------------------------------------------------------

#include "tsPlatform.h"
#if defined(TS_WINDOWS)

#include "tsDirectShowTest.h"
#include "tsWinUtils.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class DirectShowTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testDevices();

    TSUNIT_TEST_BEGIN(DirectShowTest);
    TSUNIT_TEST(testDevices);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(DirectShowTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void DirectShowTest::beforeTest()
{
    TSUNIT_ASSERT(ts::ComSuccess(::CoInitializeEx(NULL, ::COINIT_MULTITHREADED), u"CoInitializeEx", CERR));
}

// Test suite cleanup method.
void DirectShowTest::afterTest()
{
    ::CoUninitialize();
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void DirectShowTest::testDevices()
{
    ts::DirectShowTest test(debug(), CERR);
    test.runTest(ts::DirectShowTest::ENUMERATE_DEVICES);
}

#endif // TS_WINDOWS
