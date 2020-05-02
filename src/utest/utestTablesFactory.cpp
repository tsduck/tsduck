//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//  TSUnit test suite for ts::TablesFactory.
//
//----------------------------------------------------------------------------

#include "tsTablesFactory.h"
#include "tsAbstractTable.h"
#include "tsMGT.h"
#include "tsLDT.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TablesFactoryTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testRegistrations();
    void testSharedTID();

    TSUNIT_TEST_BEGIN(TablesFactoryTest);
    TSUNIT_TEST(testRegistrations);
    TSUNIT_TEST(testSharedTID);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(TablesFactoryTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TablesFactoryTest::beforeTest()
{
}

// Test suite cleanup method.
void TablesFactoryTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TablesFactoryTest::testRegistrations()
{
    ts::UStringList names;

    ts::TablesFactory::Instance()->getRegisteredTableNames(names);
    debug() << "TablesFactoryTest::testRegistrations: table names: " << ts::UString::Join(names) << std::endl;

    TSUNIT_ASSERT(!names.empty());
    TSUNIT_ASSERT(ts::UString(u"PAT").containSimilar(names));
    TSUNIT_ASSERT(ts::UString(u"PMT").containSimilar(names));

    ts::TablesFactory::Instance()->getRegisteredDescriptorNames(names);
    debug() << "TablesFactoryTest::testRegistrations: descriptor names: " << ts::UString::Join(names) << std::endl;

    TSUNIT_ASSERT(!names.empty());
    TSUNIT_ASSERT(ts::UString(u"ca_descriptor").containSimilar(names));
}

void TablesFactoryTest::testSharedTID()
{
    // Shared table ids between ATSC and ISDB.
    TSUNIT_EQUAL(ts::TID_MGT, ts::TID_LDT);
    TSUNIT_EQUAL(ts::TID_TVCT, ts::TID_CDT);

    // When the same TID is used by two distinct standards, they have no standard in common
    // (meaning encountering this TID in a TS is not sufficient to determine a standard).
    TSUNIT_EQUAL(ts::STD_NONE, ts::TablesFactory::Instance()->getTableStandards(ts::TID_MGT));
    TSUNIT_EQUAL(ts::STD_ATSC, ts::TablesFactory::Instance()->getTableStandards(ts::TID_CVCT));

    ts::TablesFactory::TableFactory factory = ts::TablesFactory::Instance()->getTableFactory(ts::TID_LDT, ts::STD_ATSC);
    TSUNIT_ASSERT(factory != nullptr);
    ts::AbstractTablePtr table(factory());
    TSUNIT_ASSERT(!table.isNull());
    TSUNIT_EQUAL(ts::TID_MGT, table->tableId());
    TSUNIT_EQUAL(ts::STD_ATSC, table->definingStandards());
    TSUNIT_EQUAL(u"MGT", table->xmlName());

    factory = ts::TablesFactory::Instance()->getTableFactory(ts::TID_LDT, ts::STD_ISDB);
    TSUNIT_ASSERT(factory != nullptr);
    table = factory();
    TSUNIT_ASSERT(!table.isNull());
    TSUNIT_EQUAL(ts::TID_LDT, table->tableId());
    TSUNIT_EQUAL(ts::STD_ISDB, table->definingStandards());
    TSUNIT_EQUAL(u"LDT", table->xmlName());

    TSUNIT_ASSERT(ts::MGT::DisplaySection == ts::TablesFactory::Instance()->getSectionDisplay(ts::TID_LDT, ts::STD_ATSC));
    TSUNIT_ASSERT(ts::LDT::DisplaySection == ts::TablesFactory::Instance()->getSectionDisplay(ts::TID_LDT, ts::STD_ISDB));
}
