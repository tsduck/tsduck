//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for ts::PSIRepository.
//
//----------------------------------------------------------------------------

#include "tsPSIRepository.h"
#include "tsAbstractTable.h"
#include "tsMGT.h"
#include "tsLDT.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class PSIRepositoryTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testRegistrations();
    void testSharedTID();

    TSUNIT_TEST_BEGIN(PSIRepositoryTest);
    TSUNIT_TEST(testRegistrations);
    TSUNIT_TEST(testSharedTID);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(PSIRepositoryTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void PSIRepositoryTest::beforeTest()
{
}

// Test suite cleanup method.
void PSIRepositoryTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void PSIRepositoryTest::testRegistrations()
{
    ts::UStringList names;

    ts::PSIRepository::Instance()->getRegisteredTableNames(names);
    debug() << "PSIRepositoryTest::testRegistrations: table names: " << ts::UString::Join(names) << std::endl;

    TSUNIT_ASSERT(!names.empty());
    TSUNIT_ASSERT(ts::UString(u"PAT").isContainedSimilarIn(names));
    TSUNIT_ASSERT(ts::UString(u"PMT").isContainedSimilarIn(names));

    ts::PSIRepository::Instance()->getRegisteredDescriptorNames(names);
    debug() << "PSIRepositoryTest::testRegistrations: descriptor names: " << ts::UString::Join(names) << std::endl;

    TSUNIT_ASSERT(!names.empty());
    TSUNIT_ASSERT(ts::UString(u"ca_descriptor").isContainedSimilarIn(names));
}

void PSIRepositoryTest::testSharedTID()
{
    // Shared table ids between ATSC and ISDB.
    TSUNIT_EQUAL(ts::TID_MGT, ts::TID_LDT);
    TSUNIT_EQUAL(ts::TID_TVCT, ts::TID_CDT);

    // When the same TID is used by two distinct standards, they have no standard in common
    // (meaning encountering this TID in a TS is not sufficient to determine a standard).
    TSUNIT_EQUAL(ts::Standards::NONE, ts::PSIRepository::Instance()->getTableStandards(ts::TID_MGT));
    TSUNIT_EQUAL(ts::Standards::ATSC, ts::PSIRepository::Instance()->getTableStandards(ts::TID_MGT, ts::PID_PSIP));
    TSUNIT_EQUAL(ts::Standards::ISDB, ts::PSIRepository::Instance()->getTableStandards(ts::TID_MGT, ts::PID_LDT));
    TSUNIT_EQUAL(ts::Standards::ATSC, ts::PSIRepository::Instance()->getTableStandards(ts::TID_CVCT));

    ts::PSIRepository::TableFactory factory = ts::PSIRepository::Instance()->getTableFactory(ts::TID_LDT, ts::Standards::ATSC);
    TSUNIT_ASSERT(factory != nullptr);
    ts::AbstractTablePtr table(factory());
    TSUNIT_ASSERT(!table.isNull());
    TSUNIT_EQUAL(ts::TID_MGT, table->tableId());
    TSUNIT_EQUAL(ts::Standards::ATSC, table->definingStandards());
    TSUNIT_EQUAL(u"MGT", table->xmlName());

    factory = ts::PSIRepository::Instance()->getTableFactory(ts::TID_LDT, ts::Standards::ISDB);
    TSUNIT_ASSERT(factory != nullptr);
    table = factory();
    TSUNIT_ASSERT(!table.isNull());
    TSUNIT_EQUAL(ts::TID_LDT, table->tableId());
    TSUNIT_EQUAL(ts::Standards::ISDB, table->definingStandards());
    TSUNIT_EQUAL(u"LDT", table->xmlName());

    factory = ts::PSIRepository::Instance()->getTableFactory(ts::TID_LDT, ts::Standards::NONE, ts::PID_PSIP);
    TSUNIT_ASSERT(factory != nullptr);
    table = factory();
    TSUNIT_ASSERT(!table.isNull());
    TSUNIT_EQUAL(ts::TID_MGT, table->tableId());
    TSUNIT_EQUAL(ts::Standards::ATSC, table->definingStandards());
    TSUNIT_EQUAL(u"MGT", table->xmlName());

    TSUNIT_ASSERT(ts::MGT::DisplaySection == ts::PSIRepository::Instance()->getSectionDisplay(ts::TID_LDT, ts::Standards::ATSC));
    TSUNIT_ASSERT(ts::LDT::DisplaySection == ts::PSIRepository::Instance()->getSectionDisplay(ts::TID_LDT, ts::Standards::ISDB));
    TSUNIT_ASSERT(ts::MGT::DisplaySection == ts::PSIRepository::Instance()->getSectionDisplay(ts::TID_LDT, ts::Standards::NONE, ts::PID_PSIP));
    TSUNIT_ASSERT(ts::LDT::DisplaySection == ts::PSIRepository::Instance()->getSectionDisplay(ts::TID_LDT, ts::Standards::NONE, ts::PID_LDT));
}
