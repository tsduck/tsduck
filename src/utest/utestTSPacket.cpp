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
#include "tsByteBlock.h"
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
    void testFlags();
    void testPrivateData();

    CPPUNIT_TEST_SUITE(TSPacketTest);
    CPPUNIT_TEST(testPacket);
    CPPUNIT_TEST(testInit);
    CPPUNIT_TEST(testCreatePCR);
    CPPUNIT_TEST(testAFStuffingSize);
    CPPUNIT_TEST(testSetPayloadSize);
    CPPUNIT_TEST(testFlags);
    CPPUNIT_TEST(testPrivateData);
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

    CPPUNIT_ASSERT(!pkt.setPCR(TS_UCONST64(0x000000126789ABCD), false));

    CPPUNIT_ASSERT(!pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(184), pkt.getPayloadSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT(!pkt.hasPCR());
    CPPUNIT_ASSERT_EQUAL(ts::INVALID_PCR, pkt.getPCR());

    CPPUNIT_ASSERT(pkt.setPCR(TS_UCONST64(0x000000126789ABCD), true));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(8), pkt.getAFSize());
    CPPUNIT_ASSERT(pkt.hasPayload());
    CPPUNIT_ASSERT_EQUAL(size_t(176), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.hasPCR());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x000000126789ABCD), pkt.getPCR());

    CPPUNIT_ASSERT(pkt.setPCR(TS_UCONST64(0x0000023456789ABC), true));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(8), pkt.getAFSize());
    CPPUNIT_ASSERT(pkt.hasPayload());
    CPPUNIT_ASSERT_EQUAL(size_t(176), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.hasPCR());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x0000023456789ABC), pkt.getPCR());

    pkt.removePCR();

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(176), pkt.getPayloadSize());
    CPPUNIT_ASSERT_EQUAL(size_t(8), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(6), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT(!pkt.hasPCR());

    CPPUNIT_ASSERT(pkt.setPCR(TS_UCONST64(0x00000089642CA4F7), true));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(8), pkt.getAFSize());
    CPPUNIT_ASSERT(pkt.hasPayload());
    CPPUNIT_ASSERT_EQUAL(size_t(176), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.hasPCR());
    CPPUNIT_ASSERT(!pkt.hasOPCR());
    CPPUNIT_ASSERT(!pkt.hasSpliceCountdown());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x00000089642CA4F7), pkt.getPCR());
    CPPUNIT_ASSERT_EQUAL(ts::INVALID_PCR, pkt.getOPCR());
    CPPUNIT_ASSERT_EQUAL(int8_t(0), pkt.getSpliceCountdown());

    CPPUNIT_ASSERT(!pkt.setSpliceCountdown(23, false));
    CPPUNIT_ASSERT(pkt.setSpliceCountdown(-97, true));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(9), pkt.getAFSize());
    CPPUNIT_ASSERT(pkt.hasPayload());
    CPPUNIT_ASSERT_EQUAL(size_t(175), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.hasPCR());
    CPPUNIT_ASSERT(!pkt.hasOPCR());
    CPPUNIT_ASSERT(pkt.hasSpliceCountdown());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x00000089642CA4F7), pkt.getPCR());
    CPPUNIT_ASSERT_EQUAL(ts::INVALID_PCR, pkt.getOPCR());
    CPPUNIT_ASSERT_EQUAL(int8_t(-97), pkt.getSpliceCountdown());

    CPPUNIT_ASSERT(pkt.setOPCR(TS_UCONST64(0x000000B964FEA456), true));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(15), pkt.getAFSize());
    CPPUNIT_ASSERT(pkt.hasPayload());
    CPPUNIT_ASSERT_EQUAL(size_t(169), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.hasPCR());
    CPPUNIT_ASSERT(pkt.hasOPCR());
    CPPUNIT_ASSERT(pkt.hasSpliceCountdown());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x00000089642CA4F7), pkt.getPCR());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x000000B964FEA456), pkt.getOPCR());
    CPPUNIT_ASSERT_EQUAL(-97, int(pkt.getSpliceCountdown()));
}

void TSPacketTest::testAFStuffingSize()
{
    ts::TSPacket pkt;

    pkt.init();
    CPPUNIT_ASSERT(!pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());

    CPPUNIT_ASSERT(pkt.setPCR(0, true));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(8), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());

    pkt.b[4] += 25;
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(33), pkt.getAFSize());
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
    CPPUNIT_ASSERT_EQUAL(size_t(84), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(82), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(100), pkt.getPayloadSize());

    CPPUNIT_ASSERT(pkt.setPayloadSize(130));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(54), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(52), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(130), pkt.getPayloadSize());

    CPPUNIT_ASSERT(!pkt.setPayloadSize(190));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(54), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(52), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(130), pkt.getPayloadSize());

    pkt.init();
    CPPUNIT_ASSERT(pkt.setPCR(0, true));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(8), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(176), pkt.getPayloadSize());

    CPPUNIT_ASSERT(pkt.setPayloadSize(100));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(84), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(76), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(100), pkt.getPayloadSize());

    // Test shift of payload.
    uint8_t* pl = pkt.getPayload();
    pl[0] = 0x10;
    pl[1] = 0x11;
    pl[2] = 0x12;
    pl[3] = 0x13;
    pl[4] = 0x14;
    pl[5] = 0x15;

    CPPUNIT_ASSERT(pkt.setPayloadSize(99, true));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(85), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(77), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(99), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.getPayload() == pl + 1);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x10), pkt.getPayload()[0]);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x11), pkt.getPayload()[1]);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x12), pkt.getPayload()[2]);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x13), pkt.getPayload()[3]);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x14), pkt.getPayload()[4]);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x15), pkt.getPayload()[5]);

    CPPUNIT_ASSERT(pkt.setPayloadSize(98, false));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(86), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(78), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(98), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.getPayload() == pl + 2);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x11), pkt.getPayload()[0]);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x12), pkt.getPayload()[1]);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x13), pkt.getPayload()[2]);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x14), pkt.getPayload()[3]);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x15), pkt.getPayload()[4]);
}

void TSPacketTest::testFlags()
{
    ts::TSPacket pkt;
    pkt.init();

    uint8_t* pl = pkt.getPayload();
    pl[0] = 0x10;
    pl[1] = 0x11;
    pl[2] = 0x12;
    pl[3] = 0x13;
    pl[4] = 0x14;
    pl[5] = 0x15;

    CPPUNIT_ASSERT(!pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(184), pkt.getPayloadSize());

    CPPUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    CPPUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    CPPUNIT_ASSERT(!pkt.getESPI());

    CPPUNIT_ASSERT(!pkt.setDiscontinuityIndicator(false));
    CPPUNIT_ASSERT(!pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(184), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.getPayload() == pl);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x10), pkt.getPayload()[0]);

    CPPUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    CPPUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    CPPUNIT_ASSERT(!pkt.getESPI());

    CPPUNIT_ASSERT(pkt.setDiscontinuityIndicator(true));
    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(2), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(182), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.getPayload() == pl + 2);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x10), pkt.getPayload()[0]);

    CPPUNIT_ASSERT(pkt.getDiscontinuityIndicator());
    CPPUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    CPPUNIT_ASSERT(!pkt.getESPI());

    pkt.clearDiscontinuityIndicator();
    CPPUNIT_ASSERT(pkt.setRandomAccessIndicator(true));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(2), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(182), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.getPayload() == pl + 2);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x10), pkt.getPayload()[0]);

    CPPUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    CPPUNIT_ASSERT(pkt.getRandomAccessIndicator());
    CPPUNIT_ASSERT(!pkt.getESPI());

    pkt.clearRandomAccessIndicator();
    CPPUNIT_ASSERT(pkt.setESPI(true));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT_EQUAL(size_t(2), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(182), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.getPayload() == pl + 2);
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x10), pkt.getPayload()[0]);

    CPPUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    CPPUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    CPPUNIT_ASSERT(pkt.getESPI());
}


void TSPacketTest::testPrivateData()
{
    ts::ByteBlock data;
    ts::TSPacket pkt;
    pkt.init();

    CPPUNIT_ASSERT(!pkt.hasAF());
    CPPUNIT_ASSERT(!pkt.hasPrivateData());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getPrivateDataSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(184), pkt.getPayloadSize());

    const ts::ByteBlock refPayload({0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29});
    const ts::ByteBlock refPrivate1({0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59});
    const ts::ByteBlock refPrivate2({0x60, 0x61, 0x62});

    uint8_t* pl = pkt.getPayload();
    ::memcpy(pl, refPayload.data(), refPayload.size());

    CPPUNIT_ASSERT(!pkt.setPrivateData(refPrivate1, false));

    CPPUNIT_ASSERT(!pkt.hasAF());
    CPPUNIT_ASSERT(!pkt.hasPrivateData());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getPrivateDataSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(184), pkt.getPayloadSize());

    CPPUNIT_ASSERT(pkt.setPrivateData(refPrivate1, true));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT(pkt.hasPrivateData());
    CPPUNIT_ASSERT_EQUAL(size_t(13), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(10), pkt.getPrivateDataSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(171), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.getPayload() == pl + 13);
    CPPUNIT_ASSERT(::memcmp(pkt.getPayload(), refPayload.data(), refPayload.size()) == 0);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(ts::UString::Dump(refPrivate1, ts::UString::SINGLE_LINE),
                                  ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    CPPUNIT_ASSERT(::memcmp(pkt.getPrivateData(), refPrivate1.data(), refPrivate1.size()) == 0);
    pkt.getPrivateData(data);
    CPPUNIT_ASSERT(data == refPrivate1);

    CPPUNIT_ASSERT(pkt.setPrivateData(refPrivate2, false));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT(pkt.hasPrivateData());
    CPPUNIT_ASSERT_EQUAL(size_t(13), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(3), pkt.getPrivateDataSize());
    CPPUNIT_ASSERT_EQUAL(size_t(7), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(171), pkt.getPayloadSize());
    CPPUNIT_ASSERT(pkt.getPayload() == pl + 13);
    CPPUNIT_ASSERT(::memcmp(pkt.getPayload(), refPayload.data(), refPayload.size()) == 0);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(ts::UString::Dump(refPrivate2, ts::UString::SINGLE_LINE),
                                  ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    CPPUNIT_ASSERT(::memcmp(pkt.getPrivateData(), refPrivate2.data(), refPrivate2.size()) == 0);
    pkt.getPrivateData(data);
    CPPUNIT_ASSERT(data == refPrivate2);

    CPPUNIT_ASSERT(pkt.setPCR(TS_UCONST64(0x000000126789ABCD), false));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT(pkt.hasPCR());
    CPPUNIT_ASSERT(pkt.hasPrivateData());
    CPPUNIT_ASSERT_EQUAL(size_t(13), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(3), pkt.getPrivateDataSize());
    CPPUNIT_ASSERT_EQUAL(size_t(1), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(171), pkt.getPayloadSize());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x000000126789ABCD), pkt.getPCR());
    CPPUNIT_ASSERT(pkt.getPayload() == pl + 13);
    CPPUNIT_ASSERT(::memcmp(pkt.getPayload(), refPayload.data(), refPayload.size()) == 0);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(ts::UString::Dump(refPrivate2, ts::UString::SINGLE_LINE),
                                  ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    CPPUNIT_ASSERT(::memcmp(pkt.getPrivateData(), refPrivate2.data(), refPrivate2.size()) == 0);
    data.clear();
    pkt.getPrivateData(data);
    CPPUNIT_ASSERT(data == refPrivate2);

    CPPUNIT_ASSERT(!pkt.setOPCR(TS_UCONST64(0x000000AB67925678), false));
    CPPUNIT_ASSERT(pkt.setOPCR(TS_UCONST64(0x000000AB67925678), true));

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT(pkt.hasPCR());
    CPPUNIT_ASSERT(pkt.hasOPCR());
    CPPUNIT_ASSERT(pkt.hasPrivateData());
    CPPUNIT_ASSERT_EQUAL(size_t(18), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(3), pkt.getPrivateDataSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(166), pkt.getPayloadSize());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x000000126789ABCD), pkt.getPCR());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x000000AB67925678), pkt.getOPCR());
    CPPUNIT_ASSERT(pkt.getPayload() == pl + 18);
    CPPUNIT_ASSERT(::memcmp(pkt.getPayload(), refPayload.data(), refPayload.size()) == 0);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(ts::UString::Dump(refPrivate2, ts::UString::SINGLE_LINE),
                                  ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    CPPUNIT_ASSERT(::memcmp(pkt.getPrivateData(), refPrivate2.data(), refPrivate2.size()) == 0);
    data.clear();
    pkt.getPrivateData(data);
    CPPUNIT_ASSERT(data == refPrivate2);

    pkt.removePrivateData();

    CPPUNIT_ASSERT(pkt.hasAF());
    CPPUNIT_ASSERT(pkt.hasPCR());
    CPPUNIT_ASSERT(pkt.hasOPCR());
    CPPUNIT_ASSERT(!pkt.hasPrivateData());
    CPPUNIT_ASSERT_EQUAL(size_t(18), pkt.getAFSize());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pkt.getPrivateDataSize());
    CPPUNIT_ASSERT_EQUAL(size_t(4), pkt.getAFStuffingSize());
    CPPUNIT_ASSERT_EQUAL(size_t(166), pkt.getPayloadSize());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x000000126789ABCD), pkt.getPCR());
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0x000000AB67925678), pkt.getOPCR());
    CPPUNIT_ASSERT(pkt.getPayload() == pl + 18);
    CPPUNIT_ASSERT(::memcmp(pkt.getPayload(), refPayload.data(), refPayload.size()) == 0);
    pkt.getPrivateData(data);
    CPPUNIT_ASSERT(data.empty());
}
