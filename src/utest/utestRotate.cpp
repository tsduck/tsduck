//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(ROL);
    TSUNIT_DECLARE_TEST(ROLc);
    TSUNIT_DECLARE_TEST(ROR);
    TSUNIT_DECLARE_TEST(RORc);
    TSUNIT_DECLARE_TEST(ROL64);
    TSUNIT_DECLARE_TEST(ROL64c);
    TSUNIT_DECLARE_TEST(ROR64);
    TSUNIT_DECLARE_TEST(ROR64c);
};

TSUNIT_REGISTER(RotateTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(ROL)
{
    TSUNIT_EQUAL(0x34567812, ts::ROL(0x12345678, 8));
    TSUNIT_EQUAL(0x23456781, ts::ROL(0x12345678, 4));
    TSUNIT_EQUAL(0x67812345, ts::ROL(0x12345678, -12));
    TSUNIT_EQUAL(0x468ACF02, ts::ROL(0x12345678, 5));
    TSUNIT_EQUAL(0x048D159E, ts::ROL(0x12345678, 30));
    TSUNIT_EQUAL(0x2468ACF0, ts::ROL(0x12345678, 1));
}

TSUNIT_DEFINE_TEST(ROLc)
{
    TSUNIT_EQUAL(0x34567812, ts::ROLc(0x12345678, 8));
    TSUNIT_EQUAL(0x23456781, ts::ROLc(0x12345678, 4));
    TSUNIT_EQUAL(0x45678123, ts::ROLc(0x12345678, 12));
    TSUNIT_EQUAL(0x468ACF02, ts::ROLc(0x12345678, 5));
    TSUNIT_EQUAL(0x048D159E, ts::ROLc(0x12345678, 30));
}

TSUNIT_DEFINE_TEST(ROR)
{
    TSUNIT_EQUAL(0x78123456, ts::ROR(0x12345678, 8));
    TSUNIT_EQUAL(0x81234567, ts::ROR(0x12345678, 4));
    TSUNIT_EQUAL(0x45678123, ts::ROR(0x12345678, -12));
}

TSUNIT_DEFINE_TEST(RORc)
{
    TSUNIT_EQUAL(0x78123456, ts::RORc(0x12345678, 8));
    TSUNIT_EQUAL(0x81234567, ts::RORc(0x12345678, 4));
    TSUNIT_EQUAL(0x45678123, ts::RORc(0x12345678, 20));
}

TSUNIT_DEFINE_TEST(ROL64)
{
    TSUNIT_EQUAL(0x23456789ABCDEF01, ts::ROL64(0x0123456789ABCDEF, 8));
    TSUNIT_EQUAL(0xBCDEF0123456789A, ts::ROL64(0x0123456789ABCDEF, 44));
    TSUNIT_EQUAL(0xDEF0123456789ABC, ts::ROL64(0x0123456789ABCDEF, -12));
}

TSUNIT_DEFINE_TEST(ROL64c)
{
    TSUNIT_EQUAL(0x23456789ABCDEF01, ts::ROL64c(0x0123456789ABCDEF, 8));
    TSUNIT_EQUAL(0xBCDEF0123456789A, ts::ROL64c(0x0123456789ABCDEF, 44));
    TSUNIT_EQUAL(0x3456789ABCDEF012, ts::ROL64c(0x0123456789ABCDEF, 12));
}

TSUNIT_DEFINE_TEST(ROR64)
{
    TSUNIT_EQUAL(0xEF0123456789ABCD, ts::ROR64(0x0123456789ABCDEF, 8));
    TSUNIT_EQUAL(0x56789ABCDEF01234, ts::ROR64(0x0123456789ABCDEF, 44));
    TSUNIT_EQUAL(0x3456789ABCDEF012, ts::ROR64(0x0123456789ABCDEF, -12));
}

TSUNIT_DEFINE_TEST(ROR64c)
{
    TSUNIT_EQUAL(0xEF0123456789ABCD, ts::ROR64c(0x0123456789ABCDEF, 8));
    TSUNIT_EQUAL(0x56789ABCDEF01234, ts::ROR64c(0x0123456789ABCDEF, 44));
    TSUNIT_EQUAL(0xDEF0123456789ABC, ts::ROR64c(0x0123456789ABCDEF, 12));
}
