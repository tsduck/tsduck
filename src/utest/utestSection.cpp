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
//  TSUnit test suite for class ts::Section
//
//----------------------------------------------------------------------------

#include "tsSection.h"
#include "tsBinaryTable.h"
#include "tsNames.h"
#include "tsunit.h"

#include "tables/psi_tot_tnt_sections.h"
#include "tables/psi_bat_tvnum_sections.h"
#include "tables/psi_nit_tntv23_sections.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SectionTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testTOT();
    void testBAT();
    void testNIT();
    void testReload();
    void testAssign();
    void testPackSections();
    void testSize();

    TSUNIT_TEST_BEGIN(SectionTest);
    TSUNIT_TEST(testTOT);
    TSUNIT_TEST(testBAT);
    TSUNIT_TEST(testNIT);
    TSUNIT_TEST(testReload);
    TSUNIT_TEST(testAssign);
    TSUNIT_TEST(testPackSections);
    TSUNIT_TEST(testSize);
    TSUNIT_TEST_END();

private:
    // Create a dummy long section.
    static ts::SectionPtr NewSection(size_t size, uint8_t secnum = 0, ts::TID tid = 0xEE);
};

TSUNIT_REGISTER(SectionTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void SectionTest::beforeTest()
{
}

// Test suite cleanup method.
void SectionTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void SectionTest::testTOT()
{
    ts::Section sec(psi_tot_tnt_sections, sizeof(psi_tot_tnt_sections), ts::PID_TOT, ts::CRC32::CHECK);

    TSUNIT_ASSERT(sec.isValid());
    TSUNIT_EQUAL(ts::TID_TOT, sec.tableId());
    TSUNIT_EQUAL(ts::PID_TOT, sec.sourcePID());
    TSUNIT_ASSERT(!sec.isLongSection());
}

void SectionTest::testBAT()
{
    ts::Section sec(psi_bat_tvnum_sections, sizeof(psi_bat_tvnum_sections), ts::PID_BAT, ts::CRC32::CHECK);

    TSUNIT_ASSERT(sec.isValid());
    TSUNIT_EQUAL(ts::TID_BAT, sec.tableId());
    TSUNIT_EQUAL(ts::PID_BAT, sec.sourcePID());
    TSUNIT_ASSERT(sec.isLongSection());
}

void SectionTest::testNIT()
{
    ts::Section sec(psi_nit_tntv23_sections, sizeof(psi_nit_tntv23_sections), ts::PID_NIT, ts::CRC32::CHECK);

    TSUNIT_ASSERT(sec.isValid());
    TSUNIT_EQUAL(ts::TID_NIT_ACT, sec.tableId());
    TSUNIT_EQUAL(ts::PID_NIT, sec.sourcePID());
    TSUNIT_ASSERT(sec.isLongSection());
}

void SectionTest::testReload()
{
    ts::Section sec(psi_tot_tnt_sections, sizeof(psi_tot_tnt_sections), ts::PID_TOT, ts::CRC32::CHECK);

    TSUNIT_ASSERT(sec.isValid());
    TSUNIT_EQUAL(ts::TID_TOT, sec.tableId());
    TSUNIT_EQUAL(ts::PID_TOT, sec.sourcePID());
    TSUNIT_ASSERT(!sec.isLongSection());

    sec.reload(psi_bat_tvnum_sections, sizeof(psi_bat_tvnum_sections), ts::PID_BAT, ts::CRC32::CHECK);

    TSUNIT_ASSERT(sec.isValid());
    TSUNIT_EQUAL(ts::TID_BAT, sec.tableId());
    TSUNIT_EQUAL(ts::PID_BAT, sec.sourcePID());
    TSUNIT_ASSERT(sec.isLongSection());
}

void SectionTest::testAssign()
{
    ts::Section sec(psi_tot_tnt_sections, sizeof(psi_tot_tnt_sections), ts::PID_TOT, ts::CRC32::CHECK);

    TSUNIT_ASSERT(sec.isValid());
    TSUNIT_EQUAL(ts::TID_TOT, sec.tableId());
    TSUNIT_EQUAL(ts::PID_TOT, sec.sourcePID());
    TSUNIT_ASSERT(!sec.isLongSection());

    ts::Section sec2(psi_nit_tntv23_sections, sizeof(psi_nit_tntv23_sections), ts::PID_NIT, ts::CRC32::CHECK);

    TSUNIT_ASSERT(sec2.isValid());
    TSUNIT_EQUAL(ts::TID_NIT_ACT, sec2.tableId());
    TSUNIT_EQUAL(ts::PID_NIT, sec2.sourcePID());
    TSUNIT_ASSERT(sec2.isLongSection());

    sec = sec2;

    TSUNIT_ASSERT(sec.isValid());
    TSUNIT_EQUAL(ts::TID_NIT_ACT, sec.tableId());
    TSUNIT_EQUAL(ts::PID_NIT, sec.sourcePID());
    TSUNIT_ASSERT(sec.isLongSection());
}

void SectionTest::testPackSections()
{
    ts::BinaryTable table;
    TSUNIT_ASSERT(!table.isValid());

    static const uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7};

    table.addSection(new ts::Section(150, true, 102, 12, true, 3, 7, data + 1, 2, 2000));
    TSUNIT_ASSERT(!table.isValid());

    table.addSection(new ts::Section(150, true, 102, 12, true, 5, 7, data + 3, 3, 2000));
    TSUNIT_ASSERT(!table.isValid());

    table.addSection(new ts::Section(150, true, 102, 12, true, 6, 7, data + 4, 4, 2000));
    TSUNIT_ASSERT(!table.isValid());

    table.packSections();

    TSUNIT_ASSERT(table.isValid());
    TSUNIT_ASSERT(!table.isShortSection());
    TSUNIT_EQUAL(150, table.tableId());
    TSUNIT_EQUAL(102, table.tableIdExtension());
    TSUNIT_EQUAL(12, table.version());
    TSUNIT_EQUAL(2000, table.sourcePID());
    TSUNIT_EQUAL(3, table.sectionCount());

    ts::SectionPtr sec(table.sectionAt(0));
    TSUNIT_ASSERT(!sec.isNull());
    TSUNIT_ASSERT(sec->isValid());
    TSUNIT_ASSERT(sec->isLongSection());
    TSUNIT_ASSERT(!sec->isShortSection());
    TSUNIT_ASSERT(sec->isPrivateSection());
    TSUNIT_ASSERT(sec->isCurrent());
    TSUNIT_ASSERT(!sec->isNext());
    TSUNIT_EQUAL(150, sec->tableId());
    TSUNIT_EQUAL(102, sec->tableIdExtension());
    TSUNIT_EQUAL(12, sec->version());
    TSUNIT_EQUAL(2000, sec->sourcePID());
    TSUNIT_EQUAL(0, sec->sectionNumber());
    TSUNIT_EQUAL(2, sec->lastSectionNumber());
    TSUNIT_EQUAL(2, sec->payloadSize());
    TSUNIT_ASSERT(sec->payload() != nullptr);
    TSUNIT_EQUAL(1, *sec->payload());

    sec = table.sectionAt(1);
    TSUNIT_ASSERT(!sec.isNull());
    TSUNIT_ASSERT(sec->isValid());
    TSUNIT_ASSERT(sec->isLongSection());
    TSUNIT_ASSERT(!sec->isShortSection());
    TSUNIT_ASSERT(sec->isPrivateSection());
    TSUNIT_ASSERT(sec->isCurrent());
    TSUNIT_ASSERT(!sec->isNext());
    TSUNIT_EQUAL(150, sec->tableId());
    TSUNIT_EQUAL(102, sec->tableIdExtension());
    TSUNIT_EQUAL(12, sec->version());
    TSUNIT_EQUAL(2000, sec->sourcePID());
    TSUNIT_EQUAL(1, sec->sectionNumber());
    TSUNIT_EQUAL(2, sec->lastSectionNumber());
    TSUNIT_EQUAL(3, sec->payloadSize());
    TSUNIT_ASSERT(sec->payload() != nullptr);
    TSUNIT_EQUAL(3, *sec->payload());

    sec = table.sectionAt(2);
    TSUNIT_ASSERT(!sec.isNull());
    TSUNIT_ASSERT(sec->isValid());
    TSUNIT_ASSERT(sec->isLongSection());
    TSUNIT_ASSERT(!sec->isShortSection());
    TSUNIT_ASSERT(sec->isPrivateSection());
    TSUNIT_ASSERT(sec->isCurrent());
    TSUNIT_ASSERT(!sec->isNext());
    TSUNIT_EQUAL(150, sec->tableId());
    TSUNIT_EQUAL(102, sec->tableIdExtension());
    TSUNIT_EQUAL(12, sec->version());
    TSUNIT_EQUAL(2000, sec->sourcePID());
    TSUNIT_EQUAL(2, sec->sectionNumber());
    TSUNIT_EQUAL(2, sec->lastSectionNumber());
    TSUNIT_EQUAL(4, sec->payloadSize());
    TSUNIT_ASSERT(sec->payload() != nullptr);
    TSUNIT_EQUAL(4, *sec->payload());
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
    TSUNIT_ASSERT(table.isValid());
    TSUNIT_EQUAL(1, table.sectionCount());
    TSUNIT_EQUAL(183, table.sectionAt(0)->size());
    TSUNIT_EQUAL(183, table.totalSize());
    TSUNIT_EQUAL(1, table.packetCount());

    table.clear();
    table.addSection(NewSection(184, 0));
    TSUNIT_ASSERT(table.isValid());
    TSUNIT_EQUAL(1, table.sectionCount());
    TSUNIT_EQUAL(184, table.sectionAt(0)->size());
    TSUNIT_EQUAL(184, table.totalSize());
    TSUNIT_EQUAL(2, table.packetCount());

    table.addSection(NewSection(182, 1));
    TSUNIT_ASSERT(table.isValid());
    TSUNIT_EQUAL(2, table.sectionCount());
    TSUNIT_EQUAL(182, table.sectionAt(1)->size());
    TSUNIT_EQUAL(366, table.totalSize());
    TSUNIT_EQUAL(2, table.packetCount());

    table.clear();
    table.addSection(NewSection(184, 0));
    table.addSection(NewSection(20, 1));
    table.addSection(NewSection(20, 2));
    table.addSection(NewSection(142, 3));
    TSUNIT_ASSERT(table.isValid());
    TSUNIT_EQUAL(4, table.sectionCount());
    TSUNIT_EQUAL(366, table.totalSize());
    TSUNIT_EQUAL(2, table.packetCount());
}
