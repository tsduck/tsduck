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
//  TSUnit test suite for class ts::MPEPacket
//
//----------------------------------------------------------------------------

#include "tsMPEPacket.h"
#include "tsunit.h"
#include "tables/psi_mpe_sections.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class MPEPacketTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testSection();
    void testBuild();

    TSUNIT_TEST_BEGIN(MPEPacketTest);
    TSUNIT_TEST(testSection);
    TSUNIT_TEST(testBuild);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(MPEPacketTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void MPEPacketTest::beforeTest()
{
}

// Test suite cleanup method.
void MPEPacketTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void MPEPacketTest::testSection()
{
    const ts::PID pid = 1234;
    ts::Section sec(psi_mpe_sections, sizeof(psi_mpe_sections), pid, ts::CRC32::CHECK);

    TSUNIT_ASSERT(sec.isValid());
    TSUNIT_EQUAL(ts::TID_DSMCC_PD, sec.tableId()); // DSM-CC Private Data
    TSUNIT_EQUAL(pid, sec.sourcePID());
    TSUNIT_ASSERT(sec.isLongSection());

    ts::MPEPacket mpe(sec);
    TSUNIT_ASSERT(mpe.isValid());
    TSUNIT_EQUAL(pid, mpe.sourcePID());
    TSUNIT_ASSERT(mpe.destinationMACAddress() == ts::MACAddress(0x01, 0x00, 0x5E, 0x14, 0x14, 0x02));
    TSUNIT_ASSERT(mpe.destinationIPAddress() == ts::IPv4Address(224, 20, 20, 2));
    TSUNIT_ASSERT(mpe.sourceIPAddress() == ts::IPv4Address(192, 168, 135, 190));
    TSUNIT_EQUAL(6000, mpe.sourceUDPPort());
    TSUNIT_EQUAL(6000, mpe.destinationUDPPort());
    TSUNIT_EQUAL(1468, mpe.udpMessageSize());
}

void MPEPacketTest::testBuild()
{
    ts::MPEPacket mpe;
    TSUNIT_ASSERT(!mpe.isValid());
    TSUNIT_EQUAL(ts::PID_NULL, mpe.sourcePID());

    static const uint8_t ref[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    mpe.setSourcePID(765);
    mpe.setDestinationMACAddress(ts::MACAddress(6, 7, 8, 9, 10, 11));
    mpe.setSourceIPAddress(ts::IPv4Address(54, 59, 197, 201));
    mpe.setDestinationIPAddress(ts::IPv4Address(123, 34, 45, 78));
    mpe.setSourceUDPPort(7920);
    mpe.setDestinationUDPPort(4654);
    mpe.setUDPMessage(ref, sizeof(ref));

    TSUNIT_ASSERT(mpe.isValid());
    TSUNIT_EQUAL(765, mpe.sourcePID());
    TSUNIT_ASSERT(mpe.destinationMACAddress() == ts::MACAddress(6, 7, 8, 9, 10, 11));
    TSUNIT_ASSERT(mpe.sourceIPAddress() == ts::IPv4Address(54, 59, 197, 201));
    TSUNIT_ASSERT(mpe.destinationIPAddress() == ts::IPv4Address(123, 34, 45, 78));
    TSUNIT_EQUAL(7920, mpe.sourceUDPPort());
    TSUNIT_EQUAL(4654, mpe.destinationUDPPort());
    TSUNIT_EQUAL(sizeof(ref), mpe.udpMessageSize());
    TSUNIT_ASSERT(mpe.udpMessage() != nullptr);
    TSUNIT_EQUAL(0, ::memcmp(mpe.udpMessage(), ref, mpe.udpMessageSize()));

    ts::Section sect;
    mpe.createSection(sect);
    TSUNIT_ASSERT(sect.isValid());

    ts::MPEPacket mpe2(sect);
    TSUNIT_ASSERT(mpe2.isValid());
    TSUNIT_EQUAL(765, mpe2.sourcePID());
    TSUNIT_ASSERT(mpe2.destinationMACAddress() == ts::MACAddress(6, 7, 8, 9, 10, 11));
    TSUNIT_ASSERT(mpe2.sourceIPAddress() == ts::IPv4Address(54, 59, 197, 201));
    TSUNIT_ASSERT(mpe2.destinationIPAddress() == ts::IPv4Address(123, 34, 45, 78));
    TSUNIT_EQUAL(7920, mpe2.sourceUDPPort());
    TSUNIT_EQUAL(4654, mpe2.destinationUDPPort());
    TSUNIT_EQUAL(sizeof(ref), mpe2.udpMessageSize());
    TSUNIT_ASSERT(mpe2.udpMessage() != nullptr);
    TSUNIT_EQUAL(0, ::memcmp(mpe2.udpMessage(), ref, mpe2.udpMessageSize()));
}
