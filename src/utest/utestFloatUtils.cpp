//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for tsFloatUtils.h
//
//----------------------------------------------------------------------------

#include "tsFloatUtils.h"
#include "tsunit.h"

// Many floating-point literal (implicitly double) are used as ieee_float32_t
TS_LLVM_NOWARNING(implicit-float-conversion)
TS_MSC_NOWARNING(4056) // overflow in floating-point constant arithmetic
TS_MSC_NOWARNING(4305) // truncation from 'double' to 'ts::ieee_float32_t'


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class FloatUtilsTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(EqualFloat);
    TSUNIT_DECLARE_TEST(Float32);
    TSUNIT_DECLARE_TEST(Float64);
    TSUNIT_DECLARE_TEST(GetFloat32LE);
    TSUNIT_DECLARE_TEST(GetFloat32BE);
    TSUNIT_DECLARE_TEST(GetFloat64LE);
    TSUNIT_DECLARE_TEST(GetFloat64BE);
    TSUNIT_DECLARE_TEST(PutFloat32LE);
    TSUNIT_DECLARE_TEST(PutFloat32BE);
    TSUNIT_DECLARE_TEST(PutFloat64LE);
    TSUNIT_DECLARE_TEST(PutFloat64BE);
};

TSUNIT_REGISTER(FloatUtilsTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(EqualFloat)
{
    double d1 = 1.0;
    double d2 = d1 + std::numeric_limits<double>::epsilon();
    double d3 = 1.00001;

    TSUNIT_ASSERT(ts::equal_float(d1, 1.0));
    TSUNIT_ASSERT(ts::equal_float(d1, d1));
    TSUNIT_ASSERT(ts::equal_float(d1, d2));
    TSUNIT_ASSERT(!ts::equal_float(d1, d3));
}

TSUNIT_DEFINE_TEST(Float32)
{
    TSUNIT_ASSERT(std::is_floating_point<ts::ieee_float32_t>::value);
    TSUNIT_ASSERT(std::numeric_limits<ts::ieee_float32_t>::is_iec559);
    TSUNIT_EQUAL(4, sizeof(ts::ieee_float32_t));
    TSUNIT_EQUAL(23 + 1, std::numeric_limits<ts::ieee_float32_t>::digits);
    // 8-bit exponent
    TSUNIT_ASSERT(std::numeric_limits<ts::ieee_float32_t>::max_exponent - std::numeric_limits<ts::ieee_float32_t>::min_exponent >= 0x0080);
    TSUNIT_ASSERT(std::numeric_limits<ts::ieee_float32_t>::max_exponent - std::numeric_limits<ts::ieee_float32_t>::min_exponent < 0x0100);
}

TSUNIT_DEFINE_TEST(Float64)
{
    TSUNIT_ASSERT(std::is_floating_point<ts::ieee_float64_t>::value);
    TSUNIT_ASSERT(std::numeric_limits<ts::ieee_float64_t>::is_iec559);
    TSUNIT_EQUAL(8, sizeof(ts::ieee_float64_t));
    TSUNIT_EQUAL(52 + 1, std::numeric_limits<ts::ieee_float64_t>::digits);
    // 11-bit exponent
    TSUNIT_ASSERT(std::numeric_limits<ts::ieee_float64_t>::max_exponent - std::numeric_limits<ts::ieee_float64_t>::min_exponent >= 0x0400);
    TSUNIT_ASSERT(std::numeric_limits<ts::ieee_float64_t>::max_exponent - std::numeric_limits<ts::ieee_float64_t>::min_exponent < 0x0800);
}

// IEEE floats Maple test vectors of 32 and 64-bit types.
// ------------------------------------------------------------------------
// Machine epsilon (2^(-p + 1))
// Precision = 24 bits
//  32-bit : 34000000          1.1920929e-07
//  64-bit : 3E80000000000000  1.1920928955078125e-07
// Precision = 53 bits
//  32-bit : 25800000          2.2204460e-16
//  64-bit : 3CB0000000000000  2.2204460492503131e-16
// Precision = 64 bits
//  32-bit : 20000000          1.0842022e-19
//  64-bit : 3C00000000000000  1.0842021724855044e-19
// Precision = 113 bits
//  32-bit : 07800000          1.9259299e-34
//  64-bit : 38F0000000000000  1.9259299443872359e-34
// ------------------------------------------------------------------------
// Largest finite (1 - 2^(-p))*2^(maximum exponent + 1)
// Precision = 24 bits
// Maximum exponent = 127
//  32-bit : 7F7FFFFF          3.4028235e+38
//  64-bit : 47EFFFFFE0000000  3.4028234663852886e+38
// Precision = 53 bits
// Maximum exponent = 1023
//  32-bit : 7F800000          1.7976931e+308 (+Infinity)
//  64-bit : 7FEFFFFFFFFFFFFF  1.7976931348623157e+308
// Precision = 64 bits
// Maximum exponent = 16383
//  32-bit : 7F800000          1.1897315e+4932 (+Infinity)
//  64-bit : 7FF0000000000000  1.1897314953572318e+4932 (+Infinity)
// Precision = 113 bits
// Maximum exponent = 16383
//  32-bit : 7F800000          1.1897315e+4932 (+Infinity)
//  64-bit : 7FF0000000000000  1.1897314953572318e+4932 (+Infinity)
// ------------------------------------------------------------------------
// Smallest normalized finite (2^(minimum exponent))
// Minimum exponent = -126
//  32-bit : 00800000           1.1754944e-38
//  64-bit : 3810000000000000   1.1754943508222875e-38
// Minimum exponent = -1022
//  32-bit : +00000000          2.2250739e-308 (UNDERFLOW)
//  64-bit : 0010000000000000   2.2250738585072014e-308
// Minimum exponent = -16382
//  32-bit : +00000000          3.3621031e-4932 (UNDERFLOW)
//  64-bit : +0000000000000000  3.3621031431120935e-4932 (UNDERFLOW)
// Minimum exponent = -16382
//  32-bit : +00000000          3.3621031e-4932 (UNDERFLOW)
//  64-bit : +0000000000000000  3.3621031431120935e-4932 (UNDERFLOW)
// ------------------------------------------------------------------------
// Smallest denormalized finite (2^(minimum exponent - stored fraction bits))
// Minimum exponent - stored fraction bits = -149
//  32-bit : +00000000          1.4012985e-45 (UNDERFLOW)
//  64-bit : 36A0000000000000   1.4012984643248171e-45
// Minimum exponent - stored fraction bits = -1074
//  32-bit : +00000000          4.9406565e-324 (UNDERFLOW)
//  64-bit : +0000000000000000  4.9406564584124654e-324 (UNDERFLOW)
// Minimum exponent - stored fraction bits = -16446
//  32-bit : +00000000          1.8225998e-4951 (UNDERFLOW)
//  64-bit : +0000000000000000  1.8225997659412373e-4951 (UNDERFLOW)
// Minimum exponent - stored fraction bits = -16494
//  32-bit : +00000000          6.4751751e-4966 (UNDERFLOW)
//  64-bit : +0000000000000000  6.4751751194380251e-4966 (UNDERFLOW)
// ------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(GetFloat32LE)
{
    static const uint8_t bin1[] = {0x00, 0x00, 0x00, 0x34};
    TSUNIT_EQUAL(1.1920929e-07, ts::GetFloat32LE(bin1));

    static const uint8_t bin2[] = {0x00, 0x00, 0x80, 0x25};
    TSUNIT_EQUAL(2.2204460e-16, ts::GetFloat32LE(bin2));

    static const uint8_t bin3[] = {0x00, 0x00, 0x00, 0x20};
    TSUNIT_EQUAL(1.0842022e-19, ts::GetFloat32LE(bin3));

    static const uint8_t bin4[] = {0x00, 0x00, 0x80, 0x07};
    TSUNIT_EQUAL(1.9259299e-34, ts::GetFloat32LE(bin4));

    static const uint8_t bin5[] = {0xFF, 0xFF, 0x7F, 0x7F};
    TSUNIT_EQUAL(3.4028235e+38, ts::GetFloat32LE(bin5));
}

TSUNIT_DEFINE_TEST(GetFloat32BE)
{
    static const uint8_t bin1[] = {0x34, 0x00, 0x00, 0x00};
    TSUNIT_EQUAL(1.1920929e-07, ts::GetFloat32BE(bin1));

    static const uint8_t bin2[] = {0x25, 0x80, 0x00, 0x00};
    TSUNIT_EQUAL(2.2204460e-16, ts::GetFloat32BE(bin2));

    static const uint8_t bin3[] = {0x20, 0x00, 0x00, 0x00};
    TSUNIT_EQUAL(1.0842022e-19, ts::GetFloat32BE(bin3));

    static const uint8_t bin4[] = {0x07, 0x80, 0x00, 0x00};
    TSUNIT_EQUAL(1.9259299e-34, ts::GetFloat32BE(bin4));

    static const uint8_t bin5[] = {0x7F, 0x7F, 0xFF, 0xFF};
    TSUNIT_EQUAL(3.4028235e+38, ts::GetFloat32BE(bin5));
}

TSUNIT_DEFINE_TEST(GetFloat64LE)
{
    static const uint8_t bin1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3E};
    TSUNIT_EQUAL(1.1920928955078125e-07, ts::GetFloat64LE(bin1));

    static const uint8_t bin2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x3C};
    TSUNIT_EQUAL(2.2204460492503131e-16, ts::GetFloat64LE(bin2));

    static const uint8_t bin3[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C};
    TSUNIT_EQUAL(1.0842021724855044e-19, ts::GetFloat64LE(bin3));

    static const uint8_t bin4[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x38};
    TSUNIT_EQUAL(1.9259299443872359e-34, ts::GetFloat64LE(bin4));

    static const uint8_t bin5[] = {0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0xEF, 0x47};
    TSUNIT_EQUAL(3.4028234663852886e+38, ts::GetFloat64LE(bin5));

    static const uint8_t bin8[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38};
    TSUNIT_EQUAL(1.1754943508222875e-38, ts::GetFloat64LE(bin8));

    static const uint8_t bin9[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00};
    TSUNIT_EQUAL(2.2250738585072014e-308, ts::GetFloat64LE(bin9));
}

TSUNIT_DEFINE_TEST(GetFloat64BE)
{
    static const uint8_t bin1[] = {0x3E, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TSUNIT_EQUAL(1.1920928955078125e-07, ts::GetFloat64BE(bin1));

    static const uint8_t bin2[] = {0x3C, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TSUNIT_EQUAL(2.2204460492503131e-16, ts::GetFloat64BE(bin2));

    static const uint8_t bin3[] = {0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TSUNIT_EQUAL(1.0842021724855044e-19, ts::GetFloat64BE(bin3));

    static const uint8_t bin4[] = {0x38, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TSUNIT_EQUAL(1.9259299443872359e-34, ts::GetFloat64BE(bin4));

    static const uint8_t bin5[] = {0x47, 0xEF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00};
    TSUNIT_EQUAL(3.4028234663852886e+38, ts::GetFloat64BE(bin5));

    // static const uint8_t bin6[] = {0x7F, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    // TSUNIT_EQUAL(1.7976931348623157e+308, ts::GetFloat64BE(bin6));

    static const uint8_t bin8[] = {0x38, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TSUNIT_EQUAL(1.1754943508222875e-38, ts::GetFloat64BE(bin8));

    static const uint8_t bin9[] = {0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TSUNIT_EQUAL(2.2250738585072014e-308, ts::GetFloat64BE(bin9));

    // static const uint8_t bin10[] = {0x3, 0x6A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x000};
    // TSUNIT_EQUAL(1.4012984643248171e-45, ts::GetFloat64BE(bin10));
}

TSUNIT_DEFINE_TEST(PutFloat32LE)
{
    tsunit::Bytes buf;

    buf.assign(4, 0xAC);
    ts::PutFloat32LE(&buf[0], 1.1920929e-07);
    TSUNIT_EQUAL((tsunit::Bytes{00, 0x00, 0x00, 0x34}), buf);

    buf.assign(4, 0xAC);
    ts::PutFloat32LE(&buf[0], 2.2204460e-16);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x80, 0x25}), buf);

    buf.assign(4, 0xAC);
    ts::PutFloat32LE(&buf[0], 1.0842022e-19);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x00, 0x20}), buf);

    buf.assign(4, 0xAC);
    ts::PutFloat32LE(&buf[0], 1.9259299e-34);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x80, 0x07}), buf);

    buf.assign(4, 0xAC);
    ts::PutFloat32LE(&buf[0], 3.4028235e+38);
    TSUNIT_EQUAL((tsunit::Bytes{0xFF, 0xFF, 0x7F, 0x7F}), buf);
}

TSUNIT_DEFINE_TEST(PutFloat32BE)
{
    tsunit::Bytes buf;

    buf.assign(4, 0xAC);
    ts::PutFloat32BE(&buf[0], 1.1920929e-07);
    TSUNIT_EQUAL((tsunit::Bytes{0x34, 0x00, 0x00, 0x00}), buf);

    buf.assign(4, 0xAC);
    ts::PutFloat32BE(&buf[0], 2.2204460e-16);
    TSUNIT_EQUAL((tsunit::Bytes{0x25, 0x80, 0x00, 0x00}), buf);

    buf.assign(4, 0xAC);
    ts::PutFloat32BE(&buf[0], 1.0842022e-19);
    TSUNIT_EQUAL((tsunit::Bytes{0x20, 0x00, 0x00, 0x00}), buf);

    buf.assign(4, 0xAC);
    ts::PutFloat32BE(&buf[0], 1.9259299e-34);
    TSUNIT_EQUAL((tsunit::Bytes{0x07, 0x80, 0x00, 0x00}), buf);

    buf.assign(4, 0xAC);
    ts::PutFloat32BE(&buf[0], 3.4028235e+38);
    TSUNIT_EQUAL((tsunit::Bytes{0x7F, 0x7F, 0xFF, 0xFF}), buf);
}

TSUNIT_DEFINE_TEST(PutFloat64LE)
{
    tsunit::Bytes buf;

    buf.assign(8, 0xAC);
    ts::PutFloat64LE(&buf[0], 1.1920928955078125e-07);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3E}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64LE(&buf[0], 2.2204460492503131e-16);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x3C}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64LE(&buf[0], 1.0842021724855044e-19);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64LE(&buf[0], 1.9259299443872359e-34);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x38}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64LE(&buf[0], 3.4028234663852886e+38);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0xEF, 0x47}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64LE(&buf[0], 1.1754943508222875e-38);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64LE(&buf[0], 2.2250738585072014e-308);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00}), buf);
}

TSUNIT_DEFINE_TEST(PutFloat64BE)
{
    tsunit::Bytes buf;

    buf.assign(8, 0xAC);
    ts::PutFloat64BE(&buf[0], 1.1920928955078125e-07);
    TSUNIT_EQUAL((tsunit::Bytes{0x3E, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64BE(&buf[0], 2.2204460492503131e-16);
    TSUNIT_EQUAL((tsunit::Bytes{0x3C, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64BE(&buf[0], 1.0842021724855044e-19);
    TSUNIT_EQUAL((tsunit::Bytes{0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64BE(&buf[0], 1.9259299443872359e-34);
    TSUNIT_EQUAL((tsunit::Bytes{0x38, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64BE(&buf[0], 3.4028234663852886e+38);
    TSUNIT_EQUAL((tsunit::Bytes{0x47, 0xEF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64BE(&buf[0], 1.1754943508222875e-38);
    TSUNIT_EQUAL((tsunit::Bytes{0x38, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), buf);

    buf.assign(8, 0xAC);
    ts::PutFloat64BE(&buf[0], 2.2250738585072014e-308);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), buf);
}
