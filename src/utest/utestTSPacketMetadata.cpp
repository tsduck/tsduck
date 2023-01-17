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
