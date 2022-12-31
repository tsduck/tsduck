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
//  TSUnit test suite for TS_STATIC_INSTANCE family of macros.
//
//----------------------------------------------------------------------------

#include "tsStaticInstance.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class StaticInstanceTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testNoInitializer();
    void testInitializerTwoArgs();
    void testWithNamespace();

    TSUNIT_TEST_BEGIN(StaticInstanceTest);
    TSUNIT_TEST(testNoInitializer);
    TSUNIT_TEST(testInitializerTwoArgs);
    TSUNIT_TEST(testWithNamespace);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(StaticInstanceTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void StaticInstanceTest::beforeTest()
{
}

// Test suite cleanup method.
void StaticInstanceTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Static instance, no initializer
TS_STATIC_INSTANCE(std::string, (), Foo1)

void StaticInstanceTest::testNoInitializer()
{
    debug() << "StaticInstanceTest: Foo1::Instance() = \"" << Foo1::Instance() << "\"" << std::endl;

    // Check the value
    TSUNIT_ASSERT(Foo1::Instance().empty());

    // Check that this is a singleton
    std::string* p1(&Foo1::Instance());
    std::string* p2(&Foo1::Instance());
    TSUNIT_ASSERT(p1 == p2);
}

// Static instance, initializer with two parameters
TS_STATIC_INSTANCE(std::string, (4, '='), Foo2)

void StaticInstanceTest::testInitializerTwoArgs()
{
    debug() << "StaticInstanceTest: Foo2::Instance() = \"" << Foo2::Instance() << "\"" << std::endl;

    // Check the value
    TSUNIT_EQUAL("====", Foo2::Instance());

    // Check that this is a singleton
    std::string* p1(&Foo2::Instance());
    std::string* p2(&Foo2::Instance());
    TSUNIT_ASSERT(p1 == p2);
}

// Static instance with separate declaration and definition, initializer with one parameter
namespace ts {
    namespace foo {
        TS_STATIC_INSTANCE_DECLARATION(std::string, , Foo3);
    }
}
TS_STATIC_INSTANCE_DEFINITION(std::string, ("this is Foo3"), ts::foo::Foo3, Foo3);

void StaticInstanceTest::testWithNamespace()
{
    debug() << "StaticInstanceTest: ts::foo::Foo3::Instance() = \"" << ts::foo::Foo3::Instance() << "\"" << std::endl;

    // Check the value
    TSUNIT_EQUAL("this is Foo3", ts::foo::Foo3::Instance());

    // Check that this is a singleton
    std::string* p1(&ts::foo::Foo3::Instance());
    std::string* p2(&ts::foo::Foo3::Instance());
    TSUNIT_ASSERT(p1 == p2);
}
