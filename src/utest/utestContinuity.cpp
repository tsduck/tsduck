//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(Analyze);
    TSUNIT_DECLARE_TEST(Fix);
};

TSUNIT_REGISTER(ContinuityTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Analyze)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    ts::ContinuityAnalyzer fixer(ts::AllPIDs(), &log);

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
    TSUNIT_ASSERT(log.empty());
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(u"foo: packet index: 7, PID: 0x0065 (101), missing 2 packets", log.messages());
    log.clear();

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(log.empty());
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(u"foo: packet index: 10, PID: 0x0065 (101), 3 duplicate packets", log.messages());
    log.clear();

    pkt.setPID(101); pkt.setCC(5);
    TSUNIT_ASSERT(fixer.feedPacket(pkt));

    TSUNIT_EQUAL(12, fixer.totalPackets());
    TSUNIT_EQUAL(12, fixer.processedPackets());
    TSUNIT_EQUAL(2, fixer.errorCount());
    TSUNIT_EQUAL(0, fixer.fixCount());
}

TSUNIT_DEFINE_TEST(Fix)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    ts::ContinuityAnalyzer fixer(ts::AllPIDs(), &log);

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
    TSUNIT_ASSERT(log.empty());
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(u"bar: packet index: 7, PID: 0x0065 (101), missing 2 packets", log.messages());
    TSUNIT_EQUAL(1, pkt.getCC());
    log.clear();

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(2, pkt.getCC());
    TSUNIT_ASSERT(log.empty());

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(2, pkt.getCC());
    TSUNIT_ASSERT(log.empty());

    pkt.setPID(101); pkt.setCC(4);
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_EQUAL(u"bar: packet index: 10, PID: 0x0065 (101), 3 duplicate packets", log.messages());
    TSUNIT_EQUAL(2, pkt.getCC());
    log.clear();

    pkt.setPID(101); pkt.setCC(5);
    TSUNIT_ASSERT(!fixer.feedPacket(pkt));
    TSUNIT_ASSERT(log.empty());
    TSUNIT_EQUAL(3, pkt.getCC());

    TSUNIT_EQUAL(12, fixer.totalPackets());
    TSUNIT_EQUAL(12, fixer.processedPackets());
    TSUNIT_EQUAL(2, fixer.errorCount());
    TSUNIT_EQUAL(5, fixer.fixCount());
}
