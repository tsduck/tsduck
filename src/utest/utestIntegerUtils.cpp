//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
#include "tsVersion.h"
#include "tsVersionInfo.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class IntegerUtilsTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testBoundedAdd();
    void testBoundedSub();
    void testRoundDown();
    void testRoundUp();
    void testSignExtend();

    TSUNIT_TEST_BEGIN(IntegerUtilsTest);
    TSUNIT_TEST(testBoundedAdd);
    TSUNIT_TEST(testBoundedSub);
    TSUNIT_TEST(testRoundDown);
    TSUNIT_TEST(testRoundUp);
    TSUNIT_TEST(testSignExtend);
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

void IntegerUtilsTest::testBoundedAdd()
{
    TSUNIT_EQUAL(201, ts::BoundedAdd(uint8_t(1),   uint8_t(200)));
    TSUNIT_EQUAL(255, ts::BoundedAdd(uint8_t(0),   uint8_t(255)));
    TSUNIT_EQUAL(255, ts::BoundedAdd(uint8_t(1),   uint8_t(255)));
    TSUNIT_EQUAL(255, ts::BoundedAdd(uint8_t(100), uint8_t(200)));

    TSUNIT_EQUAL(120,  ts::BoundedAdd(int8_t(10),   int8_t(110)));
    TSUNIT_EQUAL(127,  ts::BoundedAdd(int8_t(100),  int8_t(80)));
    TSUNIT_EQUAL(80,   ts::BoundedAdd(int8_t(100),  int8_t(-20)));
    TSUNIT_EQUAL(-120, ts::BoundedAdd(int8_t(-100), int8_t(-20)));
    TSUNIT_EQUAL(-128, ts::BoundedAdd(int8_t(-100), int8_t(-60)));
}

void IntegerUtilsTest::testBoundedSub()
{
    TSUNIT_EQUAL(80,  ts::BoundedSub(uint8_t(100), uint8_t(20)));
    TSUNIT_EQUAL(0,   ts::BoundedSub(uint8_t(100), uint8_t(200)));
    TSUNIT_EQUAL(10,  ts::BoundedSub(uint8_t(10),  uint8_t(0)));
    TSUNIT_EQUAL(0,   ts::BoundedSub(uint8_t(0),   uint8_t(10)));
    TSUNIT_EQUAL(255, ts::BoundedSub(uint8_t(255), uint8_t(0)));

    TSUNIT_EQUAL(10,   ts::BoundedSub(int8_t(20),   int8_t(10)));
    TSUNIT_EQUAL(-10,  ts::BoundedSub(int8_t(20),   int8_t(30)));
    TSUNIT_EQUAL(-50,  ts::BoundedSub(int8_t(-20),  int8_t(30)));
    TSUNIT_EQUAL(127,  ts::BoundedSub(int8_t(100),  int8_t(-50)));
    TSUNIT_EQUAL(-128, ts::BoundedSub(int8_t(-100), int8_t(40)));
}

void IntegerUtilsTest::testRoundDown()
{
    TSUNIT_EQUAL(-1, -11 % 5);
    TSUNIT_EQUAL(-4, -14 % 5);
    TSUNIT_EQUAL(0,  -15 % 5);

    TSUNIT_EQUAL(20, ts::RoundDown(20, 5));
    TSUNIT_EQUAL(20, ts::RoundDown(24, 5));

    TSUNIT_EQUAL(-20, ts::RoundDown(-20, 5));
    TSUNIT_EQUAL(-25, ts::RoundDown(-21, 5));
    TSUNIT_EQUAL(-25, ts::RoundDown(-24, -5));
    TSUNIT_EQUAL(-25, ts::RoundDown(-25, 5));

    TSUNIT_EQUAL(10, ts::RoundDown(uint32_t(10), uint32_t(5)));
    TSUNIT_EQUAL(10, ts::RoundDown(uint32_t(14), uint32_t(5)));

    TSUNIT_EQUAL(10, ts::RoundDown(int8_t(10), int8_t(5)));
    TSUNIT_EQUAL(10, ts::RoundDown(int8_t(14), int8_t(5)));

    TSUNIT_EQUAL(-10, ts::RoundDown(int8_t(-10), int8_t(5)));
    TSUNIT_EQUAL(-15, ts::RoundDown(int8_t(-11), int8_t(5)));
    TSUNIT_EQUAL(-15, ts::RoundDown(int8_t(-14), int8_t(5)));
    TSUNIT_EQUAL(-15, ts::RoundDown(int8_t(-15), int8_t(5)));

    TSUNIT_EQUAL(10, ts::RoundDown(10, 0));
    TSUNIT_EQUAL(10, ts::RoundDown(10, 1));
    TSUNIT_EQUAL(-10, ts::RoundDown(-10, 0));
    TSUNIT_EQUAL(-10, ts::RoundDown(-10, 1));

    TSUNIT_EQUAL(0, ts::RoundDown(0, 0));
    TSUNIT_EQUAL(0, ts::RoundDown(0, 1));
    TSUNIT_EQUAL(0, ts::RoundDown(0, -27));
}

void IntegerUtilsTest::testRoundUp()
{
    TSUNIT_EQUAL(20, ts::RoundUp(20, 5));
    TSUNIT_EQUAL(25, ts::RoundUp(21, 5));
    TSUNIT_EQUAL(25, ts::RoundUp(24, -5));

    TSUNIT_EQUAL(-20, ts::RoundUp(-20, 5));
    TSUNIT_EQUAL(-20, ts::RoundUp(-21, 5));
    TSUNIT_EQUAL(-20, ts::RoundUp(-24, 5));
    TSUNIT_EQUAL(-25, ts::RoundUp(-25, 5));

    TSUNIT_EQUAL(10, ts::RoundUp(uint32_t(10), uint32_t(5)));
    TSUNIT_EQUAL(15, ts::RoundUp(uint32_t(11), uint32_t(5)));
    TSUNIT_EQUAL(15, ts::RoundUp(uint32_t(14), uint32_t(5)));

    TSUNIT_EQUAL(10, ts::RoundUp(int8_t(10), int8_t(5)));
    TSUNIT_EQUAL(15, ts::RoundUp(int8_t(11), int8_t(5)));
    TSUNIT_EQUAL(15, ts::RoundUp(int8_t(14), int8_t(5)));

    TSUNIT_EQUAL(-10, ts::RoundUp(int8_t(-10), int8_t(5)));
    TSUNIT_EQUAL(-10, ts::RoundUp(int8_t(-11), int8_t(5)));
    TSUNIT_EQUAL(-10, ts::RoundUp(int8_t(-14), int8_t(5)));
    TSUNIT_EQUAL(-15, ts::RoundUp(int8_t(-15), int8_t(5)));

    TSUNIT_EQUAL(10, ts::RoundUp(10, 0));
    TSUNIT_EQUAL(10, ts::RoundUp(10, 1));
    TSUNIT_EQUAL(-10, ts::RoundUp(-10, 0));
    TSUNIT_EQUAL(-10, ts::RoundUp(-10, 1));

    TSUNIT_EQUAL(0, ts::RoundUp(0, 0));
    TSUNIT_EQUAL(0, ts::RoundUp(0, 1));
    TSUNIT_EQUAL(0, ts::RoundUp(0, -27));
}

void IntegerUtilsTest::testSignExtend()
{
    TSUNIT_EQUAL(25, ts::SignExtend(int32_t(25), 12));
    TSUNIT_EQUAL(0x07FF, ts::SignExtend(int16_t(0x07FF), 12));
    TSUNIT_EQUAL(-1, ts::SignExtend(int16_t(0x0FFF), 12));
    TSUNIT_EQUAL(-2047, ts::SignExtend(int16_t(0x0801), 12));
    TSUNIT_EQUAL(-2048, ts::SignExtend(int16_t(0x2800), 12));
}
