//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for Integer class.
//
//----------------------------------------------------------------------------

#include "tsInteger.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class IntegerTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testUnit();
    void testAssignment();
    void testComparison();
    void testBounds();
    void testToString();
    void testFromString();

    TSUNIT_TEST_BEGIN(IntegerTest);
    TSUNIT_TEST(testUnit);
    TSUNIT_TEST(testAssignment);
    TSUNIT_TEST(testComparison);
    TSUNIT_TEST(testBounds);
    TSUNIT_TEST(testToString);
    TSUNIT_TEST(testFromString);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(IntegerTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void IntegerTest::beforeTest()
{
}

// Test suite cleanup method.
void IntegerTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void IntegerTest::testUnit()
{
    typedef ts::Integer<int32_t> Int;

    Int i;
    Int n(-3);
    Int z(0);
    Int p(47);

    TSUNIT_EQUAL(-3, n.toInt());
    TSUNIT_EQUAL(0, z.toInt());
    TSUNIT_EQUAL(47, p.toInt());

    TSUNIT_EQUAL(7, (-Int(-7)).toInt());
    TSUNIT_EQUAL(-7, (-Int(7)).toInt());

    TSUNIT_EQUAL(23, (Int(21) + Int(2)).toInt());
    TSUNIT_EQUAL(30, (Int(21) + 9).toInt());
    TSUNIT_EQUAL(19, (Int(21) - Int(2)).toInt());
    TSUNIT_EQUAL(12, (Int(21) - 9).toInt());
    TSUNIT_EQUAL(42, (Int(21) * 2).toInt());
    TSUNIT_EQUAL(10, (Int(21) / 2).toInt());

    TSUNIT_EQUAL(30, (9 + Int(21)).toInt());
    TSUNIT_EQUAL(-12, (9 - Int(21)).toInt());
    TSUNIT_EQUAL(42, (2 * Int(21)).toInt());

    TSUNIT_EQUAL(21, (Int(3) * Int(7)).toInt());
    TSUNIT_EQUAL(21, (3 * Int(7)).toInt());
    TSUNIT_EQUAL(21, (Int(3) * 7).toInt());

    TSUNIT_EQUAL(2, (Int(10) / Int(4)).toInt());
    TSUNIT_EQUAL(2, (10 / Int(4)).toInt());
    TSUNIT_EQUAL(2, (Int(10) / 4).toInt());
}

void IntegerTest::testAssignment()
{
    typedef ts::Integer<int32_t> Int;

    Int n;
    TSUNIT_EQUAL(0, n.toInt());

    n = -12;
    TSUNIT_EQUAL(-12, n.toInt());

    n = 12;
    n += Int(4);
    TSUNIT_EQUAL(16, n.toInt());

    n += 2;
    TSUNIT_EQUAL(18, n.toInt());

    n -= Int(6);
    TSUNIT_EQUAL(12, n.toInt());

    n -= 2;
    TSUNIT_EQUAL(10, n.toInt());

    n *= Int(2);
    TSUNIT_EQUAL(20, n.toInt());

    n *= 3;
    TSUNIT_EQUAL(60, n.toInt());

    n /= Int(2);
    TSUNIT_EQUAL(30, n.toInt());

    n /= 4;
    TSUNIT_EQUAL(7, n.toInt());
}

void IntegerTest::testComparison()
{
    typedef ts::Integer<int32_t> Int;

    TSUNIT_ASSERT(Int(-211) == Int(-211));
    TSUNIT_ASSERT(Int(21) == 21);
    TSUNIT_ASSERT(21 == Int(21));

    TSUNIT_ASSERT(Int(-211) != Int(-212));
    TSUNIT_ASSERT(Int(21) != 22);
    TSUNIT_ASSERT(20 != Int(21));

    TSUNIT_ASSERT(Int(-2) < Int(2));
    TSUNIT_ASSERT(Int(2) < 3);
    TSUNIT_ASSERT(3 < Int(4));

    TSUNIT_ASSERT(Int(2) > Int(1));
    TSUNIT_ASSERT(Int(2) > -2);
    TSUNIT_ASSERT(4 > Int(2));

    TSUNIT_ASSERT(Int(-2) <= Int(2));
    TSUNIT_ASSERT(Int(2) <= 3);
    TSUNIT_ASSERT(3 <= Int(4));

    TSUNIT_ASSERT(Int(-2) <= Int(-2));
    TSUNIT_ASSERT(Int(2) <= 2);
    TSUNIT_ASSERT(3 <= Int(3));

    TSUNIT_ASSERT(Int(2) >= Int(1));
    TSUNIT_ASSERT(Int(2) >= -2);
    TSUNIT_ASSERT(4 >= Int(2));

    TSUNIT_ASSERT(Int(2) >= Int(2));
    TSUNIT_ASSERT(Int(2) >= 2);
    TSUNIT_ASSERT(4 >= Int(4));
}

void IntegerTest::testBounds()
{
    typedef ts::Integer<int16_t> Int;

    TSUNIT_EQUAL(-32768, Int::MIN.toInt());
    TSUNIT_EQUAL(32767, Int::MAX.toInt());
}

void IntegerTest::testToString()
{
    typedef ts::Integer<int32_t> Int;

    TSUNIT_EQUAL(u"1,234", Int(1234).toString());
    TSUNIT_EQUAL(u"   -56,789", Int(-56789).toString(10));
    TSUNIT_EQUAL(u"-56789", Int(-56789).toString(0, true, ts::CHAR_NULL));
}


void IntegerTest::testFromString()
{
    typedef ts::Integer<int32_t> Int;

    Int f0;

    TSUNIT_ASSERT(f0.fromString(u" 12"));
    TSUNIT_EQUAL(12, f0.toInt());

    TSUNIT_ASSERT(!f0.fromString(u" -12,345 =="));
    TSUNIT_EQUAL(-12345, f0.toInt());
}
