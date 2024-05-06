//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(MakeSigned);
    TSUNIT_DECLARE_TEST(bounded_add);
    TSUNIT_DECLARE_TEST(bounded_sub);
    TSUNIT_DECLARE_TEST(round_down);
    TSUNIT_DECLARE_TEST(round_up);
    TSUNIT_DECLARE_TEST(SignExtend);
    TSUNIT_DECLARE_TEST(BitSize);
    TSUNIT_DECLARE_TEST(Power10);
    TSUNIT_DECLARE_TEST(BoundCheck);
    TSUNIT_DECLARE_TEST(BoundedCast);
    TSUNIT_DECLARE_TEST(GCD);
    TSUNIT_DECLARE_TEST(Overflow);
    TSUNIT_DECLARE_TEST(SmallerUnsigned);
};

TSUNIT_REGISTER(IntegerUtilsTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(MakeSigned)
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

TSUNIT_DEFINE_TEST(bounded_add)
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

TSUNIT_DEFINE_TEST(bounded_sub)
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

TSUNIT_DEFINE_TEST(round_down)
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

TSUNIT_DEFINE_TEST(round_up)
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

TSUNIT_DEFINE_TEST(SignExtend)
{
    TSUNIT_EQUAL(25, ts::SignExtend(int32_t(25), 12));
    TSUNIT_EQUAL(0x07FF, ts::SignExtend(int16_t(0x07FF), 12));
    TSUNIT_EQUAL(-1, ts::SignExtend(int16_t(0x0FFF), 12));
    TSUNIT_EQUAL(-2047, ts::SignExtend(int16_t(0x0801), 12));
    TSUNIT_EQUAL(-2048, ts::SignExtend(int16_t(0x2800), 12));
}

TSUNIT_DEFINE_TEST(BitSize)
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
    TSUNIT_EQUAL(51, ts::BitSize(0x000500000A003000));
}

TSUNIT_DEFINE_TEST(Power10)
{
    TSUNIT_EQUAL(1, ts::Power10(0));
    TSUNIT_EQUAL(10, ts::Power10(1));
    TSUNIT_EQUAL(100, ts::Power10(2));
    TSUNIT_EQUAL(1000000, ts::Power10(6));
    TSUNIT_EQUAL(1000000000000000, ts::Power10(15));

    TSUNIT_EQUAL(1, (ts::static_power10<uint8_t, 0>::value));
    TSUNIT_EQUAL(1, (ts::static_power10<int, 0>::value));
    TSUNIT_EQUAL(10, (ts::static_power10<uint8_t, 1>::value));
    TSUNIT_EQUAL(10, (ts::static_power10<int, 1>::value));
    TSUNIT_EQUAL(100, (ts::static_power10<uint8_t, 2>::value));
    TSUNIT_EQUAL(100, (ts::static_power10<int, 2>::value));
    TSUNIT_EQUAL(1000000, (ts::static_power10<uint32_t, 6>::value));
    TSUNIT_EQUAL(1000000, (ts::static_power10<uint64_t, 6>::value));
    TSUNIT_EQUAL(1000000000000000, (ts::static_power10<uint64_t, 15>::value));
}

TSUNIT_DEFINE_TEST(BoundCheck)
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

TSUNIT_DEFINE_TEST(BoundedCast)
{
    TSUNIT_EQUAL(20, ts::bounded_cast<uint8_t>(int(20)));
    TSUNIT_EQUAL(0, ts::bounded_cast<uint8_t>(-20));
    TSUNIT_EQUAL(255, ts::bounded_cast<uint8_t>(2000));

    TSUNIT_EQUAL(-128, ts::bounded_cast<int8_t>(int16_t(-1000)));
    TSUNIT_EQUAL(-100, ts::bounded_cast<int8_t>(int16_t(-100)));
    TSUNIT_EQUAL(100, ts::bounded_cast<int8_t>(int16_t(100)));
    TSUNIT_EQUAL(127, ts::bounded_cast<int8_t>(int16_t(1000)));
}

TSUNIT_DEFINE_TEST(GCD)
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

TSUNIT_DEFINE_TEST(Overflow)
{
    int64_t a = 292021270;
    int64_t b = 31590000000;
    int64_t res = a * b;
    TSUNIT_ASSERT(res < 0);
    TSUNIT_ASSERT(ts::mul_overflow(a, b, res));
    TSUNIT_ASSERT(ts::mul_overflow(a, b));
}

TSUNIT_DEFINE_TEST(SmallerUnsigned)
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
