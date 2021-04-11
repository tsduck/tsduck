//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//  TSUnit test suite for FixedPoint class.
//
//----------------------------------------------------------------------------

#include "tsFixedPoint.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class FixedPointTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testUnit();
    void testSubUnit();

    TSUNIT_TEST_BEGIN(FixedPointTest);
    TSUNIT_TEST(testUnit);
    TSUNIT_TEST(testSubUnit);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(FixedPointTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void FixedPointTest::beforeTest()
{
}

// Test suite cleanup method.
void FixedPointTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void FixedPointTest::testUnit()
{
    typedef ts::FixedPoint<int32_t, 0> Int;

    Int i;
    Int n(-3);
    Int z(0);
    Int p(47);

    TSUNIT_EQUAL(4, sizeof(Int));
    TSUNIT_EQUAL(4, sizeof(n));
    TSUNIT_EQUAL(0, Int::PRECISION);
    TSUNIT_EQUAL(1, Int::FACTOR);
    TSUNIT_EQUAL(-3, n.toInt());
    TSUNIT_EQUAL(0, z.toInt());
    TSUNIT_EQUAL(47, p.toInt());
    TSUNIT_EQUAL(-3, n.raw());
    TSUNIT_EQUAL(0, z.raw());
    TSUNIT_EQUAL(47, p.raw());
    TSUNIT_EQUAL(21, Int(21, false).toInt());
    TSUNIT_EQUAL(21, Int(21, true).toInt());

    TSUNIT_EQUAL(7, (-Int(-7)).toInt());
    TSUNIT_EQUAL(-7, (-Int(7)).toInt());

    TSUNIT_EQUAL(23, (Int(21) + Int(2)).toInt());
    TSUNIT_EQUAL(30, (Int(21) + 9).toInt());
    TSUNIT_EQUAL(30, (9 + Int(21)).toInt());
    TSUNIT_EQUAL(19, (Int(21) - Int(2)).toInt());
    TSUNIT_EQUAL(12, (Int(21) - 9).toInt());
    TSUNIT_EQUAL(-12, (9 - Int(21)).toInt());
    TSUNIT_EQUAL(42, (Int(21) * 2).toInt());
    TSUNIT_EQUAL(42, (2 * Int(21)).toInt());
    TSUNIT_EQUAL(10, (Int(21) / 2).toInt());

    TSUNIT_ASSERT(Int::FromString(i, u" 12"));
    TSUNIT_EQUAL(12, i.toInt());
    TSUNIT_ASSERT(!Int::FromString(i, u" -12,345 =="));
    TSUNIT_EQUAL(-12345, i.toInt());

    TSUNIT_EQUAL(u"1,234", Int(1234).toString());
    TSUNIT_EQUAL(u"   -56,789", Int(-56789).toString(10));
}

void FixedPointTest::testSubUnit()
{
    typedef ts::FixedPoint<int32_t, 3> Int;

    Int i;
    Int n(-3);
    Int z(0);
    Int p(47);

    TSUNIT_EQUAL(4, sizeof(Int));
    TSUNIT_EQUAL(4, sizeof(n));
    TSUNIT_EQUAL(3, Int::PRECISION);
    TSUNIT_EQUAL(1000, Int::FACTOR);
    TSUNIT_EQUAL(-3, n.toInt());
    TSUNIT_EQUAL(0, z.toInt());
    TSUNIT_EQUAL(47, p.toInt());
    TSUNIT_EQUAL(-3000, n.raw());
    TSUNIT_EQUAL(0, z.raw());
    TSUNIT_EQUAL(47000, p.raw());
    TSUNIT_EQUAL(21, Int(21, false).toInt());
    TSUNIT_EQUAL(21, Int(21234, true).toInt());

    TSUNIT_EQUAL(7, (-Int(-7)).toInt());
    TSUNIT_EQUAL(-7, (-Int(7)).toInt());

    TSUNIT_EQUAL(23, (Int(21) + Int(2)).toInt());
    TSUNIT_EQUAL(30, (Int(21) + 9).toInt());
    TSUNIT_EQUAL(30, (9 + Int(21)).toInt());
    TSUNIT_EQUAL(19, (Int(21) - Int(2)).toInt());
    TSUNIT_EQUAL(12, (Int(21) - 9).toInt());
    TSUNIT_EQUAL(-12, (9 - Int(21)).toInt());
    TSUNIT_EQUAL(42, (Int(21) * 2).toInt());
    TSUNIT_EQUAL(42, (2 * Int(21)).toInt());
    TSUNIT_EQUAL(10, (Int(21) / 2).toInt());

    TSUNIT_ASSERT(Int::FromString(i, u" 12.3"));
    TSUNIT_EQUAL(12, i.toInt());
    TSUNIT_EQUAL(12300, i.raw());
    TSUNIT_ASSERT(!Int::FromString(i, u" -12,345.6789 =="));
    TSUNIT_EQUAL(-12345, i.toInt());
    TSUNIT_EQUAL(-12345678, i.raw());

    TSUNIT_EQUAL(u"1,234", Int(1234).toString());
    TSUNIT_EQUAL(u"1,234.5", Int(1234500, true).toString());
    TSUNIT_EQUAL(u"1,234.567", Int(1234567, true).toString());
    TSUNIT_EQUAL(u"   -56|789.000", Int(-56789).toString(14, true, u"|", true, true));
    TSUNIT_EQUAL(u"   +56|789.000", Int(56789).toString(14, true, u"|", true, true));
}
