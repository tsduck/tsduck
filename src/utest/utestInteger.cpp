//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
    TSUNIT_DECLARE_TEST(Unit);
    TSUNIT_DECLARE_TEST(Assignment);
    TSUNIT_DECLARE_TEST(Comparison);
    TSUNIT_DECLARE_TEST(Bounds);
    TSUNIT_DECLARE_TEST(ToString);
    TSUNIT_DECLARE_TEST(FromString);
};

TSUNIT_REGISTER(IntegerTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Unit)
{
    using Int = ts::Integer<int32_t>;

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

TSUNIT_DEFINE_TEST(Assignment)
{
    using Int = ts::Integer<int32_t>;

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

TSUNIT_DEFINE_TEST(Comparison)
{
    using Int = ts::Integer<int32_t>;

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

TSUNIT_DEFINE_TEST(Bounds)
{
    using Int = ts::Integer<int16_t>;

    TSUNIT_EQUAL(-32768, Int::MIN.toInt());
    TSUNIT_EQUAL(32767, Int::MAX.toInt());
}

TSUNIT_DEFINE_TEST(ToString)
{
    using Int = ts::Integer<int32_t>;

    TSUNIT_EQUAL(u"1,234", Int(1234).toString());
    TSUNIT_EQUAL(u"   -56,789", Int(-56789).toString(10));
    TSUNIT_EQUAL(u"-56789", Int(-56789).toString(0, true, ts::CHAR_NULL));
}


TSUNIT_DEFINE_TEST(FromString)
{
    using Int = ts::Integer<int32_t>;

    Int f0;

    TSUNIT_ASSERT(f0.fromString(u" 12"));
    TSUNIT_EQUAL(12, f0.toInt());

    TSUNIT_ASSERT(!f0.fromString(u" -12,345 =="));
    TSUNIT_EQUAL(-12345, f0.toInt());
}
