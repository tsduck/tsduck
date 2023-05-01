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
//  TSUnit test suite for tsIntegerUtils.h
//
//----------------------------------------------------------------------------

#include "tsIntegerUtils.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class IntegerUtilsTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testMakeSigned();
    void testbounded_add();
    void testbounded_sub();
    void testround_down();
    void testround_up();
    void testSignExtend();
    void testBitSize();
    void testPower10();
    void testBoundCheck();
    void testBoundedCast();
    void testGCD();
    void testOverflow();
    void testSmallerUnsigned();

    TSUNIT_TEST_BEGIN(IntegerUtilsTest);
    TSUNIT_TEST(testMakeSigned);
    TSUNIT_TEST(testbounded_add);
    TSUNIT_TEST(testbounded_sub);
    TSUNIT_TEST(testround_down);
    TSUNIT_TEST(testround_up);
    TSUNIT_TEST(testSignExtend);
    TSUNIT_TEST(testBitSize);
    TSUNIT_TEST(testPower10);
    TSUNIT_TEST(testBoundCheck);
    TSUNIT_TEST(testBoundedCast);
    TSUNIT_TEST(testGCD);
    TSUNIT_TEST(testOverflow);
    TSUNIT_TEST(testSmallerUnsigned);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(IntegerUtilsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void IntegerUtilsTest::beforeTest()
{
}

// Test suite cleanup method.
void IntegerUtilsTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void IntegerUtilsTest::testMakeSigned()
{
    TSUNIT_ASSERT(std::is_unsigned<bool>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<bool>::type>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<signed char>::type>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<unsigned char>::type>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<int8_t>::type>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<uint8_t>::type>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<int16_t>::type>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<uint16_t>::type>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<int32_t>::type>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<uint32_t>::type>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<int64_t>::type>::value);
    TSUNIT_ASSERT(std::is_signed<ts::make_signed<uint64_t>::type>::value);

    TSUNIT_EQUAL(1, sizeof(ts::make_signed<bool>::type));
    TSUNIT_EQUAL(1, sizeof(ts::make_signed<signed char>::type));
    TSUNIT_EQUAL(2, sizeof(ts::make_signed<unsigned char>::type));
    TSUNIT_EQUAL(1, sizeof(ts::make_signed<int8_t>::type));
    TSUNIT_EQUAL(2, sizeof(ts::make_signed<uint8_t>::type));
    TSUNIT_EQUAL(2, sizeof(ts::make_signed<int16_t>::type));
    TSUNIT_EQUAL(4, sizeof(ts::make_signed<uint16_t>::type));
    TSUNIT_EQUAL(4, sizeof(ts::make_signed<int32_t>::type));
    TSUNIT_EQUAL(8, sizeof(ts::make_signed<uint32_t>::type));
    TSUNIT_EQUAL(8, sizeof(ts::make_signed<int64_t>::type));
    TSUNIT_EQUAL(8, sizeof(ts::make_signed<uint64_t>::type));

    TSUNIT_ASSERT(std::is_floating_point<ts::make_signed<float>::type>::value);
    TSUNIT_ASSERT(std::is_floating_point<ts::make_signed<double>::type>::value);
    TSUNIT_EQUAL(sizeof(float), sizeof(ts::make_signed<float>::type));
    TSUNIT_EQUAL(sizeof(double), sizeof(ts::make_signed<double>::type));
}

void IntegerUtilsTest::testbounded_add()
{
    TSUNIT_EQUAL(201, ts::bounded_add(uint8_t(1),   uint8_t(200)));
    TSUNIT_EQUAL(255, ts::bounded_add(uint8_t(0),   uint8_t(255)));
    TSUNIT_EQUAL(255, ts::bounded_add(uint8_t(1),   uint8_t(255)));
    TSUNIT_EQUAL(255, ts::bounded_add(uint8_t(100), uint8_t(200)));

    TSUNIT_EQUAL(120,  ts::bounded_add(int8_t(10),   int8_t(110)));
    TSUNIT_EQUAL(127,  ts::bounded_add(int8_t(100),  int8_t(80)));
    TSUNIT_EQUAL(80,   ts::bounded_add(int8_t(100),  int8_t(-20)));
    TSUNIT_EQUAL(-120, ts::bounded_add(int8_t(-100), int8_t(-20)));
    TSUNIT_EQUAL(-128, ts::bounded_add(int8_t(-100), int8_t(-60)));
}

void IntegerUtilsTest::testbounded_sub()
{
    TSUNIT_EQUAL(80,  ts::bounded_sub(uint8_t(100), uint8_t(20)));
    TSUNIT_EQUAL(0,   ts::bounded_sub(uint8_t(100), uint8_t(200)));
    TSUNIT_EQUAL(10,  ts::bounded_sub(uint8_t(10),  uint8_t(0)));
    TSUNIT_EQUAL(0,   ts::bounded_sub(uint8_t(0),   uint8_t(10)));
    TSUNIT_EQUAL(255, ts::bounded_sub(uint8_t(255), uint8_t(0)));

    TSUNIT_EQUAL(10,   ts::bounded_sub(int8_t(20),   int8_t(10)));
    TSUNIT_EQUAL(-10,  ts::bounded_sub(int8_t(20),   int8_t(30)));
    TSUNIT_EQUAL(-50,  ts::bounded_sub(int8_t(-20),  int8_t(30)));
    TSUNIT_EQUAL(127,  ts::bounded_sub(int8_t(100),  int8_t(-50)));
    TSUNIT_EQUAL(-128, ts::bounded_sub(int8_t(-100), int8_t(40)));
}

void IntegerUtilsTest::testround_down()
{
    TSUNIT_EQUAL(-1, -11 % 5);
    TSUNIT_EQUAL(-4, -14 % 5);
    TSUNIT_EQUAL(0,  -15 % 5);

    TSUNIT_EQUAL(20, ts::round_down(20, 5));
    TSUNIT_EQUAL(20, ts::round_down(24, 5));

    TSUNIT_EQUAL(-20, ts::round_down(-20, 5));
    TSUNIT_EQUAL(-25, ts::round_down(-21, 5));
    TSUNIT_EQUAL(-25, ts::round_down(-24, -5));
    TSUNIT_EQUAL(-25, ts::round_down(-25, 5));

    TSUNIT_EQUAL(10, ts::round_down(uint32_t(10), uint32_t(5)));
    TSUNIT_EQUAL(10, ts::round_down(uint32_t(14), uint32_t(5)));

    TSUNIT_EQUAL(10, ts::round_down(int8_t(10), int8_t(5)));
    TSUNIT_EQUAL(10, ts::round_down(int8_t(14), int8_t(5)));

    TSUNIT_EQUAL(-10, ts::round_down(int8_t(-10), int8_t(5)));
    TSUNIT_EQUAL(-15, ts::round_down(int8_t(-11), int8_t(5)));
    TSUNIT_EQUAL(-15, ts::round_down(int8_t(-14), int8_t(5)));
    TSUNIT_EQUAL(-15, ts::round_down(int8_t(-15), int8_t(5)));

    TSUNIT_EQUAL(10, ts::round_down(10, 0));
    TSUNIT_EQUAL(10, ts::round_down(10, 1));
    TSUNIT_EQUAL(-10, ts::round_down(-10, 0));
    TSUNIT_EQUAL(-10, ts::round_down(-10, 1));

    TSUNIT_EQUAL(0, ts::round_down(0, 0));
    TSUNIT_EQUAL(0, ts::round_down(0, 1));
    TSUNIT_EQUAL(0, ts::round_down(0, -27));
}

void IntegerUtilsTest::testround_up()
{
    TSUNIT_EQUAL(20, ts::round_up(20, 5));
    TSUNIT_EQUAL(25, ts::round_up(21, 5));
    TSUNIT_EQUAL(25, ts::round_up(24, -5));

    TSUNIT_EQUAL(-20, ts::round_up(-20, 5));
    TSUNIT_EQUAL(-20, ts::round_up(-21, 5));
    TSUNIT_EQUAL(-20, ts::round_up(-24, 5));
    TSUNIT_EQUAL(-25, ts::round_up(-25, 5));

    TSUNIT_EQUAL(10, ts::round_up(uint32_t(10), uint32_t(5)));
    TSUNIT_EQUAL(15, ts::round_up(uint32_t(11), uint32_t(5)));
    TSUNIT_EQUAL(15, ts::round_up(uint32_t(14), uint32_t(5)));

    TSUNIT_EQUAL(10, ts::round_up(int8_t(10), int8_t(5)));
    TSUNIT_EQUAL(15, ts::round_up(int8_t(11), int8_t(5)));
    TSUNIT_EQUAL(15, ts::round_up(int8_t(14), int8_t(5)));

    TSUNIT_EQUAL(-10, ts::round_up(int8_t(-10), int8_t(5)));
    TSUNIT_EQUAL(-10, ts::round_up(int8_t(-11), int8_t(5)));
    TSUNIT_EQUAL(-10, ts::round_up(int8_t(-14), int8_t(5)));
    TSUNIT_EQUAL(-15, ts::round_up(int8_t(-15), int8_t(5)));

    TSUNIT_EQUAL(10, ts::round_up(10, 0));
    TSUNIT_EQUAL(10, ts::round_up(10, 1));
    TSUNIT_EQUAL(-10, ts::round_up(-10, 0));
    TSUNIT_EQUAL(-10, ts::round_up(-10, 1));

    TSUNIT_EQUAL(0, ts::round_up(0, 0));
    TSUNIT_EQUAL(0, ts::round_up(0, 1));
    TSUNIT_EQUAL(0, ts::round_up(0, -27));
}

void IntegerUtilsTest::testSignExtend()
{
    TSUNIT_EQUAL(25, ts::SignExtend(int32_t(25), 12));
    TSUNIT_EQUAL(0x07FF, ts::SignExtend(int16_t(0x07FF), 12));
    TSUNIT_EQUAL(-1, ts::SignExtend(int16_t(0x0FFF), 12));
    TSUNIT_EQUAL(-2047, ts::SignExtend(int16_t(0x0801), 12));
    TSUNIT_EQUAL(-2048, ts::SignExtend(int16_t(0x2800), 12));
}

void IntegerUtilsTest::testBitSize()
{
    TSUNIT_EQUAL(1, ts::BitSize(uint8_t(0)));
    TSUNIT_EQUAL(1, ts::BitSize(int8_t(0)));
    TSUNIT_EQUAL(1, ts::BitSize(uint8_t(1)));
    TSUNIT_EQUAL(1, ts::BitSize(int8_t(1)));
    TSUNIT_EQUAL(2, ts::BitSize(uint8_t(2)));
    TSUNIT_EQUAL(2, ts::BitSize(int8_t(2)));
    TSUNIT_EQUAL(3, ts::BitSize(uint8_t(5)));
    TSUNIT_EQUAL(3, ts::BitSize(int8_t(5)));
    TSUNIT_EQUAL(8, ts::BitSize(int8_t(-1)));
    TSUNIT_EQUAL(51, ts::BitSize(TS_UCONST64(0x000500000A003000)));
}

void IntegerUtilsTest::testPower10()
{
    TSUNIT_EQUAL(1, ts::Power10(0));
    TSUNIT_EQUAL(10, ts::Power10(1));
    TSUNIT_EQUAL(100, ts::Power10(2));
    TSUNIT_EQUAL(1000000, ts::Power10(6));
    TSUNIT_EQUAL(TS_UCONST64(1000000000000000), ts::Power10(15));

    TSUNIT_EQUAL(1, (ts::static_power10<uint8_t, 0>::value));
    TSUNIT_EQUAL(1, (ts::static_power10<int, 0>::value));
    TSUNIT_EQUAL(10, (ts::static_power10<uint8_t, 1>::value));
    TSUNIT_EQUAL(10, (ts::static_power10<int, 1>::value));
    TSUNIT_EQUAL(100, (ts::static_power10<uint8_t, 2>::value));
    TSUNIT_EQUAL(100, (ts::static_power10<int, 2>::value));
    TSUNIT_EQUAL(1000000, (ts::static_power10<uint32_t, 6>::value));
    TSUNIT_EQUAL(1000000, (ts::static_power10<uint64_t, 6>::value));
    TSUNIT_EQUAL(TS_UCONST64(1000000000000000), (ts::static_power10<uint64_t, 15>::value));
}

void IntegerUtilsTest::testBoundCheck()
{
    TSUNIT_ASSERT(ts::bound_check<uint8_t>(int(20)));
    TSUNIT_ASSERT(ts::bound_check<uint8_t>(int(255)));
    TSUNIT_ASSERT(!ts::bound_check<uint8_t>(int(256)));
    TSUNIT_ASSERT(!ts::bound_check<uint8_t>(int(-1)));

    TSUNIT_ASSERT(ts::bound_check<int8_t>(uint32_t(20)));
    TSUNIT_ASSERT(ts::bound_check<int8_t>(int32_t(20)));
    TSUNIT_ASSERT(ts::bound_check<int8_t>(int32_t(-20)));
    TSUNIT_ASSERT(!ts::bound_check<int8_t>(int32_t(-200)));
    TSUNIT_ASSERT(!ts::bound_check<int8_t>(int32_t(200)));
    TSUNIT_ASSERT(ts::bound_check<int8_t>(int32_t(-128)));
    TSUNIT_ASSERT(!ts::bound_check<int8_t>(int32_t(-129)));
    TSUNIT_ASSERT(ts::bound_check<int8_t>(uint32_t(127)));
    TSUNIT_ASSERT(!ts::bound_check<int8_t>(uint32_t(128)));
}

void IntegerUtilsTest::testBoundedCast()
{
    TSUNIT_EQUAL(20, ts::bounded_cast<uint8_t>(int(20)));
    TSUNIT_EQUAL(0, ts::bounded_cast<uint8_t>(-20));
    TSUNIT_EQUAL(255, ts::bounded_cast<uint8_t>(2000));

    TSUNIT_EQUAL(-128, ts::bounded_cast<int8_t>(int16_t(-1000)));
    TSUNIT_EQUAL(-100, ts::bounded_cast<int8_t>(int16_t(-100)));
    TSUNIT_EQUAL(100, ts::bounded_cast<int8_t>(int16_t(100)));
    TSUNIT_EQUAL(127, ts::bounded_cast<int8_t>(int16_t(1000)));
}

void IntegerUtilsTest::testGCD()
{
    TSUNIT_EQUAL(0,  ts::GCD(0, 0));
    TSUNIT_EQUAL(12, ts::GCD(0, 12));
    TSUNIT_EQUAL(12, ts::GCD(12, 0));
    TSUNIT_EQUAL(1,  ts::GCD(-7 * 3 * 2, 11 * 5));
    TSUNIT_EQUAL(3,  ts::GCD(7 * 3 * 2, 11 * 5 * 3));
    TSUNIT_EQUAL(14, ts::GCD(7 * 3 * 2, -7 * 5 * 2));

    TSUNIT_EQUAL(0,  ts::GCD<uint32_t>(0, 0));
    TSUNIT_EQUAL(12, ts::GCD<uint32_t>(0, 12));
    TSUNIT_EQUAL(12, ts::GCD<uint32_t>(12, 0));
    TSUNIT_EQUAL(1,  ts::GCD<uint32_t>(7 * 3 * 2, 11 * 5));
    TSUNIT_EQUAL(3,  ts::GCD<uint32_t>(7 * 3 * 2, 11 * 5 * 3));
    TSUNIT_EQUAL(14, ts::GCD<uint32_t>(7 * 3 * 2, 7 * 5 * 2));
}

void IntegerUtilsTest::testOverflow()
{
    int64_t a = 292021270;
    int64_t b = 31590000000;
    int64_t res = a * b;
    TSUNIT_ASSERT(res < 0);
    TSUNIT_ASSERT(ts::mul_overflow(a, b, res));
    TSUNIT_ASSERT(ts::mul_overflow(a, b));
}

void IntegerUtilsTest::testSmallerUnsigned()
{
    TSUNIT_EQUAL(1, sizeof(ts::smaller_unsigned<1>::type));
    TSUNIT_ASSERT((std::is_same<uint8_t, ts::smaller_unsigned<1>::type>::value));

    TSUNIT_EQUAL(1, sizeof(ts::smaller_unsigned<8>::type));
    TSUNIT_ASSERT((std::is_same<uint8_t, ts::smaller_unsigned<8>::type>::value));

    TSUNIT_EQUAL(2, sizeof(ts::smaller_unsigned<9>::type));
    TSUNIT_ASSERT((std::is_same<uint16_t, ts::smaller_unsigned<9>::type>::value));

    TSUNIT_EQUAL(2, sizeof(ts::smaller_unsigned<16>::type));
    TSUNIT_ASSERT((std::is_same<uint16_t, ts::smaller_unsigned<16>::type>::value));

    TSUNIT_EQUAL(4, sizeof(ts::smaller_unsigned<17>::type));
    TSUNIT_ASSERT((std::is_same<uint32_t, ts::smaller_unsigned<17>::type>::value));

    TSUNIT_EQUAL(4, sizeof(ts::smaller_unsigned<32>::type));
    TSUNIT_ASSERT((std::is_same<uint32_t, ts::smaller_unsigned<32>::type>::value));

    TSUNIT_EQUAL(8, sizeof(ts::smaller_unsigned<33>::type));
    TSUNIT_ASSERT((std::is_same<uint64_t, ts::smaller_unsigned<33>::type>::value));

    TSUNIT_EQUAL(8, sizeof(ts::smaller_unsigned<64>::type));
    TSUNIT_ASSERT((std::is_same<uint64_t, ts::smaller_unsigned<64>::type>::value));

    TSUNIT_ASSERT(std::is_void<ts::smaller_unsigned<65>::type>::value);
}
