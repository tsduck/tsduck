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
//  TSUnit test suite for PESPacketizer classes.
//
//----------------------------------------------------------------------------

#include "tsPESOneShotPacketizer.h"
#include "tsPESDemux.h"
#include "tsDuckContext.h"
#include "tsTSPacket.h"
#include "tsCerrReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class PESPacketizerTest: public tsunit::Test, private ts::PESHandlerInterface
{
public:
    PESPacketizerTest();
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testPacketizer();

    TSUNIT_TEST_BEGIN(PESPacketizerTest);
    TSUNIT_TEST(testPacketizer);
    TSUNIT_TEST_END();

private:
    size_t _pes_count;
    virtual void handlePESPacket(ts::PESDemux& demux, const ts::PESPacket& packet) override;
};

TSUNIT_REGISTER(PESPacketizerTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
PESPacketizerTest::PESPacketizerTest() :
    _pes_count(0)
{
}

// Test suite initialization method.
void PESPacketizerTest::beforeTest()
{
    _pes_count = 0;
}

// Test suite cleanup method.
void PESPacketizerTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void PESPacketizerTest::testPacketizer()
{
    // Build two PES packets from scratch.
    uint8_t data1[1234];
    data1[0] = 0x00;  // start code prefix
    data1[1] = 0x00;
    data1[2] = 0x01;
    data1[3] = 0xBE;  // '1011 1110' = padding stream, no specific structure.
    ts::PutUInt16(data1 + 4, sizeof(data1) - 6);
    for (size_t i = 6; i < sizeof(data1); i++) {
        data1[i] = uint8_t(i + 27);
    }
    ts::PESPacket pes1(data1, sizeof(data1));
    pes1.setPCR(123456789);

    uint8_t data2[10000];
    data2[0] = 0x00;  // start code prefix
    data2[1] = 0x00;
    data2[2] = 0x01;
    data2[3] = 0xBE;  // '1011 1110' = padding stream, no specific structure.
    ts::PutUInt16(data2 + 4, sizeof(data2) - 6);
    for (size_t i = 6; i < sizeof(data2); i++) {
        data2[i] = uint8_t(i + 11);
    }
    ts::PESPacket pes2(data2, sizeof(data2));
    pes2.setPCR(987654321);

    // Packetize the two PES packets at once.
    ts::DuckContext duck;
    ts::PESOneShotPacketizer zer(duck, 100);
    TSUNIT_ASSERT(zer.empty());

    zer.addPES(pes1, ts::ShareMode::SHARE);
    zer.addPES(pes2, ts::ShareMode::SHARE);
    TSUNIT_ASSERT(!zer.empty());

    ts::TSPacketVector packets;
    zer.getPackets(packets);
    TSUNIT_ASSERT(packets.size() > 2);
    TSUNIT_ASSERT(packets[0].getPUSI());
    TSUNIT_ASSERT(!packets[1].getPUSI());

    // Check that there are exactly two start unit.
    size_t pusi_count = 0;
    for (size_t i = 0; i < packets.size(); ++i) {
        pusi_count += packets[i].getPUSI();
        TSUNIT_EQUAL(100, packets[i].getPID());
    }
    TSUNIT_EQUAL(2, pusi_count);

    // Now demux the TS packets and make sure we get the right PES packets.
    ts::PESDemux demux(duck, this);
    for (size_t i = 0; i < packets.size(); ++i) {
        demux.feedPacket(packets[i]);
    }
    TSUNIT_EQUAL(2, _pes_count);
}

void PESPacketizerTest::handlePESPacket(ts::PESDemux& demux, const ts::PESPacket& pes)
{
    _pes_count++;
    TSUNIT_ASSERT(pes.isValid());
    TSUNIT_EQUAL(100, pes.sourcePID());
    TSUNIT_EQUAL(6, pes.headerSize());
    switch (_pes_count) {
        case 1:
            TSUNIT_EQUAL(123456789, pes.getPCR());
            TSUNIT_EQUAL(1234, pes.size());
            for (size_t i = pes.headerSize(); i < pes.size(); i++) {
                TSUNIT_EQUAL(uint8_t(i + 27), pes.content()[i]);
            }
            break;
        case 2:
            TSUNIT_EQUAL(987654321, pes.getPCR());
            TSUNIT_EQUAL(10000, pes.size());
            for (size_t i = pes.headerSize(); i < pes.size(); i++) {
                TSUNIT_EQUAL(uint8_t(i + 11), pes.content()[i]);
            }
            break;
        default:
            TSUNIT_FAIL("invalid PES packet count");
    }
}
