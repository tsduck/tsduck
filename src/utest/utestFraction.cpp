//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for Fraction class.
//
//----------------------------------------------------------------------------

#include "tsFraction.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class FractionTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Constructor);
    TSUNIT_DECLARE_TEST(Double);
    TSUNIT_DECLARE_TEST(Abs);
    TSUNIT_DECLARE_TEST(Min);
    TSUNIT_DECLARE_TEST(Max);
    TSUNIT_DECLARE_TEST(Proper);
    TSUNIT_DECLARE_TEST(Comparison);
    TSUNIT_DECLARE_TEST(Arithmetics);
    TSUNIT_DECLARE_TEST(ToString);
    TSUNIT_DECLARE_TEST(FromString);
};

TSUNIT_REGISTER(FractionTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Constructor)
{
    using Frac = ts::Fraction<int16_t>;

    Frac a1;
    TSUNIT_EQUAL(0, a1.toInt());
    TSUNIT_EQUAL(0, a1.numerator());
    TSUNIT_EQUAL(1, a1.denominator());

    Frac a2(12);
    TSUNIT_EQUAL(12, a2.toInt());
    TSUNIT_EQUAL(12, a2.numerator());
    TSUNIT_EQUAL(1, a2.denominator());

    Frac a3(12, -3);
    TSUNIT_EQUAL(-4, a3.toInt());
    TSUNIT_EQUAL(-4, a3.numerator());
    TSUNIT_EQUAL(1, a3.denominator());

    Frac a4(12, -3);
    TSUNIT_EQUAL(-4, a4.toInt());
    TSUNIT_EQUAL(-4, a4.numerator());
    TSUNIT_EQUAL(1, a4.denominator());

    Frac a5(12, -9);
    TSUNIT_EQUAL(-1, a5.toInt());
    TSUNIT_EQUAL(-4, a5.numerator());
    TSUNIT_EQUAL(3, a5.denominator());

    Frac a6(a5);
    TSUNIT_EQUAL(-1, a6.toInt());
    TSUNIT_EQUAL(-4, a6.numerator());
    TSUNIT_EQUAL(3, a6.denominator());

    a1 = a4;
    TSUNIT_EQUAL(-4, a1.toInt());
    TSUNIT_EQUAL(-4, a1.numerator());
    TSUNIT_EQUAL(1, a1.denominator());
}

TSUNIT_DEFINE_TEST(Double)
{
    TSUNIT_EQUAL(0.0, ts::Fraction<int>(0).toDouble());
    TSUNIT_EQUAL(-2.0, ts::Fraction<int>(4, -2).toDouble());
    TSUNIT_EQUAL(-2.5, ts::Fraction<int>(5, -2).toDouble());
    TSUNIT_EQUAL(2.5, ts::Fraction<int>(-100, -40).toDouble());
}

TSUNIT_DEFINE_TEST(Abs)
{
    using SF = ts::Fraction<int32_t>;
    using UF = ts::Fraction<uint32_t>;

    const UF a1(23, 7);
    const SF a2(45, 6);
    const SF a3(-45, 6);

    TSUNIT_EQUAL(23, a1.abs().numerator());
    TSUNIT_EQUAL(7, a1.abs().denominator());
    TSUNIT_EQUAL(15, a2.abs().numerator());
    TSUNIT_EQUAL(2, a2.abs().denominator());
    TSUNIT_EQUAL(15, a3.abs().numerator());
    TSUNIT_EQUAL(2, a3.abs().denominator());
}

TSUNIT_DEFINE_TEST(Min)
{
    using Frac = ts::Fraction<int32_t>;

    Frac a1(Frac(1, 2).min(Frac(2, 3)));
    TSUNIT_EQUAL(1, a1.numerator());
    TSUNIT_EQUAL(2, a1.denominator());

    a1 = Frac(1, 2).min(Frac(2, -3));
    TSUNIT_EQUAL(-2, a1.numerator());
    TSUNIT_EQUAL(3, a1.denominator());
}

TSUNIT_DEFINE_TEST(Max)
{
    using Frac = ts::Fraction<int32_t>;

    Frac a1(Frac(1, 2).max(Frac(2, 3)));
    TSUNIT_EQUAL(2, a1.numerator());
    TSUNIT_EQUAL(3, a1.denominator());

    a1 = Frac(1, -2).max(Frac(-2, 3));
    TSUNIT_EQUAL(-1, a1.numerator());
    TSUNIT_EQUAL(2, a1.denominator());
}

TSUNIT_DEFINE_TEST(Proper)
{
    using Frac = ts::Fraction<int32_t>;

    Frac a1(Frac(28, 6));
    TSUNIT_EQUAL(4, a1.proper());
    TSUNIT_EQUAL(2, a1.numerator());
    TSUNIT_EQUAL(3, a1.denominator());

    a1 = Frac(-14, 3);
    TSUNIT_EQUAL(-4, a1.proper());
    TSUNIT_EQUAL(-2, a1.numerator());
    TSUNIT_EQUAL(3, a1.denominator());
}

TSUNIT_DEFINE_TEST(Comparison)
{
    using Frac = ts::Fraction<int32_t>;

    TSUNIT_ASSERT(Frac(1, 2) == Frac(1, 2));
    TSUNIT_ASSERT(Frac(-1, 2) == Frac(1, -2));
    TSUNIT_ASSERT(Frac(1, 2) == Frac(4, 8));
    TSUNIT_ASSERT(Frac(1, 2) == Frac(-4, -8));
    TSUNIT_ASSERT(Frac(1, 2) != Frac(-4, 8));
    TSUNIT_ASSERT(Frac(1, 2) != Frac(1, 3));
    TSUNIT_ASSERT(Frac(1, 2) != Frac(3, 2));

    TSUNIT_ASSERT(Frac(1, 2) < Frac(2, 3));
    TSUNIT_ASSERT(Frac(1, 2) <= Frac(2, 3));
    TSUNIT_ASSERT(Frac(1, 2) < Frac(3, 2));
    TSUNIT_ASSERT(Frac(1, 2) <= Frac(3, 2));

    TSUNIT_ASSERT(Frac(3, 2) > Frac(2, 3));
    TSUNIT_ASSERT(Frac(3, 2) >= Frac(2, 3));
    TSUNIT_ASSERT(Frac(3, 2) > Frac(1, 2));
    TSUNIT_ASSERT(Frac(3, 2) >= Frac(1, 2));

    TSUNIT_ASSERT(Frac(4, 2) == 2);
    TSUNIT_ASSERT(Frac(4, 2) >= 2);
    TSUNIT_ASSERT(Frac(4, 2) <= 2);
    TSUNIT_ASSERT(Frac(3, 2) != 2);
    TSUNIT_ASSERT(Frac(3, 2) < 2);
    TSUNIT_ASSERT(Frac(3, 2) <= 2);
    TSUNIT_ASSERT(Frac(5, 2) > 2);
    TSUNIT_ASSERT(Frac(5, 2) >= 2);

    TSUNIT_ASSERT(2 == Frac(4, 2));
    TSUNIT_ASSERT(2 >= Frac(4, 2));
    TSUNIT_ASSERT(2 <= Frac(4, 2));
    TSUNIT_ASSERT(2 != Frac(3, 2));
    TSUNIT_ASSERT(2 > Frac(3, 2));
    TSUNIT_ASSERT(2 >= Frac(3, 2));
    TSUNIT_ASSERT(2 < Frac(5, 2));
    TSUNIT_ASSERT(2 <= Frac(5, 2));
}

TSUNIT_DEFINE_TEST(Arithmetics)
{
    using Frac = ts::Fraction<int32_t>;

    Frac a1;
    TSUNIT_EQUAL(0, a1.numerator());
    TSUNIT_EQUAL(1, a1.denominator());

    // Addition

    a1 = Frac(1, 2) + Frac(3, 2);
    TSUNIT_EQUAL(2, a1.numerator());
    TSUNIT_EQUAL(1, a1.denominator());

    a1 = Frac(1, 2) + Frac(1, 3);
    TSUNIT_EQUAL(5, a1.numerator());
    TSUNIT_EQUAL(6, a1.denominator());

    a1 = Frac(1, 2) + 4;
    TSUNIT_EQUAL(9, a1.numerator());
    TSUNIT_EQUAL(2, a1.denominator());

    a1 = 2 + Frac(1, 2);
    TSUNIT_EQUAL(5, a1.numerator());
    TSUNIT_EQUAL(2, a1.denominator());

    a1 += Frac(2, 3);
    TSUNIT_EQUAL(19, a1.numerator());
    TSUNIT_EQUAL(6, a1.denominator());

    a1 += 2;
    TSUNIT_EQUAL(31, a1.numerator());
    TSUNIT_EQUAL(6, a1.denominator());

    // Substraction

    a1 = Frac(1, 2) - Frac(3, 2);
    TSUNIT_EQUAL(-1, a1.numerator());
    TSUNIT_EQUAL(1, a1.denominator());

    a1 = Frac(1, 2) - Frac(2, 3);
    TSUNIT_EQUAL(-1, a1.numerator());
    TSUNIT_EQUAL(6, a1.denominator());

    a1 = Frac(1, 2) - 4;
    TSUNIT_EQUAL(-7, a1.numerator());
    TSUNIT_EQUAL(2, a1.denominator());

    a1 = 2 - Frac(5, 2);
    TSUNIT_EQUAL(-1, a1.numerator());
    TSUNIT_EQUAL(2, a1.denominator());

    a1 -= Frac(2, 3);
    TSUNIT_EQUAL(-7, a1.numerator());
    TSUNIT_EQUAL(6, a1.denominator());

    a1 -= 2;
    TSUNIT_EQUAL(-19, a1.numerator());
    TSUNIT_EQUAL(6, a1.denominator());

    // Multiplication

    a1 = Frac(5, 2) * Frac(3, 2);
    TSUNIT_EQUAL(15, a1.numerator());
    TSUNIT_EQUAL(4, a1.denominator());

    a1 = Frac(5, 2) * 3;
    TSUNIT_EQUAL(15, a1.numerator());
    TSUNIT_EQUAL(2, a1.denominator());

    a1 = 4 * Frac(5, 2);
    TSUNIT_EQUAL(10, a1.numerator());
    TSUNIT_EQUAL(1, a1.denominator());

    a1 *= Frac(5, 3);
    TSUNIT_EQUAL(50, a1.numerator());
    TSUNIT_EQUAL(3, a1.denominator());

    a1 *= 6;
    TSUNIT_EQUAL(100, a1.numerator());
    TSUNIT_EQUAL(1, a1.denominator());

    // Division

    a1 = Frac(5, 2) / Frac(3, 2);
    TSUNIT_EQUAL(5, a1.numerator());
    TSUNIT_EQUAL(3, a1.denominator());

    a1 = Frac(5, 4) / 2;
    TSUNIT_EQUAL(5, a1.numerator());
    TSUNIT_EQUAL(8, a1.denominator());

    a1 = 2 / Frac(5, 4);
    TSUNIT_EQUAL(8, a1.numerator());
    TSUNIT_EQUAL(5, a1.denominator());

    a1 /= Frac(3, 4);
    TSUNIT_EQUAL(32, a1.numerator());
    TSUNIT_EQUAL(15, a1.denominator());

    a1 /= 4;
    TSUNIT_EQUAL(8, a1.numerator());
    TSUNIT_EQUAL(15, a1.denominator());
}

TSUNIT_DEFINE_TEST(ToString)
{
    using Frac = ts::Fraction<int32_t>;
    TSUNIT_EQUAL(u"12,345", Frac(12345).toString());
    TSUNIT_EQUAL(u"12,345/4", Frac(12345, 4).toString());
    TSUNIT_EQUAL(u"0", Frac().toString());
    TSUNIT_EQUAL(u"-1/2", Frac(1, -2).toString());
}

TSUNIT_DEFINE_TEST(FromString)
{
    ts::Fraction<int32_t> a;

    TSUNIT_ASSERT(!a.fromString(u""));
    TSUNIT_ASSERT(!a.fromString(u"a1"));
    TSUNIT_ASSERT(!a.fromString(u"1/3a"));

    TSUNIT_ASSERT(a.fromString(u"0"));
    TSUNIT_EQUAL(0, a.numerator());
    TSUNIT_EQUAL(1, a.denominator());

    TSUNIT_ASSERT(a.fromString(u" -12,345 / 56,789"));
    TSUNIT_EQUAL(-12345, a.numerator());
    TSUNIT_EQUAL(56789, a.denominator());

    TSUNIT_ASSERT(a.fromString(u"56789/12345"));
    TSUNIT_EQUAL(56789, a.numerator());
    TSUNIT_EQUAL(12345, a.denominator());

    TSUNIT_ASSERT(a.fromString(u" 123456 "));
    TSUNIT_EQUAL(123456, a.numerator());
    TSUNIT_EQUAL(1, a.denominator());
}
