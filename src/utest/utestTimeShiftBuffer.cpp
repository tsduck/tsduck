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
//  TSUnit test suite for class ts::TimeShiftBuffer
//
//----------------------------------------------------------------------------

#include "tsTimeShiftBuffer.h"
#include "tsCerrReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TimeShiftBufferTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testMinimum();
    void testMemory();
    void testFile();

    TSUNIT_TEST_BEGIN(TimeShiftBufferTest);
    TSUNIT_TEST(testMinimum);
    TSUNIT_TEST(testMemory);
    TSUNIT_TEST(testFile);
    TSUNIT_TEST_END();

private:
    void testCommon(uint8_t total, uint8_t memory);
};

TSUNIT_REGISTER(TimeShiftBufferTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TimeShiftBufferTest::beforeTest()
{
}

// Test suite cleanup method.
void TimeShiftBufferTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TimeShiftBufferTest::testCommon(uint8_t total, uint8_t memory)
{
    ts::TimeShiftBuffer buf(total);
    TSUNIT_ASSERT(buf.setMemoryPackets(memory));
    TSUNIT_ASSERT(!buf.isOpen());
    TSUNIT_ASSERT(buf.open(CERR));
    TSUNIT_ASSERT(buf.isOpen());
    TSUNIT_EQUAL(total, buf.size());
    TSUNIT_EQUAL(0, buf.count());
    TSUNIT_ASSERT(buf.empty());
    TSUNIT_ASSERT(!buf.full());
    TSUNIT_EQUAL(memory >= total, buf.memoryResident());

    ts::TSPacket pkt;
    ts::TSPacketMetadata mdata;
    size_t in_label = 0;
    size_t out_label = 0;

    // Fill the buffer, return null packets.
    for (uint8_t i = 0; i < total; i++) {

        pkt.init(i, i, i);
        mdata.reset();
        mdata.setLabel(in_label);
        in_label = (in_label + 1) % ts::TSPacketLabelSet::SIZE;

        TSUNIT_EQUAL(184, pkt.getPayloadSize());
        TSUNIT_EQUAL(i, pkt.getPID());
        TSUNIT_EQUAL(i, *pkt.getPayload());
        TSUNIT_EQUAL(i, buf.count());
        TSUNIT_ASSERT(!buf.full());

        TSUNIT_ASSERT(buf.shift(pkt, mdata, CERR));

        TSUNIT_EQUAL(ts::PID_NULL, pkt.getPID());
        TSUNIT_ASSERT(mdata.getInputStuffing());
        TSUNIT_ASSERT(!mdata.hasAnyLabel());
    }
    TSUNIT_ASSERT(buf.full());

    // Actual time shift by 'total' packets.
    for (uint8_t i = total; i < 3 * total; i++) {

        pkt.init(i, i, i);
        mdata.reset();
        mdata.setLabel(in_label);
        in_label = (in_label + 1) % ts::TSPacketLabelSet::SIZE;

        TSUNIT_EQUAL(184, pkt.getPayloadSize());
        TSUNIT_EQUAL(i, pkt.getPID());
        TSUNIT_EQUAL(i, *pkt.getPayload());
        TSUNIT_EQUAL(total, buf.count());
        TSUNIT_ASSERT(buf.full());

        TSUNIT_ASSERT(buf.shift(pkt, mdata, CERR));

        TSUNIT_EQUAL(184, pkt.getPayloadSize());
        TSUNIT_EQUAL(i - total, pkt.getPID());
        TSUNIT_EQUAL(i - total, *pkt.getPayload());
        TSUNIT_ASSERT(!mdata.getInputStuffing());
        TSUNIT_ASSERT(mdata.hasAnyLabel());
        TSUNIT_ASSERT(mdata.hasLabel(out_label));
        out_label = (out_label + 1) % ts::TSPacketLabelSet::SIZE;
        TSUNIT_ASSERT(!mdata.hasLabel(out_label));
    }

    TSUNIT_ASSERT(buf.close(CERR));
    TSUNIT_ASSERT(!buf.isOpen());
}

void TimeShiftBufferTest::testMinimum()
{
    testCommon(2, 2);
}

void TimeShiftBufferTest::testMemory()
{
    testCommon(10, 16);
}

void TimeShiftBufferTest::testFile()
{
    testCommon(20, 4);
}
