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
//  TSUnit test suite for tsRotate.h
//
//----------------------------------------------------------------------------

#include "tsRotate.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class RotateTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testROL();
    void testROLc();
    void testROR();
    void testRORc();
    void testROL64();
    void testROL64c();
    void testROR64();
    void testROR64c();

    TSUNIT_TEST_BEGIN(RotateTest);
    TSUNIT_TEST(testROL);
    TSUNIT_TEST(testROLc);
    TSUNIT_TEST(testROR);
    TSUNIT_TEST(testRORc);
    TSUNIT_TEST(testROL64);
    TSUNIT_TEST(testROL64c);
    TSUNIT_TEST(testROR64);
    TSUNIT_TEST(testROR64c);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(RotateTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void RotateTest::beforeTest()
{
}

// Test suite cleanup method.
void RotateTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void RotateTest::testROL()
{
    TSUNIT_EQUAL(0x34567812, ts::ROL(0x12345678, 8));
    TSUNIT_EQUAL(0x23456781, ts::ROL(0x12345678, 4));
    TSUNIT_EQUAL(0x67812345, ts::ROL(0x12345678, -12));
    TSUNIT_EQUAL(0x468ACF02, ts::ROL(0x12345678, 5));
    TSUNIT_EQUAL(0x048D159E, ts::ROL(0x12345678, 30));
    TSUNIT_EQUAL(0x2468ACF0, ts::ROL(0x12345678, 1));
}

void RotateTest::testROLc()
{
    TSUNIT_EQUAL(0x34567812, ts::ROLc(0x12345678, 8));
    TSUNIT_EQUAL(0x23456781, ts::ROLc(0x12345678, 4));
    TSUNIT_EQUAL(0x45678123, ts::ROLc(0x12345678, 12));
    TSUNIT_EQUAL(0x468ACF02, ts::ROLc(0x12345678, 5));
    TSUNIT_EQUAL(0x048D159E, ts::ROLc(0x12345678, 30));
}

void RotateTest::testROR()
{
    TSUNIT_EQUAL(0x78123456, ts::ROR(0x12345678, 8));
    TSUNIT_EQUAL(0x81234567, ts::ROR(0x12345678, 4));
    TSUNIT_EQUAL(0x45678123, ts::ROR(0x12345678, -12));
}

void RotateTest::testRORc()
{
    TSUNIT_EQUAL(0x78123456, ts::RORc(0x12345678, 8));
    TSUNIT_EQUAL(0x81234567, ts::RORc(0x12345678, 4));
    TSUNIT_EQUAL(0x45678123, ts::RORc(0x12345678, 20));
}

void RotateTest::testROL64()
{
    TSUNIT_EQUAL(TS_UCONST64(0x23456789ABCDEF01), ts::ROL64(TS_UCONST64(0x0123456789ABCDEF), 8));
    TSUNIT_EQUAL(TS_UCONST64(0xBCDEF0123456789A), ts::ROL64(TS_UCONST64(0x0123456789ABCDEF), 44));
    TSUNIT_EQUAL(TS_UCONST64(0xDEF0123456789ABC), ts::ROL64(TS_UCONST64(0x0123456789ABCDEF), -12));
}

void RotateTest::testROL64c()
{
    TSUNIT_EQUAL(TS_UCONST64(0x23456789ABCDEF01), ts::ROL64c(TS_UCONST64(0x0123456789ABCDEF), 8));
    TSUNIT_EQUAL(TS_UCONST64(0xBCDEF0123456789A), ts::ROL64c(TS_UCONST64(0x0123456789ABCDEF), 44));
    TSUNIT_EQUAL(TS_UCONST64(0x3456789ABCDEF012), ts::ROL64c(TS_UCONST64(0x0123456789ABCDEF), 12));
}

void RotateTest::testROR64()
{
    TSUNIT_EQUAL(TS_UCONST64(0xEF0123456789ABCD), ts::ROR64(TS_UCONST64(0x0123456789ABCDEF), 8));
    TSUNIT_EQUAL(TS_UCONST64(0x56789ABCDEF01234), ts::ROR64(TS_UCONST64(0x0123456789ABCDEF), 44));
    TSUNIT_EQUAL(TS_UCONST64(0x3456789ABCDEF012), ts::ROR64(TS_UCONST64(0x0123456789ABCDEF), -12));
}

void RotateTest::testROR64c()
{
    TSUNIT_EQUAL(TS_UCONST64(0xEF0123456789ABCD), ts::ROR64c(TS_UCONST64(0x0123456789ABCDEF), 8));
    TSUNIT_EQUAL(TS_UCONST64(0x56789ABCDEF01234), ts::ROR64c(TS_UCONST64(0x0123456789ABCDEF), 44));
    TSUNIT_EQUAL(TS_UCONST64(0xDEF0123456789ABC), ts::ROR64c(TS_UCONST64(0x0123456789ABCDEF), 12));
}
