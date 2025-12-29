//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(Devices);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;
};

TSUNIT_REGISTER(DirectShowTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void DirectShowTest::beforeTest()
{
    TSUNIT_ASSERT(ts::ComSuccess(::CoInitializeEx(nullptr, ::COINIT_MULTITHREADED), u"CoInitializeEx", CERR));
}

// Test suite cleanup method.
void DirectShowTest::afterTest()
{
    ::CoUninitialize();
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Devices)
{
    ts::DirectShowTest test(debug(), CERR);
    test.runTest(ts::DirectShowTest::ENUMERATE_DEVICES);
}

#endif // TS_WINDOWS
