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
//  CppUnit test suite for subclasses of ts::ContinuityAnalyzer
//
//----------------------------------------------------------------------------

#include "tsContinuityAnalyzer.h"
#include "tsReportBuffer.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ContinuityTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testAnalyze();
    void testFix();

    CPPUNIT_TEST_SUITE(ContinuityTest);
    CPPUNIT_TEST(testAnalyze);
    CPPUNIT_TEST(testFix);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ContinuityTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

void ContinuityTest::setUp()
{
}

void ContinuityTest::tearDown()
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
    CPPUNIT_ASSERT(pkt.hasPayload());

    pkt.setPID(100); pkt.setCC(5);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(13);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(100); pkt.setCC(6);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(14);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(14);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(15);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(0);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(3);
    CPPUNIT_ASSERT(log.emptyMessages());
    CPPUNIT_ASSERT(!fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"foo: packet: 7, PID: 0x0065, missing:  2 packets", log.getMessages());
    log.resetMessages();

    pkt.setPID(101); pkt.setCC(4);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(4);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));

    pkt.setPID(101); pkt.setCC(4);
    CPPUNIT_ASSERT(log.emptyMessages());
    CPPUNIT_ASSERT(!fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"foo: packet: 10, PID: 0x0065, 3 duplicate packets", log.getMessages());
    log.resetMessages();

    pkt.setPID(101); pkt.setCC(5);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));

    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(12), fixer.totalPackets());
    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(12), fixer.processedPackets());
    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(2), fixer.errorCount());
    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(0), fixer.fixCount());
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
    CPPUNIT_ASSERT(pkt.hasPayload());

    pkt.setPID(100); pkt.setCC(5);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_EQUAL(uint8_t(5), pkt.getCC());

    pkt.setPID(101); pkt.setCC(13);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_EQUAL(uint8_t(13), pkt.getCC());

    pkt.setPID(100); pkt.setCC(6);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_EQUAL(uint8_t(6), pkt.getCC());

    pkt.setPID(101); pkt.setCC(14);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_EQUAL(uint8_t(14), pkt.getCC());

    pkt.setPID(101); pkt.setCC(14);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_EQUAL(uint8_t(14), pkt.getCC());

    pkt.setPID(101); pkt.setCC(15);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_EQUAL(uint8_t(15), pkt.getCC());

    pkt.setPID(101); pkt.setCC(0);
    CPPUNIT_ASSERT(fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_EQUAL(uint8_t(0), pkt.getCC());

    pkt.setPID(101); pkt.setCC(3);
    CPPUNIT_ASSERT(log.emptyMessages());
    CPPUNIT_ASSERT(!fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"bar: packet: 7, PID: 0x0065, missing:  2 packets", log.getMessages());
    CPPUNIT_ASSERT_EQUAL(uint8_t(1), pkt.getCC());
    log.resetMessages();

    pkt.setPID(101); pkt.setCC(4);
    CPPUNIT_ASSERT(!fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_EQUAL(uint8_t(2), pkt.getCC());
    CPPUNIT_ASSERT(log.emptyMessages());

    pkt.setPID(101); pkt.setCC(4);
    CPPUNIT_ASSERT(!fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_EQUAL(uint8_t(2), pkt.getCC());
    CPPUNIT_ASSERT(log.emptyMessages());

    pkt.setPID(101); pkt.setCC(4);
    CPPUNIT_ASSERT(!fixer.feedPacket(pkt));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"bar: packet: 10, PID: 0x0065, 3 duplicate packets", log.getMessages());
    CPPUNIT_ASSERT_EQUAL(uint8_t(2), pkt.getCC());
    log.resetMessages();

    pkt.setPID(101); pkt.setCC(5);
    CPPUNIT_ASSERT(!fixer.feedPacket(pkt));
    CPPUNIT_ASSERT(log.emptyMessages());
    CPPUNIT_ASSERT_EQUAL(uint8_t(3), pkt.getCC());

    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(12), fixer.totalPackets());
    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(12), fixer.processedPackets());
    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(2), fixer.errorCount());
    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(5), fixer.fixCount());
}
