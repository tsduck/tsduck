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

#include "tsTables.h"
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

    CPPUNIT_TEST_SUITE(TableTest);
    CPPUNIT_TEST(testAssignPMT);
    CPPUNIT_TEST(testCopyPMT);
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
