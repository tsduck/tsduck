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
//  CppUnit test suite for various tables.
//
//----------------------------------------------------------------------------

#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsBAT.h"
#include "tsNIT.h"
#include "tsSDT.h"
#include "tsTOT.h"
#include "tsTSDT.h"
#include "tsEIT.h"
#include "tsAIT.h"
#include "tsCADescriptor.h"
#include "tsAVCVideoDescriptor.h"
#include "tsAC3Descriptor.h"
#include "tsEacemPreferredNameIdentifierDescriptor.h"
#include "tsLogicalChannelNumberDescriptor.h"
#include "tsEutelsatChannelNumberDescriptor.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TableTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testAssignPMT();
    void testCopyPMT();
    void testAIT();
    void testBAT();
    void testCAT();
    void testEIT();
    void testNIT();
    void testSDT();
    void testTOT();
    void testTSDT();
    void testCleanupPrivateDescriptors();

    CPPUNIT_TEST_SUITE(TableTest);
    CPPUNIT_TEST(testAssignPMT);
    CPPUNIT_TEST(testCopyPMT);
    CPPUNIT_TEST(testAIT);
    CPPUNIT_TEST(testBAT);
    CPPUNIT_TEST(testCAT);
    CPPUNIT_TEST(testEIT);
    CPPUNIT_TEST(testNIT);
    CPPUNIT_TEST(testSDT);
    CPPUNIT_TEST(testTOT);
    CPPUNIT_TEST(testTSDT);
    CPPUNIT_TEST(testCleanupPrivateDescriptors);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TableTest);

//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TableTest::setUp()
{
}

// Test suite cleanup method.
void TableTest::tearDown()
{
}

//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TableTest::testAssignPMT()
{
    ts::PMT pmt1(1, true, 27, 1001);
    pmt1.descs.add(ts::CADescriptor(0x1234, 2002));
    pmt1.streams[3003].stream_type = 45;
    pmt1.streams[3003].descs.add(ts::AVCVideoDescriptor());
    pmt1.streams[4004].stream_type = 149;
    pmt1.streams[4004].descs.add(ts::AC3Descriptor());
    pmt1.streams[4004].descs.add(ts::CADescriptor());

    const ts::PMT pmt2(pmt1);

    CPPUNIT_ASSERT(pmt2.isValid());
    CPPUNIT_ASSERT_EQUAL(uint8_t(ts::TID_PMT), pmt2.tableId());
    CPPUNIT_ASSERT(pmt2.is_current);
    CPPUNIT_ASSERT_EQUAL(uint8_t(1), pmt2.version);
    CPPUNIT_ASSERT_EQUAL(uint16_t(27), pmt2.service_id);
    CPPUNIT_ASSERT_EQUAL(ts::PID(1001), pmt2.pcr_pid);

    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_PMT), pmt2.descs.tableId());
    CPPUNIT_ASSERT(pmt2.descs.table() == &pmt2);
    CPPUNIT_ASSERT_EQUAL(size_t(1), pmt2.descs.count());
    CPPUNIT_ASSERT(pmt2.descs[0]->isValid());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_CA), pmt2.descs[0]->tag());

    CPPUNIT_ASSERT_EQUAL(size_t(2), pmt2.streams.size());

    CPPUNIT_ASSERT_EQUAL(uint8_t(45), pmt2.streams[3003].stream_type);
    CPPUNIT_ASSERT_EQUAL(size_t(1), pmt2.streams[3003].descs.count());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_AVC_VIDEO), pmt2.streams[3003].descs[0]->tag());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_PMT), pmt2.streams[3003].descs.tableId());
    CPPUNIT_ASSERT(pmt2.streams[3003].descs.table() == &pmt2);

    CPPUNIT_ASSERT_EQUAL(uint8_t(149), pmt2.streams[4004].stream_type);
    CPPUNIT_ASSERT_EQUAL(size_t(2), pmt2.streams[4004].descs.count());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_AC3), pmt2.streams[4004].descs[0]->tag());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_CA), pmt2.streams[4004].descs[1]->tag());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_PMT), pmt2.streams[4004].descs.tableId());
    CPPUNIT_ASSERT(pmt2.streams[4004].descs.table() == &pmt2);
}

void TableTest::testCopyPMT()
{
    ts::PMT pmt1(1, true, 27, 1001);
    pmt1.descs.add(ts::CADescriptor(0x1234, 2002));
    pmt1.streams[3003].stream_type = 45;
    pmt1.streams[3003].descs.add(ts::AVCVideoDescriptor());
    pmt1.streams[4004].stream_type = 149;
    pmt1.streams[4004].descs.add(ts::AC3Descriptor());
    pmt1.streams[4004].descs.add(ts::CADescriptor());

    ts::PMT pmt2;
    pmt2 = pmt1;

    CPPUNIT_ASSERT(pmt2.isValid());
    CPPUNIT_ASSERT_EQUAL(uint8_t(ts::TID_PMT), pmt2.tableId());
    CPPUNIT_ASSERT(pmt2.is_current);
    CPPUNIT_ASSERT_EQUAL(uint8_t(1), pmt2.version);
    CPPUNIT_ASSERT_EQUAL(uint16_t(27), pmt2.service_id);
    CPPUNIT_ASSERT_EQUAL(ts::PID(1001), pmt2.pcr_pid);

    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_PMT), pmt2.descs.tableId());
    CPPUNIT_ASSERT(pmt2.descs.table() == &pmt2);
    CPPUNIT_ASSERT_EQUAL(size_t(1), pmt2.descs.count());
    CPPUNIT_ASSERT(pmt2.descs[0]->isValid());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_CA), pmt2.descs[0]->tag());

    CPPUNIT_ASSERT_EQUAL(size_t(2), pmt2.streams.size());

    CPPUNIT_ASSERT_EQUAL(uint8_t(45), pmt2.streams[3003].stream_type);
    CPPUNIT_ASSERT_EQUAL(size_t(1), pmt2.streams[3003].descs.count());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_AVC_VIDEO), pmt2.streams[3003].descs[0]->tag());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_PMT), pmt2.streams[3003].descs.tableId());
    CPPUNIT_ASSERT(pmt2.streams[3003].descs.table() == &pmt2);

    CPPUNIT_ASSERT_EQUAL(uint8_t(149), pmt2.streams[4004].stream_type);
    CPPUNIT_ASSERT_EQUAL(size_t(2), pmt2.streams[4004].descs.count());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_AC3), pmt2.streams[4004].descs[0]->tag());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_CA), pmt2.streams[4004].descs[1]->tag());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_PMT), pmt2.streams[4004].descs.tableId());
    CPPUNIT_ASSERT(pmt2.streams[4004].descs.table() == &pmt2);
}

void TableTest::testAIT()
{
    ts::ApplicationIdentifier id;
    ts::AIT ait1;
    ait1.applications[id].descs.add(ts::CADescriptor());
    CPPUNIT_ASSERT_EQUAL(size_t(1), ait1.applications.size());
    CPPUNIT_ASSERT(ait1.applications.begin()->first == id);
    CPPUNIT_ASSERT(ait1.applications.begin()->second.descs.table() == &ait1);

    ts::AIT ait2(ait1);
    CPPUNIT_ASSERT_EQUAL(size_t(1), ait2.applications.size());
    CPPUNIT_ASSERT(ait2.applications.begin()->first == id);
    CPPUNIT_ASSERT(ait2.applications.begin()->second.descs.table() == &ait2);

    ts::AIT ait3;
    ait3 = ait1;
    CPPUNIT_ASSERT_EQUAL(size_t(1), ait3.applications.size());
    CPPUNIT_ASSERT(ait3.applications.begin()->first == id);
    CPPUNIT_ASSERT(ait3.applications.begin()->second.descs.table() == &ait3);
}

void TableTest::testBAT()
{
    ts::BAT bat1;
    bat1.transports[ts::TransportStreamId(1, 2)].descs.add(ts::CADescriptor());
    CPPUNIT_ASSERT(bat1.descs.table() == &bat1);
    CPPUNIT_ASSERT_EQUAL(size_t(1), bat1.transports.size());
    CPPUNIT_ASSERT(bat1.transports.begin()->first == ts::TransportStreamId(1, 2));
    CPPUNIT_ASSERT(bat1.transports.begin()->second.descs.table() == &bat1);

    ts::BAT bat2(bat1);
    CPPUNIT_ASSERT(bat2.descs.table() == &bat2);
    CPPUNIT_ASSERT_EQUAL(size_t(1), bat2.transports.size());
    CPPUNIT_ASSERT(bat2.transports.begin()->first == ts::TransportStreamId(1, 2));
    CPPUNIT_ASSERT(bat2.transports.begin()->second.descs.table() == &bat2);

    ts::BAT bat3;
    bat3 = bat1;
    CPPUNIT_ASSERT(bat3.descs.table() == &bat3);
    CPPUNIT_ASSERT_EQUAL(size_t(1), bat3.transports.size());
    CPPUNIT_ASSERT(bat3.transports.begin()->first == ts::TransportStreamId(1, 2));
    CPPUNIT_ASSERT(bat3.transports.begin()->second.descs.table() == &bat3);
}

void TableTest::testCAT()
{
    ts::CAT cat1;
    CPPUNIT_ASSERT(cat1.descs.table() == &cat1);

    ts::CAT cat2(cat1);
    CPPUNIT_ASSERT(cat2.descs.table() == &cat2);

    ts::CAT cat3;
    cat3 = cat1;
    CPPUNIT_ASSERT(cat3.descs.table() == &cat3);
}

void TableTest::testEIT()
{
    ts::EIT eit1;
    eit1.events[1].descs.add(ts::CADescriptor());
    CPPUNIT_ASSERT_EQUAL(size_t(1), eit1.events.size());
    CPPUNIT_ASSERT(eit1.events.begin()->first == 1);
    CPPUNIT_ASSERT(eit1.events.begin()->second.descs.table() == &eit1);

    ts::EIT eit2(eit1);
    CPPUNIT_ASSERT_EQUAL(size_t(1), eit2.events.size());
    CPPUNIT_ASSERT(eit2.events.begin()->first == 1);
    CPPUNIT_ASSERT(eit2.events.begin()->second.descs.table() == &eit2);

    ts::EIT eit3;
    eit3 = eit1;
    CPPUNIT_ASSERT_EQUAL(size_t(1), eit3.events.size());
    CPPUNIT_ASSERT(eit3.events.begin()->first == 1);
    CPPUNIT_ASSERT(eit3.events.begin()->second.descs.table() == &eit3);
}

void TableTest::testNIT()
{
    ts::NIT nit1;
    nit1.transports[ts::TransportStreamId(1, 2)].descs.add(ts::CADescriptor());
    CPPUNIT_ASSERT(nit1.descs.table() == &nit1);
    CPPUNIT_ASSERT_EQUAL(size_t(1), nit1.transports.size());
    CPPUNIT_ASSERT(nit1.transports.begin()->first == ts::TransportStreamId(1, 2));
    CPPUNIT_ASSERT(nit1.transports.begin()->second.descs.table() == &nit1);

    ts::NIT nit2(nit1);
    CPPUNIT_ASSERT(nit2.descs.table() == &nit2);
    CPPUNIT_ASSERT_EQUAL(size_t(1), nit2.transports.size());
    CPPUNIT_ASSERT(nit2.transports.begin()->first == ts::TransportStreamId(1, 2));
    CPPUNIT_ASSERT(nit2.transports.begin()->second.descs.table() == &nit2);

    ts::NIT nit3;
    nit3 = nit1;
    CPPUNIT_ASSERT(nit3.descs.table() == &nit3);
    CPPUNIT_ASSERT_EQUAL(size_t(1), nit3.transports.size());
    CPPUNIT_ASSERT(nit3.transports.begin()->first == ts::TransportStreamId(1, 2));
    CPPUNIT_ASSERT(nit3.transports.begin()->second.descs.table() == &nit3);
}

void TableTest::testSDT()
{
    ts::SDT sdt1;
    sdt1.services[1].descs.add(ts::CADescriptor());
    CPPUNIT_ASSERT_EQUAL(size_t(1), sdt1.services.size());
    CPPUNIT_ASSERT(sdt1.services.begin()->first == 1);
    CPPUNIT_ASSERT(sdt1.services.begin()->second.descs.table() == &sdt1);

    ts::SDT sdt2(sdt1);
    CPPUNIT_ASSERT_EQUAL(size_t(1), sdt2.services.size());
    CPPUNIT_ASSERT(sdt2.services.begin()->first == 1);
    CPPUNIT_ASSERT(sdt2.services.begin()->second.descs.table() == &sdt2);

    ts::SDT sdt3;
    sdt3 = sdt1;
    CPPUNIT_ASSERT_EQUAL(size_t(1), sdt3.services.size());
    CPPUNIT_ASSERT(sdt3.services.begin()->first == 1);
    CPPUNIT_ASSERT(sdt3.services.begin()->second.descs.table() == &sdt3);
}

void TableTest::testTOT()
{
    ts::TOT tot1;
    tot1.descs.add(ts::CADescriptor());
    CPPUNIT_ASSERT(tot1.descs.table() == &tot1);
    CPPUNIT_ASSERT_EQUAL(size_t(1), tot1.descs.count());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_CA), tot1.descs[0]->tag());

    ts::TOT tot2(tot1);
    CPPUNIT_ASSERT(tot2.descs.table() == &tot2);
    CPPUNIT_ASSERT_EQUAL(size_t(1), tot2.descs.count());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_CA), tot2.descs[0]->tag());

    ts::TOT tot3;
    tot3 = tot1;
    CPPUNIT_ASSERT(tot3.descs.table() == &tot3);
    CPPUNIT_ASSERT_EQUAL(size_t(1), tot2.descs.count());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_CA), tot2.descs[0]->tag());
}

void TableTest::testTSDT()
{
    ts::TSDT tsdt1;
    CPPUNIT_ASSERT(tsdt1.descs.table() == &tsdt1);

    ts::TSDT tsdt2(tsdt1);
    CPPUNIT_ASSERT(tsdt2.descs.table() == &tsdt2);

    ts::TSDT tsdt3;
    tsdt3 = tsdt1;
    CPPUNIT_ASSERT(tsdt3.descs.table() == &tsdt3);
}

void TableTest::testCleanupPrivateDescriptors()
{
    // Issue #87 non-regression.
    ts::DescriptorList dlist(nullptr);
    dlist.add(ts::EacemPreferredNameIdentifierDescriptor());
    dlist.add(ts::LogicalChannelNumberDescriptor());
    dlist.add(ts::ServiceDescriptor());
    dlist.add(ts::EutelsatChannelNumberDescriptor());

    CPPUNIT_ASSERT_EQUAL(size_t(4), dlist.count());
    dlist.removeInvalidPrivateDescriptors();
    CPPUNIT_ASSERT_EQUAL(size_t(1), dlist.count());
    CPPUNIT_ASSERT_EQUAL(ts::DID(ts::DID_SERVICE), dlist[0]->tag());
}
