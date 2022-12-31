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
//  TSUnit test suite for FloatingPoint class.
//
//----------------------------------------------------------------------------

#include "tsFloatingPoint.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class FloatingPointTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testConstructor();
    void testComparison();
    void testArithmetics();
    void testToInt();
    void testToString();
    void testFromString();

    TSUNIT_TEST_BEGIN(FloatingPointTest);
    TSUNIT_TEST(testConstructor);
    TSUNIT_TEST(testComparison);
    TSUNIT_TEST(testArithmetics);
    TSUNIT_TEST(testToInt);
    TSUNIT_TEST(testToString);
    TSUNIT_TEST(testFromString);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(FloatingPointTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void FloatingPointTest::beforeTest()
{
}

// Test suite cleanup method.
void FloatingPointTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void FloatingPointTest::testConstructor()
{
    typedef ts::FloatingPoint<double> Double;

    Double a1;
    TSUNIT_EQUAL(0, a1.toInt());
    TSUNIT_EQUAL(0.0, a1.toDouble());

    Double a2(12);
    TSUNIT_EQUAL(12, a2.toInt());
    TSUNIT_EQUAL(12.0, a2.toDouble());

    Double a3(-12.3);
    TSUNIT_EQUAL(-12, a3.toInt());
    TSUNIT_EQUAL(-12.3, a3.toDouble());

    Double a4(a3);
    TSUNIT_EQUAL(-12, a4.toInt());
    TSUNIT_EQUAL(-12.3, a4.toDouble());
}

void FloatingPointTest::testComparison()
{
    typedef ts::FloatingPoint<double> Double;

    TSUNIT_ASSERT(Double(1.2) == Double(1.2));
    TSUNIT_ASSERT(Double(1.2) != Double(-4.8));

    TSUNIT_ASSERT(Double(1.2) < Double(2.3));
    TSUNIT_ASSERT(Double(1.2) <= Double(2.3));

    TSUNIT_ASSERT(Double(3.2) > Double(2.3));
    TSUNIT_ASSERT(Double(3.2) >= Double(2.3));

    TSUNIT_ASSERT(Double(2) == 2);
    TSUNIT_ASSERT(Double(2) == 2.0);
    TSUNIT_ASSERT(Double(2.1) >= 2);
    TSUNIT_ASSERT(Double(1.9) <= 2);
    TSUNIT_ASSERT(Double(2.1) != 2);
    TSUNIT_ASSERT(Double(1.5) < 2.1);
    TSUNIT_ASSERT(Double(3.2) <= 3.22);
    TSUNIT_ASSERT(Double(5.2) > 5.1);
    TSUNIT_ASSERT(Double(5.2) >= 5.1);

    TSUNIT_ASSERT(2 == Double(2));
    TSUNIT_ASSERT(2.0 == Double(2));
    TSUNIT_ASSERT(2 >= Double(2.0));
    TSUNIT_ASSERT(2 <= Double(2.0));
    TSUNIT_ASSERT(2 != Double(3.2));
    TSUNIT_ASSERT(2 > Double(1.9999));
    TSUNIT_ASSERT(2 >= Double(1.9999));
    TSUNIT_ASSERT(2 < Double(5.2));
    TSUNIT_ASSERT(2 <= Double(5.2));
}

void FloatingPointTest::testArithmetics()
{
    typedef ts::FloatingPoint<double> Double;

    Double a1;
    TSUNIT_EQUAL(0.0, a1.toDouble());

    // Addition

    a1 = Double(1.2) + Double(3.2);
    TSUNIT_EQUAL(4.4, a1.toDouble());

    a1 = Double(1.2) + 4;
    TSUNIT_EQUAL(5.2, a1.toDouble());

    a1 = 2 + Double(1.2);
    TSUNIT_EQUAL(3.2, a1.toDouble());

    a1 += Double(2.3);
    TSUNIT_EQUAL(5.5, a1.toDouble());

    a1 += 2;
    TSUNIT_EQUAL(7.5, a1.toDouble());

    // Substraction

    a1 = Double(1.9) - Double(3.2);
    TSUNIT_EQUAL(-1.3, a1.toDouble());

    a1 = Double(1.2) - Double(2.3);
    TSUNIT_EQUAL(-1.1, a1.toDouble());

    a1 = Double(1.2) - 4;
    TSUNIT_EQUAL(-2.8, a1.toDouble());

    a1 = 2 - Double(5.2);
    TSUNIT_EQUAL(-3.2, a1.toDouble());

    a1 -= Double(2.3);
    TSUNIT_EQUAL(-5.5, a1.toDouble());

    a1 -= 2;
    TSUNIT_EQUAL(-7.5, a1.toDouble());

    // Multiplication

    a1 = Double(5.2) * Double(3.2);
    TSUNIT_EQUAL(16.64, a1.toDouble());

    a1 = Double(5.2) * 2;
    TSUNIT_EQUAL(10.4, a1.toDouble());

    a1 = 4 * Double(5.2);
    TSUNIT_EQUAL(20.8, a1.toDouble());

    a1 *= Double(5.3);
    TSUNIT_EQUAL(110.24, a1.toDouble());

    a1 *= 6;
    TSUNIT_EQUAL(661.44, a1.toDouble());

    // Division

    a1 = Double(5.2) / Double(3.2);
    TSUNIT_EQUAL(1.625, a1.toDouble());

    a1 = 10 / Double(2.5);
    TSUNIT_EQUAL(4.0, a1.toDouble());

    a1 = Double(5.4) / 2;
    TSUNIT_EQUAL(2.7, a1.toDouble());

    a1 /= Double(1.2);
    TSUNIT_EQUAL(2.25, a1.toDouble());

    a1 /= 4;
    TSUNIT_EQUAL(0.5625, a1.toDouble());
}

void FloatingPointTest::testToInt()
{
    typedef ts::FloatingPoint<double> Double;

    TSUNIT_EQUAL(3, Double(3.4999).toInt());
    TSUNIT_EQUAL(4, Double(3.5001).toInt());
    TSUNIT_EQUAL(-3, Double(-3.4999).toInt());
    TSUNIT_EQUAL(-4, Double(-3.5001).toInt());

    TSUNIT_EQUAL(3, Double(3.4999).toInt64());
    TSUNIT_EQUAL(4, Double(3.5001).toInt64());
    TSUNIT_EQUAL(-3, Double(-3.4999).toInt64());
    TSUNIT_EQUAL(-4, Double(-3.5001).toInt64());
}

void FloatingPointTest::testToString()
{
    typedef ts::FloatingPoint<double> Double;

    TSUNIT_EQUAL(u"12,345", Double(12345).toString());
    TSUNIT_EQUAL(u"-12,345.04", Double(-12345.04).toString());
    TSUNIT_EQUAL(u"0", Double().toString());
    TSUNIT_EQUAL(u"1.333,333", (Double(4) / Double(3)).toString());
    TSUNIT_EQUAL(u"1.333333", ts::UString::Format(u"%s", {Double(4) / Double(3)}));
    TSUNIT_EQUAL(u"1.333333", ts::UString::Format(u"%f", {Double(4) / Double(3)}));
    TSUNIT_EQUAL(u"1.333,333", ts::UString::Format(u"%'s", {Double(4) / Double(3)}));
    TSUNIT_EQUAL(u"1.333,333", ts::UString::Format(u"%'f", {Double(4) / Double(3)}));

    typedef ts::FloatingPoint<double, 2> Double2;
    TSUNIT_EQUAL(u"1.33", (Double2(4) / Double2(3)).toString());
    TSUNIT_EQUAL(u"1.33", ts::UString::Format(u"%s", {Double2(4) / Double2(3)}));
    TSUNIT_EQUAL(u"1.33", ts::UString::Format(u"%f", {Double2(4) / Double2(3)}));
}

void FloatingPointTest::testFromString()
{
    typedef ts::FloatingPoint<double> Double;

    Double a;

    TSUNIT_ASSERT(!a.fromString(u""));
    TSUNIT_ASSERT(!a.fromString(u"a1"));
    TSUNIT_ASSERT(!a.fromString(u"1/3a"));

    TSUNIT_ASSERT(a.fromString(u"0"));
    TSUNIT_EQUAL(0.0, a.toDouble());

    TSUNIT_ASSERT(a.fromString(u"  1.200 "));
    TSUNIT_EQUAL(1.2, a.toDouble());

    TSUNIT_ASSERT(a.fromString(u" -12,345.123,4"));
    TSUNIT_EQUAL(-12345.1234, a.toDouble());
}
