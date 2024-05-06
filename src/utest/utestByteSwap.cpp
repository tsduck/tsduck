//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for ByteSwap.h
//
//----------------------------------------------------------------------------

#include "tsByteSwap.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ByteSwapTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(SignExtend24);
    TSUNIT_DECLARE_TEST(SignExtend40);
    TSUNIT_DECLARE_TEST(SignExtend48);
    TSUNIT_DECLARE_TEST(ByteSwap16);
    TSUNIT_DECLARE_TEST(ByteSwap24);
    TSUNIT_DECLARE_TEST(ByteSwap32);
    TSUNIT_DECLARE_TEST(ByteSwap64);
    TSUNIT_DECLARE_TEST(CondByteSwap16BE);
    TSUNIT_DECLARE_TEST(CondByteSwap16LE);
    TSUNIT_DECLARE_TEST(CondByteSwap32BE);
    TSUNIT_DECLARE_TEST(CondByteSwap32LE);
    TSUNIT_DECLARE_TEST(CondByteSwap64BE);
    TSUNIT_DECLARE_TEST(CondByteSwap64LE);
};

TSUNIT_REGISTER(ByteSwapTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(SignExtend24)
{
    TSUNIT_EQUAL(0x00723456, ts::SignExtend24(0xAA723456));
    TSUNIT_EQUAL(0xFF923456, ts::SignExtend24(0xAA923456));
}

TSUNIT_DEFINE_TEST(SignExtend40)
{
    TSUNIT_EQUAL(0x000000723456789A, ts::SignExtend40(0xAAAAAA723456789A));
    TSUNIT_EQUAL(0xFFFFFFA23456789A, ts::SignExtend40(0xAAAAAAA23456789A));
}

TSUNIT_DEFINE_TEST(SignExtend48)
{
    TSUNIT_EQUAL(0x0000723456789ABC, ts::SignExtend48(0xAAAA723456789ABC));
    TSUNIT_EQUAL(0xFFFFA23456789ABC, ts::SignExtend48(0xAAAAA23456789ABC));
}

TSUNIT_DEFINE_TEST(ByteSwap16)
{
    TSUNIT_EQUAL(0x3412, ts::ByteSwap16(0x1234));
}

TSUNIT_DEFINE_TEST(ByteSwap24)
{
    TSUNIT_EQUAL(0x563412, ts::ByteSwap24(0x123456));
    TSUNIT_EQUAL(0xEFCDAB, ts::ByteSwap24(0xABCDEF));
}

TSUNIT_DEFINE_TEST(ByteSwap32)
{
    TSUNIT_EQUAL(0x78563412, ts::ByteSwap32(0x12345678));
}

TSUNIT_DEFINE_TEST(ByteSwap64)
{
    TSUNIT_EQUAL(0xEFCDAB8967452301, ts::ByteSwap64(0x0123456789ABCDEF));
}

TSUNIT_DEFINE_TEST(CondByteSwap16BE)
{
#if defined(TS_LITTLE_ENDIAN)
    TSUNIT_EQUAL(0x3412, ts::CondByteSwap16BE(0x1234));
#else
    TSUNIT_EQUAL(0x1234, ts::CondByteSwap16BE(0x1234));
#endif
}

TSUNIT_DEFINE_TEST(CondByteSwap16LE)
{
#if defined(TS_LITTLE_ENDIAN)
    TSUNIT_EQUAL(0x1234, ts::CondByteSwap16LE(0x1234));
#else
    TSUNIT_EQUAL(0x3412, ts::CondByteSwap16LE(0x1234));
#endif
}

TSUNIT_DEFINE_TEST(CondByteSwap32BE)
{
#if defined(TS_LITTLE_ENDIAN)
    TSUNIT_EQUAL(0x78563412, ts::CondByteSwap32BE(0x12345678));
#else
    TSUNIT_EQUAL(0x12345678, ts::CondByteSwap32BE(0x12345678));
#endif
}

TSUNIT_DEFINE_TEST(CondByteSwap32LE)
{
#if defined(TS_LITTLE_ENDIAN)
    TSUNIT_EQUAL(0x12345678, ts::CondByteSwap32LE(0x12345678));
#else
    TSUNIT_EQUAL(0x78563412, ts::CondByteSwap32LE(0x12345678));
#endif
}

TSUNIT_DEFINE_TEST(CondByteSwap64BE)
{
#if defined(TS_LITTLE_ENDIAN)
    TSUNIT_EQUAL(0xEFCDAB8967452301, ts::CondByteSwap64BE(0x0123456789ABCDEF));
#else
    TSUNIT_EQUAL(0x0123456789ABCDEF, ts::CondByteSwap64BE(0x0123456789ABCDEF));
#endif
}

TSUNIT_DEFINE_TEST(CondByteSwap64LE)
{
#if defined(TS_LITTLE_ENDIAN)
    TSUNIT_EQUAL(0x0123456789ABCDEF, ts::CondByteSwap64LE(0x0123456789ABCDEF));
#else
    TSUNIT_EQUAL(0xEFCDAB8967452301, ts::CondByteSwap64LE(0x0123456789ABCDEF));
#endif
}
