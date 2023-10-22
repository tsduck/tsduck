//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for singletons.
//
//----------------------------------------------------------------------------

#include "tsSingletonManager.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SingletonTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testSingleton();

    TSUNIT_TEST_BEGIN(SingletonTest);
    TSUNIT_TEST(testSingleton);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(SingletonTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void SingletonTest::beforeTest()
{
}

// Test suite cleanup method.
void SingletonTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Singleton class
//----------------------------------------------------------------------------

namespace {
    class Singleton
    {
        TS_DECLARE_SINGLETON(Singleton);
    };

    TS_DEFINE_SINGLETON(Singleton);

    Singleton::Singleton()
    {
        tsunit::Test::debug() << "SingletonTest: constructor of the singleton" << std::endl;
    }
}

//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void SingletonTest::testSingleton()
{
    // Check that this is a singleton
    Singleton* p1(Singleton::Instance());
    Singleton* p2(Singleton::Instance());
    TSUNIT_ASSERT(p1 == p2);
}
