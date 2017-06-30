//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  CppUnit test suite for tsDirectShowUtils.h (Windows only)
//
//----------------------------------------------------------------------------

#include "tsPlatform.h"
TSDUCK_SOURCE;
#if defined (__windows)

#include "tsTunerUtils.h"
#include "tsDirectShowUtils.h"
#include "tsComUtils.h"
#include "utestCppUnitTest.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class DirectShowTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testDevices();

    CPPUNIT_TEST_SUITE(DirectShowTest);
    CPPUNIT_TEST(testDevices);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DirectShowTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void DirectShowTest::setUp()
{
}

// Test suite cleanup method.
void DirectShowTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void DirectShowTest::testDevices()
{
    CPPUNIT_ASSERT(ts::ComSuccess(::CoInitializeEx(NULL, ::COINIT_MULTITHREADED), "CoInitializeEx", CERR));

    // List devices by category
#define _C_(cat) ts::DisplayDevicesByCategory(utest::Out(), cat, "", #cat, CERR)

    _C_(KSCATEGORY_BDA_NETWORK_PROVIDER);
    _C_(KSCATEGORY_BDA_TRANSPORT_INFORMATION);
    _C_(KSCATEGORY_CAPTURE);
    _C_(KSCATEGORY_SPLITTER);
    _C_(KSCATEGORY_TVTUNER);
    _C_(KSCATEGORY_BDA_RECEIVER_COMPONENT);
    _C_(KSCATEGORY_BDA_NETWORK_TUNER);

#undef _C_

    ::CoUninitialize();
}

#endif // __windows
