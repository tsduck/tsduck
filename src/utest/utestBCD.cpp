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
//  TSUnit test suite for Binary Code Decimal (BCD) utilities.
//
//----------------------------------------------------------------------------

#include "tsBCD.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class BCDTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testIsValid();
    void testEncodeByte();
    void testDecodeByte();
    void testEncodeString();
    void testDecodeString();
    void testToString();

    TSUNIT_TEST_BEGIN(BCDTest);
    TSUNIT_TEST(testIsValid);
    TSUNIT_TEST(testEncodeByte);
    TSUNIT_TEST(testDecodeByte);
    TSUNIT_TEST(testEncodeString);
    TSUNIT_TEST(testDecodeString);
    TSUNIT_TEST(testToString);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(BCDTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void BCDTest::beforeTest()
{
}

// Test suite cleanup method.
void BCDTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void BCDTest::testIsValid()
{
    TSUNIT_ASSERT(ts::IsValidBCD(0x00));
    TSUNIT_ASSERT(ts::IsValidBCD(0x99));
    TSUNIT_ASSERT(ts::IsValidBCD(0x12));
    TSUNIT_ASSERT(ts::IsValidBCD(0x90));
    TSUNIT_ASSERT(ts::IsValidBCD(0x09));

    TSUNIT_ASSERT(!ts::IsValidBCD(0xA0));
    TSUNIT_ASSERT(!ts::IsValidBCD(0x0A));
    TSUNIT_ASSERT(!ts::IsValidBCD(0x9A));
    TSUNIT_ASSERT(!ts::IsValidBCD(0xFF));
    TSUNIT_ASSERT(!ts::IsValidBCD(0x0E));
    TSUNIT_ASSERT(!ts::IsValidBCD(0x0E));
}

void BCDTest::testEncodeByte()
{
    TSUNIT_EQUAL(0x00, ts::EncodeBCD(0));
    TSUNIT_EQUAL(0x10, ts::EncodeBCD(10));
    TSUNIT_EQUAL(0x09, ts::EncodeBCD(9));
    TSUNIT_EQUAL(0x99, ts::EncodeBCD(99));
    TSUNIT_EQUAL(0x47, ts::EncodeBCD(47));
}

void BCDTest::testDecodeByte()
{
    TSUNIT_EQUAL(0,  ts::DecodeBCD(0x00));
    TSUNIT_EQUAL(99, ts::DecodeBCD(0x99));
    TSUNIT_EQUAL(9,  ts::DecodeBCD(0x09));
    TSUNIT_EQUAL(90, ts::DecodeBCD(0x90));
    TSUNIT_EQUAL(21, ts::DecodeBCD(0x21));
}

void BCDTest::testEncodeString()
{
    uint8_t buf[16];

#define TEST(expr,...) \
    static const uint8_t TS_UNIQUE_NAME(REF)[]={ __VA_ARGS__}; expr; TSUNIT_EQUAL(0, ::memcmp(TS_UNIQUE_NAME(REF),buf,sizeof(TS_UNIQUE_NAME(REF))))

    TEST(ts::EncodeBCD(buf, 4, 1234), 0x12, 0x34);
    TEST(ts::EncodeBCD(buf, 5, 1234), 0x01, 0x23, 0x40);
    TEST(ts::EncodeBCD(buf, 5, 1234, false), 0x00, 0x12, 0x34);
    TEST(ts::EncodeBCD(buf, 5, 1234, true, 10), 0x01, 0x23, 0x4A);
    TEST(ts::EncodeBCD(buf, 5, 1234, false, 10), 0xA0, 0x12, 0x34);

    TEST(ts::EncodeBCD(buf, 3, 1234), 0x23, 0x40);
    TEST(ts::EncodeBCD(buf, 3, 1234, false), 0x02, 0x34);
    TEST(ts::EncodeBCD(buf, 3, 1234, true, 7), 0x23, 0x47);
    TEST(ts::EncodeBCD(buf, 3, 1234, false, 7), 0x72, 0x34);

#undef TEST
}

void BCDTest::testDecodeString()
{
#define TEST(value, bcd_count, ljustified,...) \
    static const uint8_t TS_UNIQUE_NAME(REF)[]={ __VA_ARGS__}; TSUNIT_EQUAL((value), ts::DecodeBCD(TS_UNIQUE_NAME(REF), (bcd_count), (ljustified)))

    TEST(1234, 4, true,  0x12, 0x34);
    TEST(123,  3, true,  0x12, 0x34);
    TEST(234,  3, false, 0x12, 0x34);

#undef TEST
}

void BCDTest::testToString()
{
    std::string buf;

#define TEST(value, bcd_count, decimal, ljustified,...) \
    static const uint8_t TS_UNIQUE_NAME(REF)[]={ __VA_ARGS__}; ts::BCDToString(buf, TS_UNIQUE_NAME(REF), (bcd_count), (decimal), (ljustified)); TSUNIT_EQUAL((value), buf)

    TEST("123",   3, -1, true,  0x12, 0x34);
    TEST("0.123", 3,  0, true,  0x12, 0x34);
    TEST("23.4",  3,  2, false, 0x12, 0x34);

#undef TEST
}
