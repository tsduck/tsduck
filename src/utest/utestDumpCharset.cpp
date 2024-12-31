//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class DumpCharset.
//
//----------------------------------------------------------------------------

#include "tsDumpCharset.h"
#include "tsMemory.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class DumpCharsetTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Decode);
    TSUNIT_DECLARE_TEST(Encode);
    TSUNIT_DECLARE_TEST(CanEncode);
};

TSUNIT_REGISTER(DumpCharsetTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(CanEncode)
{
    const ts::Charset& cset(ts::DumpCharset::DUMP);

    TSUNIT_ASSERT(cset.canEncode(u""));
    TSUNIT_ASSERT(cset.canEncode(u" 012 345 "));
    TSUNIT_ASSERT(!cset.canEncode(u"012 345 6"));
    TSUNIT_ASSERT(!cset.canEncode(u"01 a"));
}

TSUNIT_DEFINE_TEST(Decode)
{
    static const uint8_t data[] = {0x00, 0x01, 0x02, 0x11, 0xEA, 0x07, 0x80, 0x34, 0xB2};

    const ts::Charset& cset(ts::DumpCharset::DUMP);

    TSUNIT_EQUAL(u"00 01 02 11 EA 07 80 34 B2", cset.decoded(data, 9));
    TSUNIT_EQUAL(u"02 11 EA", cset.decoded(data + 2, 3));
    TSUNIT_EQUAL(u"", cset.decoded(data, 0));
    TSUNIT_EQUAL(u"", cset.decoded(nullptr, 3));
}

TSUNIT_DEFINE_TEST(Encode)
{
    const ts::Charset& cset(ts::DumpCharset::DUMP);

    uint8_t buffer[20];
    uint8_t* data = buffer;
    size_t size = sizeof(buffer);
    TS_ZERO(buffer);

    TSUNIT_EQUAL(0, cset.encode(data, size, u""));
    TSUNIT_EQUAL(buffer, data);
    TSUNIT_EQUAL(20, size);

    data = buffer;
    size = sizeof(buffer);
    TS_ZERO(buffer);

    TSUNIT_EQUAL(14, cset.encode(data, size, u"  01 0211 e a zz 01"));
    TSUNIT_EQUAL(buffer + 4, data);
    TSUNIT_EQUAL(16, size);
    TSUNIT_EQUAL(0x01, buffer[0]);
    TSUNIT_EQUAL(0x02, buffer[1]);
    TSUNIT_EQUAL(0x11, buffer[2]);
    TSUNIT_EQUAL(0xEA, buffer[3]);
}
