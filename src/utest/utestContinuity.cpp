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
//  TSUnit test suite for subclasses of ts::ContinuityAnalyzer
//
//----------------------------------------------------------------------------

#include "tsContinuityAnalyzer.h"
#include "tsReportBuffer.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ContinuityTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testAnalyze();
    void testFix();

    TSUNIT_TEST_BEGIN(ContinuityTest);
    TSUNIT_TEST(testAnalyze);
    TSUNIT_TEST(testFix);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(ContinuityTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

void ContinuityTest::beforeTest()
{
}

void ContinuityTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void ContinuityTest::testAnalyze()
{
    ts::ReportBuffer<> log;
    ts::ContinuityAnalyzer fixer(ts::AllPIDs, &log);

    fixer.setDisplay(true);
    fixer.setMessagePrefix(u"foo: ");

    // Scenario: PID CC
    //        0: 100  5
    //        1: 101 13
    //        2; 100  6
    //        3: 101 14
    //        4: 101 14 <- duplicate OK
    //        5: 101 15
    //        6: 101  0
    //        7: 101  3 <- discontinuity
    //        8: 101  4
    //        9: 101  4
    //       10: 101  4 <- too many duplicates
    //       11: 101  5

    ts::TSPacket pkt(ts::NullPacket);
    TSUNIT_ASSERT(pkt.hasPayload());

    pkt.setPID(100); pkt.setCC(5);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(13);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(100); pkt.setCC(6);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(14);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(14);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(15);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(0);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(3);
    TSUNIT_ASSERT(log.emptyMessages());
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(u"foo: packet index: 7, PID: 0x0065, missing 2 packets", log.getMessages());
    log.resetMessages();

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(log.emptyMessages());
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(u"foo: packet index: 10, PID: 0x0065, 3 duplicate packets", log.getMessages());
    log.resetMessages();

    pkt.setPID(101); pkt.setCC(5);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    TSUNIT_EQUAL(12, fixer.totalPackets());
    TSUNIT_EQUAL(12, fixer.processedPackets());
    TSUNIT_EQUAL(2, fixer.errorCount());
    TSUNIT_EQUAL(0, fixer.fixCount());
}

void ContinuityTest::testFix()
{
    ts::ReportBuffer<> log;
    ts::ContinuityAnalyzer fixer(ts::AllPIDs, &log);

    fixer.setDisplay(true);
    fixer.setFix(true);
    fixer.setMessagePrefix(u"bar: ");

    // Scenario: PID CC
    //        0: 100  5
    //        1: 101 13
    //        2; 100  6
    //        3: 101 14
    //        4: 101 14 <- duplicate OK
    //        5: 101 15
    //        6: 101  0
    //        7: 101  3 -> 1 <- discontinuity
    //        8: 101  4 -> 2
    //        9: 101  4 -> 2
    //       10: 101  4 -> 2 <- too many duplicates
    //       11: 101  5 -> 3

    ts::TSPacket pkt(ts::NullPacket);
    TSUNIT_ASSERT(pkt.hasPayload());

    pkt.setPID(100); pkt.setCC(5);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));
    TSUNIT_EQUAL(5, pkt.getCC());

    pkt.setPID(101); pkt.setCC(13);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));
    TSUNIT_EQUAL(13, pkt.getCC());

    pkt.setPID(100); pkt.setCC(6);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));
    TSUNIT_EQUAL(6, pkt.getCC());

    pkt.setPID(101); pkt.setCC(14);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));
    TSUNIT_EQUAL(14, pkt.getCC());

    pkt.setPID(101); pkt.setCC(14);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));
    TSUNIT_EQUAL(14, pkt.getCC());

    pkt.setPID(101); pkt.setCC(15);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));
    TSUNIT_EQUAL(15, pkt.getCC());

    pkt.setPID(101); pkt.setCC(0);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));
    TSUNIT_EQUAL(0, pkt.getCC());

    pkt.setPID(101); pkt.setCC(3);
    TSUNIT_ASSERT(log.emptyMessages());
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(u"bar: packet index: 7, PID: 0x0065, missing 2 packets", log.getMessages());
    TSUNIT_EQUAL(1, pkt.getCC());
    log.resetMessages();

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(2, pkt.getCC());
    TSUNIT_ASSERT(log.emptyMessages());

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(2, pkt.getCC());
    TSUNIT_ASSERT(log.emptyMessages());

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(u"bar: packet index: 10, PID: 0x0065, 3 duplicate packets", log.getMessages());
    TSUNIT_EQUAL(2, pkt.getCC());
    log.resetMessages();

    pkt.setPID(101); pkt.setCC(5);
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_ASSERT(log.emptyMessages());
    TSUNIT_EQUAL(3, pkt.getCC());

    TSUNIT_EQUAL(12, fixer.totalPackets());
    TSUNIT_EQUAL(12, fixer.processedPackets());
    TSUNIT_EQUAL(2, fixer.errorCount());
    TSUNIT_EQUAL(5, fixer.fixCount());
}
