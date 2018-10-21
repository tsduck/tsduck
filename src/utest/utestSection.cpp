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
//  CppUnit test suite for class ts::Section
//
//----------------------------------------------------------------------------

#include "tsSection.h"
#include "tsBinaryTable.h"
#include "tsNames.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;

#include "tables/psi_tot_tnt_sections.h"
#include "tables/psi_bat_tvnum_sections.h"
#include "tables/psi_nit_tntv23_sections.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SectionTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testTOT();
    void testBAT();
    void testNIT();
    void testReload();
    void testAssign();
    void testPackSections();
    void testSize();

    CPPUNIT_TEST_SUITE(SectionTest);
    CPPUNIT_TEST(testTOT);
    CPPUNIT_TEST(testBAT);
    CPPUNIT_TEST(testNIT);
    CPPUNIT_TEST(testReload);
    CPPUNIT_TEST(testAssign);
    CPPUNIT_TEST(testPackSections);
    CPPUNIT_TEST(testSize);
    CPPUNIT_TEST_SUITE_END();

private:
    // Create a dummy long section.
    static ts::SectionPtr NewSection(size_t size, uint8_t secnum = 0, ts::TID tid = 0xEE);
};

CPPUNIT_TEST_SUITE_REGISTRATION(SectionTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void SectionTest::setUp()
{
}

// Test suite cleanup method.
void SectionTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void SectionTest::testTOT()
{
    ts::Section sec(psi_tot_tnt_sections, sizeof(psi_tot_tnt_sections), ts::PID_TOT, ts::CRC32::CHECK);

    CPPUNIT_ASSERT(sec.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_TOT), sec.tableId());
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_TOT), sec.sourcePID());
    CPPUNIT_ASSERT(!sec.isLongSection());
}

void SectionTest::testBAT()
{
    ts::Section sec(psi_bat_tvnum_sections, sizeof(psi_bat_tvnum_sections), ts::PID_BAT, ts::CRC32::CHECK);

    CPPUNIT_ASSERT(sec.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_BAT), sec.tableId());
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_BAT), sec.sourcePID());
    CPPUNIT_ASSERT(sec.isLongSection());
}

void SectionTest::testNIT()
{
    ts::Section sec(psi_nit_tntv23_sections, sizeof(psi_nit_tntv23_sections), ts::PID_NIT, ts::CRC32::CHECK);

    CPPUNIT_ASSERT(sec.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_NIT_ACT), sec.tableId());
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_NIT), sec.sourcePID());
    CPPUNIT_ASSERT(sec.isLongSection());
}

void SectionTest::testReload()
{
    ts::Section sec(psi_tot_tnt_sections, sizeof(psi_tot_tnt_sections), ts::PID_TOT, ts::CRC32::CHECK);

    CPPUNIT_ASSERT(sec.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_TOT), sec.tableId());
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_TOT), sec.sourcePID());
    CPPUNIT_ASSERT(!sec.isLongSection());

    sec.reload(psi_bat_tvnum_sections, sizeof(psi_bat_tvnum_sections), ts::PID_BAT, ts::CRC32::CHECK);

    CPPUNIT_ASSERT(sec.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_BAT), sec.tableId());
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_BAT), sec.sourcePID());
    CPPUNIT_ASSERT(sec.isLongSection());
}

void SectionTest::testAssign()
{
    ts::Section sec(psi_tot_tnt_sections, sizeof(psi_tot_tnt_sections), ts::PID_TOT, ts::CRC32::CHECK);

    CPPUNIT_ASSERT(sec.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_TOT), sec.tableId());
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_TOT), sec.sourcePID());
    CPPUNIT_ASSERT(!sec.isLongSection());

    ts::Section sec2(psi_nit_tntv23_sections, sizeof(psi_nit_tntv23_sections), ts::PID_NIT, ts::CRC32::CHECK);

    CPPUNIT_ASSERT(sec2.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_NIT_ACT), sec2.tableId());
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_NIT), sec2.sourcePID());
    CPPUNIT_ASSERT(sec2.isLongSection());

    sec = sec2;

    CPPUNIT_ASSERT(sec.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(ts::TID_NIT_ACT), sec.tableId());
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_NIT), sec.sourcePID());
    CPPUNIT_ASSERT(sec.isLongSection());
}

void SectionTest::testPackSections()
{
    ts::BinaryTable table;
    CPPUNIT_ASSERT(!table.isValid());

    static const uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7};

    table.addSection(new ts::Section(150, true, 102, 12, true, 3, 7, data + 1, 2, 2000));
    CPPUNIT_ASSERT(!table.isValid());

    table.addSection(new ts::Section(150, true, 102, 12, true, 5, 7, data + 3, 3, 2000));
    CPPUNIT_ASSERT(!table.isValid());

    table.addSection(new ts::Section(150, true, 102, 12, true, 6, 7, data + 4, 4, 2000));
    CPPUNIT_ASSERT(!table.isValid());

    table.packSections();

    CPPUNIT_ASSERT(table.isValid());
    CPPUNIT_ASSERT(!table.isShortSection());
    CPPUNIT_ASSERT_EQUAL(ts::TID(150), table.tableId());
    CPPUNIT_ASSERT_EQUAL(uint16_t(102), table.tableIdExtension());
    CPPUNIT_ASSERT_EQUAL(uint8_t(12), table.version());
    CPPUNIT_ASSERT_EQUAL(ts::PID(2000), table.sourcePID());
    CPPUNIT_ASSERT_EQUAL(size_t(3), table.sectionCount());

    ts::SectionPtr sec(table.sectionAt(0));
    CPPUNIT_ASSERT(!sec.isNull());
    CPPUNIT_ASSERT(sec->isValid());
    CPPUNIT_ASSERT(sec->isLongSection());
    CPPUNIT_ASSERT(!sec->isShortSection());
    CPPUNIT_ASSERT(sec->isPrivateSection());
    CPPUNIT_ASSERT(sec->isCurrent());
    CPPUNIT_ASSERT(!sec->isNext());
    CPPUNIT_ASSERT_EQUAL(ts::TID(150), sec->tableId());
    CPPUNIT_ASSERT_EQUAL(uint16_t(102), sec->tableIdExtension());
    CPPUNIT_ASSERT_EQUAL(uint8_t(12), sec->version());
    CPPUNIT_ASSERT_EQUAL(ts::PID(2000), sec->sourcePID());
    CPPUNIT_ASSERT_EQUAL(uint8_t(0), sec->sectionNumber());
    CPPUNIT_ASSERT_EQUAL(uint8_t(2), sec->lastSectionNumber());
    CPPUNIT_ASSERT_EQUAL(size_t(2), sec->payloadSize());
    CPPUNIT_ASSERT(sec->payload() != nullptr);
    CPPUNIT_ASSERT_EQUAL(uint8_t(1), *sec->payload());

    sec = table.sectionAt(1);
    CPPUNIT_ASSERT(!sec.isNull());
    CPPUNIT_ASSERT(sec->isValid());
    CPPUNIT_ASSERT(sec->isLongSection());
    CPPUNIT_ASSERT(!sec->isShortSection());
    CPPUNIT_ASSERT(sec->isPrivateSection());
    CPPUNIT_ASSERT(sec->isCurrent());
    CPPUNIT_ASSERT(!sec->isNext());
    CPPUNIT_ASSERT_EQUAL(ts::TID(150), sec->tableId());
    CPPUNIT_ASSERT_EQUAL(uint16_t(102), sec->tableIdExtension());
    CPPUNIT_ASSERT_EQUAL(uint8_t(12), sec->version());
    CPPUNIT_ASSERT_EQUAL(ts::PID(2000), sec->sourcePID());
    CPPUNIT_ASSERT_EQUAL(uint8_t(1), sec->sectionNumber());
    CPPUNIT_ASSERT_EQUAL(uint8_t(2), sec->lastSectionNumber());
    CPPUNIT_ASSERT_EQUAL(size_t(3), sec->payloadSize());
    CPPUNIT_ASSERT(sec->payload() != nullptr);
    CPPUNIT_ASSERT_EQUAL(uint8_t(3), *sec->payload());

    sec = table.sectionAt(2);
    CPPUNIT_ASSERT(!sec.isNull());
    CPPUNIT_ASSERT(sec->isValid());
    CPPUNIT_ASSERT(sec->isLongSection());
    CPPUNIT_ASSERT(!sec->isShortSection());
    CPPUNIT_ASSERT(sec->isPrivateSection());
    CPPUNIT_ASSERT(sec->isCurrent());
    CPPUNIT_ASSERT(!sec->isNext());
    CPPUNIT_ASSERT_EQUAL(ts::TID(150), sec->tableId());
    CPPUNIT_ASSERT_EQUAL(uint16_t(102), sec->tableIdExtension());
    CPPUNIT_ASSERT_EQUAL(uint8_t(12), sec->version());
    CPPUNIT_ASSERT_EQUAL(ts::PID(2000), sec->sourcePID());
    CPPUNIT_ASSERT_EQUAL(uint8_t(2), sec->sectionNumber());
    CPPUNIT_ASSERT_EQUAL(uint8_t(2), sec->lastSectionNumber());
    CPPUNIT_ASSERT_EQUAL(size_t(4), sec->payloadSize());
    CPPUNIT_ASSERT(sec->payload() != nullptr);
    CPPUNIT_ASSERT_EQUAL(uint8_t(4), *sec->payload());
}

// Create a dummy long section.
ts::SectionPtr SectionTest::NewSection(size_t size, uint8_t secnum, ts::TID tid)
{
    const size_t overhead = ts::LONG_SECTION_HEADER_SIZE + ts::SECTION_CRC32_SIZE;
    ts::ByteBlock payload(size >= overhead ? size - overhead : 0);
    return new ts::Section(tid, true, 0x0000, 0, true, secnum, secnum, payload.data(), payload.size());
}

void SectionTest::testSize()
{
    ts::BinaryTable table;
    table.addSection(NewSection(183, 0));
    CPPUNIT_ASSERT(table.isValid());
    CPPUNIT_ASSERT_EQUAL(size_t(1), table.sectionCount());
    CPPUNIT_ASSERT_EQUAL(size_t(183), table.sectionAt(0)->size());
    CPPUNIT_ASSERT_EQUAL(size_t(183), table.totalSize());
    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(1), table.packetCount());

    table.clear();
    table.addSection(NewSection(184, 0));
    CPPUNIT_ASSERT(table.isValid());
    CPPUNIT_ASSERT_EQUAL(size_t(1), table.sectionCount());
    CPPUNIT_ASSERT_EQUAL(size_t(184), table.sectionAt(0)->size());
    CPPUNIT_ASSERT_EQUAL(size_t(184), table.totalSize());
    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(2), table.packetCount());

    table.addSection(NewSection(182, 1));
    CPPUNIT_ASSERT(table.isValid());
    CPPUNIT_ASSERT_EQUAL(size_t(2), table.sectionCount());
    CPPUNIT_ASSERT_EQUAL(size_t(182), table.sectionAt(1)->size());
    CPPUNIT_ASSERT_EQUAL(size_t(366), table.totalSize());
    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(2), table.packetCount());

    table.clear();
    table.addSection(NewSection(184, 0));
    table.addSection(NewSection(20, 1));
    table.addSection(NewSection(20, 2));
    table.addSection(NewSection(142, 3));
    CPPUNIT_ASSERT(table.isValid());
    CPPUNIT_ASSERT_EQUAL(size_t(4), table.sectionCount());
    CPPUNIT_ASSERT_EQUAL(size_t(366), table.totalSize());
    CPPUNIT_ASSERT_EQUAL(ts::PacketCounter(2), table.packetCount());
}
