//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for ts::PSIRepository.
//
//----------------------------------------------------------------------------

#include "tsPSIRepository.h"
#include "tsMGT.h"
#include "tsLDT.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class PSIRepositoryTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(DataTypes);
    TSUNIT_DECLARE_TEST(Registrations);
    TSUNIT_DECLARE_TEST(SharedTID);
};

TSUNIT_REGISTER(PSIRepositoryTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(DataTypes)
{
    // These types are supposed to be compact.
    TSUNIT_ASSUME(sizeof(ts::XTID) == 4);
    TSUNIT_ASSUME(sizeof(ts::XDID) == 2);
    TSUNIT_ASSUME(sizeof(ts::EDID) == 8);

    // Dump PSI Repository internal state in debug mode.
    if (debugMode()) {
        ts::PSIRepository::Instance().dumpInternalState(debug());
    }
}

TSUNIT_DEFINE_TEST(Registrations)
{
    ts::UStringList names;

    ts::PSIRepository::Instance().getRegisteredTableNames(names);
    debug() << "PSIRepositoryTest::testRegistrations: table names: " << ts::UString::Join(names) << std::endl;

    TSUNIT_ASSERT(!names.empty());
    TSUNIT_ASSERT(ts::UString(u"PAT").isContainedSimilarIn(names));
    TSUNIT_ASSERT(ts::UString(u"PMT").isContainedSimilarIn(names));

    ts::PSIRepository::Instance().getRegisteredDescriptorNames(names);
    debug() << "PSIRepositoryTest::testRegistrations: descriptor names: " << ts::UString::Join(names) << std::endl;

    TSUNIT_ASSERT(!names.empty());
    TSUNIT_ASSERT(ts::UString(u"ca_descriptor").isContainedSimilarIn(names));
}

TSUNIT_DEFINE_TEST(SharedTID)
{
    // Shared table ids between ATSC and ISDB.
    TSUNIT_EQUAL(ts::TID_MGT, ts::TID_LDT);
    TSUNIT_EQUAL(ts::TID_TVCT, ts::TID_CDT);

    // When the same TID is used by two distinct standards, they have no standard in common
    // (meaning encountering this TID in a TS is not sufficient to determine a standard).
    TSUNIT_EQUAL(ts::Standards::NONE, ts::PSIRepository::Instance().getTableStandards(ts::TID_MGT));
    TSUNIT_EQUAL(ts::Standards::ATSC, ts::PSIRepository::Instance().getTableStandards(ts::TID_MGT, ts::PID_PSIP));
    TSUNIT_EQUAL(ts::Standards::ISDB, ts::PSIRepository::Instance().getTableStandards(ts::TID_MGT, ts::PID_LDT));
    TSUNIT_EQUAL(ts::Standards::ATSC, ts::PSIRepository::Instance().getTableStandards(ts::TID_CVCT));

    ts::PSIRepository::TableFactory factory = ts::PSIRepository::Instance().getTable(ts::TID_LDT, ts::SectionContext(ts::PID_NULL, ts::Standards::ATSC)).factory;
    TSUNIT_ASSERT(factory != nullptr);
    ts::AbstractTablePtr table(factory());
    TSUNIT_ASSERT(table != nullptr);
    TSUNIT_EQUAL(ts::TID_MGT, table->tableId());
    TSUNIT_EQUAL(ts::Standards::ATSC, table->definingStandards());
    TSUNIT_EQUAL(u"MGT", table->xmlName());

    factory = ts::PSIRepository::Instance().getTable(ts::TID_LDT, ts::SectionContext(ts::PID_NULL, ts::Standards::ISDB)).factory;
    TSUNIT_ASSERT(factory != nullptr);
    table = factory();
    TSUNIT_ASSERT(table != nullptr);
    TSUNIT_EQUAL(ts::TID_LDT, table->tableId());
    TSUNIT_EQUAL(ts::Standards::ISDB, table->definingStandards());
    TSUNIT_EQUAL(u"LDT", table->xmlName());

    factory = ts::PSIRepository::Instance().getTable(ts::TID_LDT, ts::SectionContext(ts::PID_PSIP, ts::Standards::NONE)).factory;
    TSUNIT_ASSERT(factory != nullptr);
    table = factory();
    TSUNIT_ASSERT(table != nullptr);
    TSUNIT_EQUAL(ts::TID_MGT, table->tableId());
    TSUNIT_EQUAL(ts::Standards::ATSC, table->definingStandards());
    TSUNIT_EQUAL(u"MGT", table->xmlName());

    TSUNIT_ASSERT(ts::MGT::DisplaySection == ts::PSIRepository::Instance().getTable(ts::TID_LDT, ts::SectionContext(ts::PID_NULL, ts::Standards::ATSC)).display);
    TSUNIT_ASSERT(ts::LDT::DisplaySection == ts::PSIRepository::Instance().getTable(ts::TID_LDT, ts::SectionContext(ts::PID_NULL, ts::Standards::ISDB)).display);
    TSUNIT_ASSERT(ts::MGT::DisplaySection == ts::PSIRepository::Instance().getTable(ts::TID_LDT, ts::SectionContext(ts::PID_PSIP, ts::Standards::NONE)).display);
    TSUNIT_ASSERT(ts::LDT::DisplaySection == ts::PSIRepository::Instance().getTable(ts::TID_LDT, ts::SectionContext(ts::PID_LDT, ts::Standards::NONE)).display);
}
