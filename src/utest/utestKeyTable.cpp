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
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testXML();

    TSUNIT_TEST_BEGIN(KeyTableTest);
    TSUNIT_TEST(testXML);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(KeyTableTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void KeyTableTest::beforeTest()
{
}

// Test suite cleanup method.
void KeyTableTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void KeyTableTest::testXML()
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
