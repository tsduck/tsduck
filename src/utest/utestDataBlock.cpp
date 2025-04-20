//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for the DataBlock class.
//
//----------------------------------------------------------------------------

#include "tsDataBlock.h"
#include "tsunit.h"

//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class DataBlockTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(LengthField);
    TSUNIT_DECLARE_TEST(Constructors);
};

TSUNIT_REGISTER(DataBlockTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(LengthField)
{
    static const uint8_t data[] = {0x12, 0xF3, 0x02};
    TSUNIT_EQUAL(ts::NPOS, (ts::DataBlock<>::GetLengthField(data, sizeof(data))));
    TSUNIT_EQUAL(0x0012, (ts::DataBlock<0, 8>::GetLengthField(data, sizeof(data))));
    TSUNIT_EQUAL(0xF302, (ts::DataBlock<8, 16>::GetLengthField(data, sizeof(data))));
    TSUNIT_EQUAL(0x0302, (ts::DataBlock<12, 12>::GetLengthField(data, sizeof(data))));
    TSUNIT_EQUAL(0x0F30, (ts::DataBlock<8, 12>::GetLengthField(data, sizeof(data))));
    TSUNIT_EQUAL(0x0F30, (ts::DataBlock<8, 12, false>::GetLengthField(data, sizeof(data))));
    TSUNIT_EQUAL(0x0F30, (ts::DataBlock<8, 12, true>::GetLengthField(data, sizeof(data))));
    TSUNIT_EQUAL(0x0000, (ts::DataBlock<16, 4>::GetLengthField(data, sizeof(data))));
    TSUNIT_EQUAL(0x17981, (ts::DataBlock<5, 18>::GetLengthField(data, sizeof(data))));
    TSUNIT_EQUAL(ts::NPOS, (ts::DataBlock<16, 4, true>::GetLengthField(data, sizeof(data))));
}

TSUNIT_DEFINE_TEST(Constructors)
{
    using Section = ts::DataBlock<12, 12>;

    static const ts::ByteBlock data1 {0x12, 0xF0, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    Section d1(data1);
    TSUNIT_ASSERT(d1.isValid());
    TSUNIT_EQUAL(9, d1.size());
    TSUNIT_EQUAL(9, d1.rawDataSize());

    Section d2(data1.data(), data1.size() - 1);
    TSUNIT_ASSERT(!d2.isValid());
    TSUNIT_EQUAL(0, d2.size());
    TSUNIT_EQUAL(0, d2.rawDataSize());
}
