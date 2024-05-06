//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for KeyTable class.
//
//----------------------------------------------------------------------------

#include "tsKeyTable.h"
#include "tsCerrReport.h"
#include "tsNullReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class KeyTableTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(XML);
};

TSUNIT_REGISTER(KeyTableTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(XML)
{
    static const ts::UChar* const xmlText =
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<!-- test keys -->\n"
        u"<tsduck>\n"
        u"  <key id=\"5A99C9E8CB9A81EEAE80C69BFB55B4B6\" value=\"D9F91156DC9FC5B6E679C3E1ED8B6667\"/>\n"
        u"  <key id=\"7A77C81AF4E1EE0E94DB6A0C359F2E71\" value=\"C01BEF7E15D7AFD2B4CBB0A696892899613084BE742406725E547FDE45DB4E29\"/>\n"
        u"</tsduck>\n";

    ts::KeyTable table;
    TSUNIT_ASSERT(table.empty());
    TSUNIT_EQUAL(0, table.size());

    TSUNIT_ASSERT(table.loadXML(CERR, xmlText, true, 16, 0));

    TSUNIT_ASSERT(!table.empty());
    TSUNIT_EQUAL(2, table.size());

    TSUNIT_ASSERT(table.hasKey(u"5A99C9E8CB9A81EEAE80C69BFB55B4B6"));
    TSUNIT_ASSERT(table.hasKey(u"7A77C81AF4E1EE0E94DB6A0C359F2E71"));
    TSUNIT_ASSERT(!table.hasKey(u"D1CDB386C4AFCEF329A987B93D913140"));

    TSUNIT_ASSERT(table.storeKey(u"D1CDB386C4AFCEF329A987B93D913140", u"2974B88317CAE2DCECA19BE408376B7D", true));
    TSUNIT_ASSERT(table.hasKey(u"D1CDB386C4AFCEF329A987B93D913140"));
    TSUNIT_EQUAL(3, table.size());

    TSUNIT_ASSERT(!table.storeKey(u"D1CDB386C4AFCEF329A987B93D913140", u"210C502F2FFDCA98587DBA7C9082F1A1", false));
    TSUNIT_ASSERT(table.storeKey(u"D1CDB386C4AFCEF329A987B93D913140", u"210C502F2FFDCA98587DBA7C9082F1A1", true));
    TSUNIT_ASSERT(table.hasKey(u"D1CDB386C4AFCEF329A987B93D913140"));
    TSUNIT_EQUAL(3, table.size());

    ts::ByteBlock value;
    TSUNIT_ASSERT(!table.getKey(u"2299C9E8CB9A81EEAE80C69BFB55B4B6", value));
    TSUNIT_ASSERT(table.getKey(u"5A99C9E8CB9A81EEAE80C69BFB55B4B6", value));
    TSUNIT_EQUAL(u"D9F91156DC9FC5B6E679C3E1ED8B6667", ts::UString::Dump(value, ts::UString::COMPACT));
}
