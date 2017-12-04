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
//  CppUnit test suite for ts::TablesFactory.
//
//----------------------------------------------------------------------------

#include "tsTablesFactory.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TablesFactoryTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testRegistrations();

    CPPUNIT_TEST_SUITE(TablesFactoryTest);
    CPPUNIT_TEST(testRegistrations);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TablesFactoryTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TablesFactoryTest::setUp()
{
}

// Test suite cleanup method.
void TablesFactoryTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TablesFactoryTest::testRegistrations()
{
    ts::UStringList names;

    ts::TablesFactory::Instance()->getRegisteredTableNames(names);
    utest::Out() << "TablesFactoryTest::testRegistrations: table names: " << ts::UString::Join(names) << std::endl;

    CPPUNIT_ASSERT(!names.empty());
    CPPUNIT_ASSERT(ts::UString(u"PAT").containSimilar(names));
    CPPUNIT_ASSERT(ts::UString(u"PMT").containSimilar(names));

    ts::TablesFactory::Instance()->getRegisteredDescriptorNames(names);
    utest::Out() << "TablesFactoryTest::testRegistrations: descriptor names: " << ts::UString::Join(names) << std::endl;

    CPPUNIT_ASSERT(!names.empty());
    CPPUNIT_ASSERT(ts::UString(u"ca_descriptor").containSimilar(names));
}
