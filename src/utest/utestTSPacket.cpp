//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//  TSUnit test suite for class ts::TSPacket
//
//----------------------------------------------------------------------------

#include "tsTSPacket.h"
#include "tsByteBlock.h"
#include "tsMemory.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TSPacketTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testPacket();
    void testInit();
    void testCreatePCR();
    void testAFStuffingSize();
    void testSetPayloadSize();
    void testFlags();
    void testPrivateData();

    TSUNIT_TEST_BEGIN(TSPacketTest);
    TSUNIT_TEST(testPacket);
    TSUNIT_TEST(testInit);
    TSUNIT_TEST(testCreatePCR);
    TSUNIT_TEST(testAFStuffingSize);
    TSUNIT_TEST(testSetPayloadSize);
    TSUNIT_TEST(testFlags);
    TSUNIT_TEST(testPrivateData);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(TSPacketTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TSPacketTest::beforeTest()
{
}

// Test suite cleanup method.
void TSPacketTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TSPacketTest::testPacket()
{
    ts::TSPacket::SanityCheck();

    ts::NullPacket.display(debug(), ts::TSPacket::DUMP_TS_HEADER | ts::TSPacket::DUMP_RAW);

    ts::TSPacket packets[7];
    TS_ZERO(packets); // to avoid unreferenced or uninitialized warning

    TSUNIT_EQUAL(7 * ts::PKT_SIZE, sizeof(packets));
}

void TSPacketTest::testInit()
{
    ts::TSPacket pkt;
    pkt.init(0x1ABC, 7, 0x35);
    TSUNIT_ASSERT(pkt.hasValidSync());
    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(7, pkt.getCC());
    TSUNIT_EQUAL(0x1ABC, pkt.getPID());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());
    for (size_t i = 4; i < ts::PKT_SIZE; ++i) {
        TSUNIT_EQUAL(0x35, pkt.b[i]);
    }
}

void TSPacketTest::testCreatePCR()
{
    ts::TSPacket pkt;
    pkt.init(0x1ABC);

    TSUNIT_ASSERT(pkt.hasValidSync());
    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(0x1ABC, pkt.getPID());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());
    TSUNIT_ASSERT(!pkt.hasPCR());
    TSUNIT_EQUAL(ts::INVALID_PCR, pkt.getPCR());

    TSUNIT_ASSERT(!pkt.setPCR(TS_UCONST64(0x000000126789ABCD), false));

    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_ASSERT(!pkt.hasPCR());
    TSUNIT_EQUAL(ts::INVALID_PCR, pkt.getPCR());

    TSUNIT_ASSERT(pkt.setPCR(TS_UCONST64(0x000000126789ABCD), true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(176, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_EQUAL(TS_UCONST64(0x000000126789ABCD), pkt.getPCR());

    TSUNIT_ASSERT(pkt.setPCR(TS_UCONST64(0x0000023456789ABC), true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(176, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_EQUAL(TS_UCONST64(0x0000023456789ABC), pkt.getPCR());

    pkt.removePCR();

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(176, pkt.getPayloadSize());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_EQUAL(6, pkt.getAFStuffingSize());
    TSUNIT_ASSERT(!pkt.hasPCR());

    TSUNIT_ASSERT(pkt.setPCR(TS_UCONST64(0x00000089642CA4F7), true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(176, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(!pkt.hasOPCR());
    TSUNIT_ASSERT(!pkt.hasSpliceCountdown());
    TSUNIT_EQUAL(TS_UCONST64(0x00000089642CA4F7), pkt.getPCR());
    TSUNIT_EQUAL(ts::INVALID_PCR, pkt.getOPCR());
    TSUNIT_EQUAL(0, pkt.getSpliceCountdown());

    TSUNIT_ASSERT(!pkt.setSpliceCountdown(23, false));
    TSUNIT_ASSERT(pkt.setSpliceCountdown(-97, true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(9, pkt.getAFSize());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(175, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(!pkt.hasOPCR());
    TSUNIT_ASSERT(pkt.hasSpliceCountdown());
    TSUNIT_EQUAL(TS_UCONST64(0x00000089642CA4F7), pkt.getPCR());
    TSUNIT_EQUAL(ts::INVALID_PCR, pkt.getOPCR());
    TSUNIT_EQUAL(-97, pkt.getSpliceCountdown());

    TSUNIT_ASSERT(pkt.setOPCR(TS_UCONST64(0x000000B964FEA456), true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(15, pkt.getAFSize());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(169, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(pkt.hasOPCR());
    TSUNIT_ASSERT(pkt.hasSpliceCountdown());
    TSUNIT_EQUAL(TS_UCONST64(0x00000089642CA4F7), pkt.getPCR());
    TSUNIT_EQUAL(TS_UCONST64(0x000000B964FEA456), pkt.getOPCR());
    TSUNIT_EQUAL(-97, int(pkt.getSpliceCountdown()));
}

void TSPacketTest::testAFStuffingSize()
{
    ts::TSPacket pkt;

    pkt.init();
    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());

    TSUNIT_ASSERT(pkt.setPCR(0, true));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());

    pkt.b[4] += 25;
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(33, pkt.getAFSize());
    TSUNIT_EQUAL(25, pkt.getAFStuffingSize());
}

void TSPacketTest::testSetPayloadSize()
{
    ts::TSPacket pkt;

    pkt.init();
    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());

    TSUNIT_ASSERT(pkt.setPayloadSize(100));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(84, pkt.getAFSize());
    TSUNIT_EQUAL(82, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(100, pkt.getPayloadSize());

    TSUNIT_ASSERT(pkt.setPayloadSize(130));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(54, pkt.getAFSize());
    TSUNIT_EQUAL(52, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(130, pkt.getPayloadSize());

    TSUNIT_ASSERT(!pkt.setPayloadSize(190));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(54, pkt.getAFSize());
    TSUNIT_EQUAL(52, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(130, pkt.getPayloadSize());

    pkt.init();
    TSUNIT_ASSERT(pkt.setPCR(0, true));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(176, pkt.getPayloadSize());

    TSUNIT_ASSERT(pkt.setPayloadSize(100));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(84, pkt.getAFSize());
    TSUNIT_EQUAL(76, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(100, pkt.getPayloadSize());

    // Test shift of payload.
    uint8_t* pl = pkt.getPayload();
    pl[0] = 0x10;
    pl[1] = 0x11;
    pl[2] = 0x12;
    pl[3] = 0x13;
    pl[4] = 0x14;
    pl[5] = 0x15;

    TSUNIT_ASSERT(pkt.setPayloadSize(99, true));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(85, pkt.getAFSize());
    TSUNIT_EQUAL(77, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(99, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 1);
    TSUNIT_EQUAL(0x10, pkt.getPayload()[0]);
    TSUNIT_EQUAL(0x11, pkt.getPayload()[1]);
    TSUNIT_EQUAL(0x12, pkt.getPayload()[2]);
    TSUNIT_EQUAL(0x13, pkt.getPayload()[3]);
    TSUNIT_EQUAL(0x14, pkt.getPayload()[4]);
    TSUNIT_EQUAL(0x15, pkt.getPayload()[5]);

    TSUNIT_ASSERT(pkt.setPayloadSize(98, false));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(86, pkt.getAFSize());
    TSUNIT_EQUAL(78, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(98, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 2);
    TSUNIT_EQUAL(0x11, pkt.getPayload()[0]);
    TSUNIT_EQUAL(0x12, pkt.getPayload()[1]);
    TSUNIT_EQUAL(0x13, pkt.getPayload()[2]);
    TSUNIT_EQUAL(0x14, pkt.getPayload()[3]);
    TSUNIT_EQUAL(0x15, pkt.getPayload()[4]);
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

    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());

    TSUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    TSUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    TSUNIT_ASSERT(!pkt.getESPI());

    TSUNIT_ASSERT(!pkt.setDiscontinuityIndicator(false));
    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl);
    TSUNIT_EQUAL(0x10, pkt.getPayload()[0]);

    TSUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    TSUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    TSUNIT_ASSERT(!pkt.getESPI());

    TSUNIT_ASSERT(pkt.setDiscontinuityIndicator(true));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(2, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(182, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 2);
    TSUNIT_EQUAL(0x10, pkt.getPayload()[0]);

    TSUNIT_ASSERT(pkt.getDiscontinuityIndicator());
    TSUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    TSUNIT_ASSERT(!pkt.getESPI());

    pkt.clearDiscontinuityIndicator();
    TSUNIT_ASSERT(pkt.setRandomAccessIndicator(true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(2, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(182, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 2);
    TSUNIT_EQUAL(0x10, pkt.getPayload()[0]);

    TSUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    TSUNIT_ASSERT(pkt.getRandomAccessIndicator());
    TSUNIT_ASSERT(!pkt.getESPI());

    pkt.clearRandomAccessIndicator();
    TSUNIT_ASSERT(pkt.setESPI(true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(2, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(182, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 2);
    TSUNIT_EQUAL(0x10, pkt.getPayload()[0]);

    TSUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    TSUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    TSUNIT_ASSERT(pkt.getESPI());
}


void TSPacketTest::testPrivateData()
{
    ts::ByteBlock data;
    ts::TSPacket pkt;
    pkt.init();

    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_ASSERT(!pkt.hasPrivateData());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());

    const ts::ByteBlock refPayload({0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29});
    const ts::ByteBlock refPrivate1({0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59});
    const ts::ByteBlock refPrivate2({0x60, 0x61, 0x62});

    uint8_t* pl = pkt.getPayload();
    ::memcpy(pl, refPayload.data(), refPayload.size());

    TSUNIT_ASSERT(!pkt.setPrivateData(refPrivate1, false));

    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_ASSERT(!pkt.hasPrivateData());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());

    TSUNIT_ASSERT(pkt.setPrivateData(refPrivate1, true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPrivateData());
    TSUNIT_EQUAL(13, pkt.getAFSize());
    TSUNIT_EQUAL(10, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(171, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 13);
    TSUNIT_ASSERT(::memcmp(pkt.getPayload(), refPayload.data(), refPayload.size()) == 0);
    TSUNIT_EQUAL(ts::UString::Dump(refPrivate1, ts::UString::SINGLE_LINE),
                 ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    TSUNIT_ASSERT(::memcmp(pkt.getPrivateData(), refPrivate1.data(), refPrivate1.size()) == 0);
    pkt.getPrivateData(data);
    TSUNIT_ASSERT(data == refPrivate1);

    TSUNIT_ASSERT(pkt.setPrivateData(refPrivate2, false));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPrivateData());
    TSUNIT_EQUAL(13, pkt.getAFSize());
    TSUNIT_EQUAL(3, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(7, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(171, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 13);
    TSUNIT_ASSERT(::memcmp(pkt.getPayload(), refPayload.data(), refPayload.size()) == 0);
    TSUNIT_EQUAL(ts::UString::Dump(refPrivate2, ts::UString::SINGLE_LINE),
                 ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    TSUNIT_ASSERT(::memcmp(pkt.getPrivateData(), refPrivate2.data(), refPrivate2.size()) == 0);
    pkt.getPrivateData(data);
    TSUNIT_ASSERT(data == refPrivate2);

    TSUNIT_ASSERT(pkt.setPCR(TS_UCONST64(0x000000126789ABCD), false));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(pkt.hasPrivateData());
    TSUNIT_EQUAL(13, pkt.getAFSize());
    TSUNIT_EQUAL(3, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(1, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(171, pkt.getPayloadSize());
    TSUNIT_EQUAL(TS_UCONST64(0x000000126789ABCD), pkt.getPCR());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 13);
    TSUNIT_ASSERT(::memcmp(pkt.getPayload(), refPayload.data(), refPayload.size()) == 0);
    TSUNIT_EQUAL(ts::UString::Dump(refPrivate2, ts::UString::SINGLE_LINE),
                 ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    TSUNIT_ASSERT(::memcmp(pkt.getPrivateData(), refPrivate2.data(), refPrivate2.size()) == 0);
    data.clear();
    pkt.getPrivateData(data);
    TSUNIT_ASSERT(data == refPrivate2);

    TSUNIT_ASSERT(!pkt.setOPCR(TS_UCONST64(0x000000AB67925678), false));
    TSUNIT_ASSERT(pkt.setOPCR(TS_UCONST64(0x000000AB67925678), true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(pkt.hasOPCR());
    TSUNIT_ASSERT(pkt.hasPrivateData());
    TSUNIT_EQUAL(18, pkt.getAFSize());
    TSUNIT_EQUAL(3, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(166, pkt.getPayloadSize());
    TSUNIT_EQUAL(TS_UCONST64(0x000000126789ABCD), pkt.getPCR());
    TSUNIT_EQUAL(TS_UCONST64(0x000000AB67925678), pkt.getOPCR());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 18);
    TSUNIT_ASSERT(::memcmp(pkt.getPayload(), refPayload.data(), refPayload.size()) == 0);
    TSUNIT_EQUAL(ts::UString::Dump(refPrivate2, ts::UString::SINGLE_LINE),
                                  ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    TSUNIT_ASSERT(::memcmp(pkt.getPrivateData(), refPrivate2.data(), refPrivate2.size()) == 0);
    data.clear();
    pkt.getPrivateData(data);
    TSUNIT_ASSERT(data == refPrivate2);

    pkt.removePrivateData();

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(pkt.hasOPCR());
    TSUNIT_ASSERT(!pkt.hasPrivateData());
    TSUNIT_EQUAL(18, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(4, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(166, pkt.getPayloadSize());
    TSUNIT_EQUAL(TS_UCONST64(0x000000126789ABCD), pkt.getPCR());
    TSUNIT_EQUAL(TS_UCONST64(0x000000AB67925678), pkt.getOPCR());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 18);
    TSUNIT_ASSERT(::memcmp(pkt.getPayload(), refPayload.data(), refPayload.size()) == 0);
    pkt.getPrivateData(data);
    TSUNIT_ASSERT(data.empty());
}
