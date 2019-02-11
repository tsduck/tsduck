//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  CppUnit test suite for class ts::TSPacket
//
//----------------------------------------------------------------------------

#include "tsTSPacket.h"
#include "tsMemoryUtils.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TSPacketTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testPacket();
    void testInit();
    void testCreatePCR();
    void testAFStuffingSize();
    void testSetPayloadSize();

    CPPUNIT_TEST_SUITE(TSPacketTest);
    CPPUNIT_TEST(testPacket);
    CPPUNIT_TEST(testInit);
    CPPUNIT_TEST(testCreatePCR);
    CPPUNIT_TEST(testAFStuffingSize);
    CPPUNIT_TEST(testSetPayloadSize);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TSPacketTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TSPacketTest::setUp()
{
}

// Test suite cleanup method.
void TSPacketTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TSPacketTest::testPacket()
{
    ts::TSPacket::SanityCheck();

    ts::NullPacket.display(utest::Out(), ts::TSPacket::DUMP_TS_HEADER | ts::TSPacket::DUMP_RAW);

    ts::TSPacket packets[7];
    TS_ZERO(packets); // to avoid unreferenced or uninitialized warning

    CPPUNIT_ASSERT_EQUAL(size_t(7 * ts::PKT_SIZE), sizeof(packets));
}

void TSPacketTest::testInit()
{
    ts::TSPacket pkt;
    pkt.init(0x1ABC, 7, 0x35);
    CPPUNIT_ASSERT(pkt.hasValidSync());
    CPPUNIT_ASSERT(!pkt.hasAF());
    CPPUNIT_ASSERT(pkt.hasPayload());
    CPPUNIT_ASSERT_EQUAL(uint8_t(7), pkt.getCC());
    CPPUNIT_ASSERT_EQUAL(ts::PID(0x1ABC), pkt.getPID());
    CPPUNIT_ASSERT_EQUAL(size_t(184), pkt.getPayloadSize());
    for (size_t i = 4; i < ts::PKT_SIZE; ++i) {
        CPPUNIT_ASSERT_EQUAL(uint8_t(0x35), pkt.b[i]);
    }
}

void TSPacketTest::testCreatePCR()
{
    ts::TSPacket pkt;
    pkt.init(0x1ABC);

    CPPUNIT_ASSERT(pkt.hasValidSync());
    CPPUNIT_ASSERT(!pkt.hasAF());
    CPPUNIT_ASSERT(pkt.hasPayload());
    CPPUNIT_ASSERT_EQUAL(ts::PID(0x1ABC), pkt.getPID());
    CPPUNIT_ASSERT_EQUAL(size_t(184), pkt.getPayloadSize());
    CPPUNIT_ASSERT(!pkt.hasPCR());
    CPPUNIT_ASSERT_EQUAL(ts::INVALID_PCR, pkt.getPCR());

    pkt.createPCR(TS_UCONST64(0x000000126789ABCD));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(7), pkt.getAFSize());
    CPPUNIT_ASSERT(pkt.hasPayload());
    CPPUNIT_ASSERT_EQUAL(size_t(176), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.hasPCR());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x000000126789ABCD), pkt.getPCR());

    pkt.createPCR(TS_UCONST64(0x0000023456789ABC));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(7), pkt.getAFSize());
    CPPUNIT_ASSERT(pkt.hasPayload());
    CPPUNIT_ASSERT_EQUAL(size_t(176), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.hasPCR());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x0000023456789ABC), pkt.getPCR());
}

void TSPacketTest::testAFStuffingSize()
{
    ts::TSPacket pkt;

    pkt.init();
    CPPUNIT_ASSERT(!pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());

    pkt.createPCR(0);
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(7), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());

    pkt.b[4] += 25;
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(32), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(25), pkt.getAFStuffingSize());
}

void TSPacketTest::testSetPayloadSize()
{
    ts::TSPacket pkt;

    pkt.init();
    CPPUNIT_ASSERT(!pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(184), pkt.getPayloadSize());

    CPPUNIT_ASSERT(pkt.setPayloadSize(100));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(83), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(82), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(100), pkt.getPayloadSize());

    CPPUNIT_ASSERT(pkt.setPayloadSize(130));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(53), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(52), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(130), pkt.getPayloadSize());

    CPPUNIT_ASSERT(!pkt.setPayloadSize(190));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(53), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(52), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(130), pkt.getPayloadSize());

    pkt.init();
    pkt.createPCR(0);
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(7), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(176), pkt.getPayloadSize());

    CPPUNIT_ASSERT(pkt.setPayloadSize(100));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(83), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(76), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(100), pkt.getPayloadSize());
}
