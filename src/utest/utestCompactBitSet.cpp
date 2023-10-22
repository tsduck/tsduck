//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::CompactBitSet
//
//----------------------------------------------------------------------------

#include "tsCompactBitSet.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class CompactBitSetTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testSize();
    void testOps14();

    TSUNIT_TEST_BEGIN(CompactBitSetTest);
    TSUNIT_TEST(testSize);
    TSUNIT_TEST(testOps14);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(CompactBitSetTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void CompactBitSetTest::beforeTest()
{
}

// Test suite cleanup method.
void CompactBitSetTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void CompactBitSetTest::testSize()
{
    TSUNIT_EQUAL(2, ts::CompactBitSet<2>::SIZE);
    TSUNIT_EQUAL(1, ts::CompactBitSet<2>::MAX);
    TSUNIT_EQUAL(0x03, ts::CompactBitSet<2>::ALL);
    TSUNIT_ASSUME(1 == sizeof(ts::CompactBitSet<2>));

    TSUNIT_EQUAL(8, ts::CompactBitSet<8>::SIZE);
    TSUNIT_EQUAL(7, ts::CompactBitSet<8>::MAX);
    TSUNIT_EQUAL(0xFF, ts::CompactBitSet<8>::ALL);
    TSUNIT_ASSUME(1 == sizeof(ts::CompactBitSet<8>));

    TSUNIT_EQUAL(14, ts::CompactBitSet<14>::SIZE);
    TSUNIT_EQUAL(13, ts::CompactBitSet<14>::MAX);
    TSUNIT_EQUAL(0x3FFF, ts::CompactBitSet<14>::ALL);
    TSUNIT_ASSUME(2 == sizeof(ts::CompactBitSet<14>));

    TSUNIT_EQUAL(16, ts::CompactBitSet<16>::SIZE);
    TSUNIT_EQUAL(15, ts::CompactBitSet<16>::MAX);
    TSUNIT_EQUAL(0xFFFF, ts::CompactBitSet<16>::ALL);
    TSUNIT_ASSUME(2 == sizeof(ts::CompactBitSet<16>));

    TSUNIT_EQUAL(30, ts::CompactBitSet<30>::SIZE);
    TSUNIT_EQUAL(29, ts::CompactBitSet<30>::MAX);
    TSUNIT_EQUAL(0x3FFFFFFF, ts::CompactBitSet<30>::ALL);
    TSUNIT_ASSUME(4 == sizeof(ts::CompactBitSet<30>));

    TSUNIT_EQUAL(32, ts::CompactBitSet<32>::SIZE);
    TSUNIT_EQUAL(31, ts::CompactBitSet<32>::MAX);
    TSUNIT_EQUAL(0xFFFFFFFF, ts::CompactBitSet<32>::ALL);
    TSUNIT_ASSUME(4 == sizeof(ts::CompactBitSet<32>));

    TSUNIT_EQUAL(64, ts::CompactBitSet<64>::SIZE);
    TSUNIT_EQUAL(63, ts::CompactBitSet<64>::MAX);
    TSUNIT_EQUAL(0xFFFFFFFFFFFFFFFF, ts::CompactBitSet<64>::ALL);
    TSUNIT_ASSUME(8 == sizeof(ts::CompactBitSet<64>));
}

void CompactBitSetTest::testOps14()
{
    typedef ts::CompactBitSet<14> Set;

    Set set1(0x0081);

    TSUNIT_ASSERT(set1.test(0));
    TSUNIT_ASSERT(!set1.test(1));
    TSUNIT_ASSERT(!set1.test(6));
    TSUNIT_ASSERT(set1.test(7));
    TSUNIT_ASSERT(!set1.test(8));
    TSUNIT_ASSERT(!set1.test(13));
    TSUNIT_ASSERT(!set1.test(14)); // out of range

    TSUNIT_ASSERT(!set1.none());
    TSUNIT_ASSERT(set1.any());
    TSUNIT_ASSERT(!set1.all());

    TSUNIT_EQUAL(0x0081, set1.toInt());

    set1.flip();
    TSUNIT_EQUAL(0x3F7E, set1.toInt());

    TSUNIT_ASSERT(!set1.test(0));
    TSUNIT_ASSERT(set1.test(1));
    TSUNIT_ASSERT(set1.test(6));
    TSUNIT_ASSERT(!set1.test(7));
    TSUNIT_ASSERT(set1.test(8));
    TSUNIT_ASSERT(set1.test(13));
    TSUNIT_ASSERT(!set1.test(14)); // out of range

    set1.reset(4);
    TSUNIT_EQUAL(0x3F6E, set1.toInt());
}
