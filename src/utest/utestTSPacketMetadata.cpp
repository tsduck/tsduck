//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::TSPacketMetadata
//
//----------------------------------------------------------------------------

#include "tsTSPacketMetadata.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TSPacketMetadataTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testSize();

    TSUNIT_TEST_BEGIN(TSPacketMetadataTest);
    TSUNIT_TEST(testSize);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(TSPacketMetadataTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TSPacketMetadataTest::beforeTest()
{
}

// Test suite cleanup method.
void TSPacketMetadataTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TSPacketMetadataTest::testSize()
{
    ts::TSPacketMetadata arr[10];
    ts::TSPacketMetadataVector vec(10);
    ts::PacketMetadataBuffer buf(10);

    ts::TSPacketMetadata::DisplayLayout(debug(), "TSPacketMetadataTest::testSize: ");
    debug() << "TSPacketMetadataTest::testSize: sizeof(ts::TSPacketLabelSet): " << sizeof(ts::TSPacketLabelSet) << " bytes" << std::endl
            << "TSPacketMetadataTest::testSize: in array: " << (reinterpret_cast<char*>(&arr[1]) - reinterpret_cast<char*>(&arr[0])) << " bytes" << std::endl
            << "TSPacketMetadataTest::testSize: in vector: " << (reinterpret_cast<char*>(&vec[1]) - reinterpret_cast<char*>(&vec[0])) << " bytes" << std::endl
            << "TSPacketMetadataTest::testSize: in resident buffer: " << (reinterpret_cast<char*>(&buf.base()[1]) - reinterpret_cast<char*>(&buf.base()[0])) << " bytes"
            << ", is locked: " << buf.isLocked() << std::endl;

    TSUNIT_ASSUME(4 == sizeof(ts::TSPacketLabelSet));
}
