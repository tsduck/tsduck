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
//  CppUnit test suite for class ts::SystemRandomGenerator
//
//----------------------------------------------------------------------------

#include "tsSystemRandomGenerator.h"
#include "tsByteBlock.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SystemRandomGeneratorTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testSystemRandomGenerator();

    CPPUNIT_TEST_SUITE (SystemRandomGeneratorTest);
    CPPUNIT_TEST (testSystemRandomGenerator);
    CPPUNIT_TEST_SUITE_END ();
};

CPPUNIT_TEST_SUITE_REGISTRATION (SystemRandomGeneratorTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void SystemRandomGeneratorTest::setUp()
{
}

// Test suite cleanup method.
void SystemRandomGeneratorTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void SystemRandomGeneratorTest::testSystemRandomGenerator()
{
    ts::SystemRandomGenerator gen;

    // System PRNG are supposed to be immediately ready
    CPPUNIT_ASSERT(gen.ready());

    // But make sure they accept to be seeded anyway
    ts::ByteBlock seed(256);
    CPPUNIT_ASSERT(gen.seed(&seed[0], seed.size()));

    // Now, it is difficult to "test" random generator.
    // We use the following scenario:
    // - Pre-fill two 1000-byte vectors with zeroes.
    // - Generate random data over them.
    // - Check that they are not identical.
    // - Check that no more than 10% of the bytes are zero
    // The later condition is a bit arbitrary. Statistically, the proportion
    // of zeroes is 3.9% (1/256) but this is only statistics.

    ts::ByteBlock data1(1000, 0);
    ts::ByteBlock data2(1000, 0);

    CPPUNIT_ASSERT(data1 == data2);
    CPPUNIT_ASSERT_EQUAL(data1.size(), size_t(std::count(data1.begin(), data1.end(), 0)));
    CPPUNIT_ASSERT_EQUAL(data2.size(), size_t(std::count(data2.begin(), data2.end(), 0)));

    CPPUNIT_ASSERT(gen.read(&data1[0], data1.size()));
    CPPUNIT_ASSERT(gen.read(&data2[0], data2.size()));

    const size_t zero1 = std::count(data1.begin(), data1.end(), 0);
    const size_t zero2 = std::count(data2.begin(), data2.end(), 0);

    utest::Out() << "SystemRandomGeneratorTest: zeroes over " << data1.size() << " bytes: "
                 << zero1 << ", " << zero2 << std::endl;

    CPPUNIT_ASSERT(zero1 < data1.size() / 10);
    CPPUNIT_ASSERT(zero2 < data2.size() / 10);
    CPPUNIT_ASSERT(data1 != data2);
}
