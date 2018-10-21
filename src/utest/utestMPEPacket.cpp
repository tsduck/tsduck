//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  CppUnit test suite for class ts::MPEPacket
//
//----------------------------------------------------------------------------

#include "tsMPEPacket.h"
#include "utestCppUnitTest.h"
#include "tables/psi_mpe_sections.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class MPEPacketTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testSection();
    void testBuild();

    CPPUNIT_TEST_SUITE(MPEPacketTest);
    CPPUNIT_TEST(testSection);
    CPPUNIT_TEST(testBuild);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MPEPacketTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void MPEPacketTest::setUp()
{
}

// Test suite cleanup method.
void MPEPacketTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void MPEPacketTest::testSection()
{
    const ts::PID pid = 1234;
    ts::Section sec(psi_mpe_sections, sizeof(psi_mpe_sections), pid, ts::CRC32::CHECK);

    CPPUNIT_ASSERT(sec.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_DSMCC_PD), sec.tableId()); // DSM-CC Private Data
    CPPUNIT_ASSERT_EQUAL(pid, sec.sourcePID());
    CPPUNIT_ASSERT(sec.isLongSection());

    ts::MPEPacket mpe(sec);
    CPPUNIT_ASSERT(mpe.isValid());
    CPPUNIT_ASSERT_EQUAL(pid, mpe.sourcePID());
    CPPUNIT_ASSERT(mpe.destinationMACAddress() == ts::MACAddress(0x01, 0x00, 0x5E, 0x14, 0x14, 0x02));
    CPPUNIT_ASSERT(mpe.destinationIPAddress() == ts::IPAddress(224, 20, 20, 2));
    CPPUNIT_ASSERT(mpe.sourceIPAddress() == ts::IPAddress(192, 168, 135, 190));
    CPPUNIT_ASSERT_EQUAL(uint16_t(6000), mpe.sourceUDPPort());
    CPPUNIT_ASSERT_EQUAL(uint16_t(6000), mpe.destinationUDPPort());
    CPPUNIT_ASSERT_EQUAL(size_t(1468), mpe.udpMessageSize());
}

void MPEPacketTest::testBuild()
{
    ts::MPEPacket mpe;
    CPPUNIT_ASSERT(!mpe.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_NULL), mpe.sourcePID());

    static const uint8_t ref[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    mpe.setSourcePID(765);
    mpe.setDestinationMACAddress(ts::MACAddress(6, 7, 8, 9, 10, 11));
    mpe.setSourceIPAddress(ts::IPAddress(54, 59, 197, 201));
    mpe.setDestinationIPAddress(ts::IPAddress(123, 34, 45, 78));
    mpe.setSourceUDPPort(7920);
    mpe.setDestinationUDPPort(4654);
    mpe.setUDPMessage(ref, sizeof(ref));

    CPPUNIT_ASSERT(mpe.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::PID(765), mpe.sourcePID());
    CPPUNIT_ASSERT(mpe.destinationMACAddress() == ts::MACAddress(6, 7, 8, 9, 10, 11));
    CPPUNIT_ASSERT(mpe.sourceIPAddress() == ts::IPAddress(54, 59, 197, 201));
    CPPUNIT_ASSERT(mpe.destinationIPAddress() == ts::IPAddress(123, 34, 45, 78));
    CPPUNIT_ASSERT_EQUAL(uint16_t(7920), mpe.sourceUDPPort());
    CPPUNIT_ASSERT_EQUAL(uint16_t(4654), mpe.destinationUDPPort());
    CPPUNIT_ASSERT_EQUAL(sizeof(ref), mpe.udpMessageSize());
    CPPUNIT_ASSERT(mpe.udpMessage() != nullptr);
    CPPUNIT_ASSERT_EQUAL(0, ::memcmp(mpe.udpMessage(), ref, mpe.udpMessageSize()));

    ts::Section sect;
    mpe.createSection(sect);
    CPPUNIT_ASSERT(sect.isValid());

    ts::MPEPacket mpe2(sect);
    CPPUNIT_ASSERT(mpe2.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::PID(765), mpe2.sourcePID());
    CPPUNIT_ASSERT(mpe2.destinationMACAddress() == ts::MACAddress(6, 7, 8, 9, 10, 11));
    CPPUNIT_ASSERT(mpe2.sourceIPAddress() == ts::IPAddress(54, 59, 197, 201));
    CPPUNIT_ASSERT(mpe2.destinationIPAddress() == ts::IPAddress(123, 34, 45, 78));
    CPPUNIT_ASSERT_EQUAL(uint16_t(7920), mpe2.sourceUDPPort());
    CPPUNIT_ASSERT_EQUAL(uint16_t(4654), mpe2.destinationUDPPort());
    CPPUNIT_ASSERT_EQUAL(sizeof(ref), mpe2.udpMessageSize());
    CPPUNIT_ASSERT(mpe2.udpMessage() != nullptr);
    CPPUNIT_ASSERT_EQUAL(0, ::memcmp(mpe2.udpMessage(), ref, mpe2.udpMessageSize()));
}
