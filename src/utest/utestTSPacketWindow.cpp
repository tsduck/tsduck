//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//  TSUnit test suite for class ts::TSPacketWindow
//
//----------------------------------------------------------------------------

#include "tsTSPacketWindow.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TSPacketWindowTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testAll();

    TSUNIT_TEST_BEGIN(TSPacketWindowTest);
    TSUNIT_TEST(testAll);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(TSPacketWindowTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TSPacketWindowTest::beforeTest()
{
}

// Test suite cleanup method.
void TSPacketWindowTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TSPacketWindowTest::testAll()
{
    // Use a contiguous buffer but with ranges to exercise TSPacketWindow.
    ts::TSPacket packets[10];
    ts::TSPacketMetadata mdata[10];
    for (size_t i = 0; i < 10; ++i) {
        packets[i].init(ts::PID(100 + i));
    }


    ts::TSPacketWindow win({
        {&packets[0], &mdata[0], 2},
        {&packets[2], &mdata[2], 4},
        {&packets[6], &mdata[6], 1},
        {&packets[7], &mdata[7], 3},
    });

    TSUNIT_EQUAL(10, win.size());
    TSUNIT_EQUAL(0, win.nullifyCount());
    TSUNIT_EQUAL(0, win.dropCount());

    // Sequential access.
    TSUNIT_ASSERT(win.packet(0) == &packets[0]);
    TSUNIT_ASSERT(win.metadata(0) == &mdata[0]);
    TSUNIT_ASSERT(win.packet(1) == &packets[1]);
    TSUNIT_ASSERT(win.metadata(1) == &mdata[1]);
    TSUNIT_ASSERT(win.packet(2) == &packets[2]);
    TSUNIT_ASSERT(win.metadata(2) == &mdata[2]);
    TSUNIT_ASSERT(win.packet(3) == &packets[3]);
    TSUNIT_ASSERT(win.metadata(3) == &mdata[3]);
    TSUNIT_ASSERT(win.packet(4) == &packets[4]);
    TSUNIT_ASSERT(win.metadata(4) == &mdata[4]);
    TSUNIT_ASSERT(win.packet(5) == &packets[5]);
    TSUNIT_ASSERT(win.metadata(5) == &mdata[5]);
    TSUNIT_ASSERT(win.packet(6) == &packets[6]);
    TSUNIT_ASSERT(win.metadata(6) == &mdata[6]);
    TSUNIT_ASSERT(win.packet(7) == &packets[7]);
    TSUNIT_ASSERT(win.metadata(7) == &mdata[7]);
    TSUNIT_ASSERT(win.packet(8) == &packets[8]);
    TSUNIT_ASSERT(win.metadata(8) == &mdata[8]);
    TSUNIT_ASSERT(win.packet(9) == &packets[9]);
    TSUNIT_ASSERT(win.metadata(9) == &mdata[9]);

    // Random access.
    TSUNIT_ASSERT(win.packet(2) == &packets[2]);
    TSUNIT_ASSERT(win.metadata(8) == &mdata[8]);
    TSUNIT_ASSERT(win.packet(1) == &packets[1]);
    TSUNIT_ASSERT(win.metadata(9) == &mdata[9]);
    TSUNIT_ASSERT(win.packet(4) == &packets[4]);
    TSUNIT_ASSERT(win.packet(10) == nullptr);
    TSUNIT_ASSERT(win.packet(7) == &packets[7]);
    TSUNIT_ASSERT(win.metadata(1) == &mdata[1]);
    TSUNIT_ASSERT(win.packet(0) == &packets[0]);
    TSUNIT_ASSERT(win.metadata(11) == nullptr);
    TSUNIT_ASSERT(win.metadata(5) == &mdata[5]);

    TSUNIT_EQUAL(104, packets[4].getPID());
    win.nullify(4);
    TSUNIT_EQUAL(ts::PID_NULL, packets[4].getPID());
    TSUNIT_EQUAL(1, win.nullifyCount());
    win.nullify(4);
    win.nullify(4);
    TSUNIT_EQUAL(1, win.nullifyCount());

    TSUNIT_EQUAL(108, packets[8].getPID());
    win.nullify(8);
    TSUNIT_EQUAL(2, win.nullifyCount());
    TSUNIT_EQUAL(ts::PID_NULL, packets[8].getPID());

    TSUNIT_EQUAL(ts::SYNC_BYTE, packets[7].b[0]);
    win.drop(7);
    TSUNIT_EQUAL(0, packets[7].b[0]);
    TSUNIT_EQUAL(1, win.dropCount());
    win.drop(7);
    win.drop(7);
    win.drop(7);
    TSUNIT_EQUAL(1, win.dropCount());

    TSUNIT_EQUAL(ts::SYNC_BYTE, packets[1].b[0]);
    TSUNIT_EQUAL(ts::SYNC_BYTE, packets[9].b[0]);
    win.drop(1);
    win.drop(9);
    TSUNIT_EQUAL(0, packets[1].b[0]);
    TSUNIT_EQUAL(0, packets[9].b[0]);
    TSUNIT_EQUAL(3, win.dropCount());

    TSUNIT_EQUAL(0, win.packetIndexInBuffer(0, packets, 10));
    TSUNIT_EQUAL(ts::NPOS, win.packetIndexInBuffer(0, packets + 1, 9));
    TSUNIT_EQUAL(9, win.packetIndexInBuffer(9, packets, 10));
    TSUNIT_EQUAL(7, win.packetIndexInBuffer(7, packets, 10));
    TSUNIT_EQUAL(ts::NPOS, win.packetIndexInBuffer(11, packets, 9));
}
