//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  CppUnit test suite for ts::XMLTables.
//
//----------------------------------------------------------------------------

#include "tsXMLTables.h"
#include "tsTables.h"
#include "tsSysUtils.h"
#include "tsBinaryTable.h"
#include "tsStringUtils.h"
#include "tsCerrReport.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;

#include "tables/psi_pat1_xml.h"
#include "tables/psi_pat1_sections.h"
#include "tables/psi_all_xml.h"
#include "tables/psi_all_sections.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class XMLTablesTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testConfigurationFile();
    void testGenericDescriptor();
    void testGenericShortTable();
    void testGenericLongTable();
    void testPAT1();
    void testAllTables();

    CPPUNIT_TEST_SUITE(XMLTablesTest);
    CPPUNIT_TEST(testConfigurationFile);
    CPPUNIT_TEST(testGenericDescriptor);
    CPPUNIT_TEST(testGenericShortTable);
    CPPUNIT_TEST(testGenericLongTable);
    CPPUNIT_TEST(testPAT1);
    CPPUNIT_TEST(testAllTables);
    CPPUNIT_TEST_SUITE_END();

private:
    // Unitary test for one table.
    void testTable(const char* name, const char* ref_xml, const uint8_t* ref_sections, size_t ref_sections_size);
};

CPPUNIT_TEST_SUITE_REGISTRATION(XMLTablesTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void XMLTablesTest::setUp()
{
}

// Test suite cleanup method.
void XMLTablesTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests from XML tables.
//----------------------------------------------------------------------------

#define TESTTABLE(name, data)                                                                     \
    void XMLTablesTest::test##name()                                                              \
    {                                                                                             \
        testTable(#name, psi_##data##_xml, psi_##data##_sections, sizeof(psi_##data##_sections)); \
    }

TESTTABLE(PAT1, pat1)
TESTTABLE(AllTables, all)

void XMLTablesTest::testTable(const char* name, const char* ref_xml, const uint8_t* ref_sections, size_t ref_sections_size)
{
    utest::Out() << "XMLTablesTest: Testing " << name << std::endl;

    // Convert XML reference content to binary tables.
    ts::XMLTables xml;
    CPPUNIT_ASSERT(xml.parseXML(std::string(ref_xml), CERR));

    // Serialize binary tables to section data.
    std::ostringstream strm;
    CPPUNIT_ASSERT(ts::BinaryTable::SaveFile(xml.tables(), strm, CERR));

    // Compare serialized section data to reference section data.
    const std::string sections(strm.str());
    CPPUNIT_ASSERT_EQUAL(ref_sections_size, sections.size());
    CPPUNIT_ASSERT_EQUAL(0, ::memcmp(ref_sections, sections.data(), ref_sections_size));

    // Convert binary tables to XML.
    CPPUNIT_ASSERT_USTRINGS_EQUAL(ref_xml, xml.toText(CERR));
}


//----------------------------------------------------------------------------
// Other unitary tests.
//----------------------------------------------------------------------------

void XMLTablesTest::testConfigurationFile()
{
    const std::string conf(ts::SearchConfigurationFile("tsduck.xml"));
    utest::Out() << "XMLTablesTest::testConfigurationFile: " << conf << std::endl;
    CPPUNIT_ASSERT(ts::FileExists(conf));
}

void XMLTablesTest::testGenericDescriptor()
{
    static const uint8_t descData[] = {
        0x72,   // tag
        0x07,   // length
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };
    ts::Descriptor desc(descData, sizeof(descData));
    CPPUNIT_ASSERT(desc.isValid());
    CPPUNIT_ASSERT_EQUAL(0x72, int(desc.tag()));
    CPPUNIT_ASSERT_EQUAL(9, int(desc.size()));
    CPPUNIT_ASSERT_EQUAL(7, int(desc.payloadSize()));

    ts::XML xml(CERR);
    ts::XML::Document doc;
    ts::XML::Element* root = xml.initializeDocument(&doc, "test");
    CPPUNIT_ASSERT(root != 0);
    CPPUNIT_ASSERT(ts::XMLTables::ToGenericDescriptor(xml, root, desc) != 0);

    ts::UString text(xml.toString(doc));
    utest::Out() << "XMLTablesTest::testGenericDescriptor: " << text << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<test>\n"
        "  <generic_descriptor tag=\"0x72\">\n"
        "    01 02 03 04 05 06 07\n"
        "  </generic_descriptor>\n"
        "</test>\n",
        text);

    ts::XML::Document doc2;
    CPPUNIT_ASSERT(xml.parseDocument(doc2, text));
    root = doc2.RootElement();
    CPPUNIT_ASSERT(root != 0);
    CPPUNIT_ASSERT(root->Name() != 0);
    CPPUNIT_ASSERT_STRINGS_EQUAL("test", root->Name());

    ts::XML::ElementVector children;
    CPPUNIT_ASSERT(xml.getChildren(children, root, "generic_descriptor", 1, 1));
    CPPUNIT_ASSERT_EQUAL(size_t(1), children.size());

    ts::ByteBlock payload;
    CPPUNIT_ASSERT(xml.getHexaText(payload, children[0]));
    CPPUNIT_ASSERT_EQUAL(size_t(7), payload.size());
    CPPUNIT_ASSERT(payload == ts::ByteBlock(descData + 2, sizeof(descData) - 2));

    ts::DescriptorPtr dp(ts::XMLTables::FromGenericDescriptorXML(xml, children[0]));
    CPPUNIT_ASSERT(!dp.isNull());
    CPPUNIT_ASSERT_EQUAL(ts::DID(0x72), dp->tag());
    CPPUNIT_ASSERT_EQUAL(size_t(7), dp->payloadSize());
    CPPUNIT_ASSERT(ts::ByteBlock(dp->payload(), dp->payloadSize()) == ts::ByteBlock(descData + 2, sizeof(descData) - 2));
}

void XMLTablesTest::testGenericShortTable()
{
    static const uint8_t refData[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    const ts::SectionPtr refSection(new ts::Section(0xAB, false, refData, sizeof(refData)));
    CPPUNIT_ASSERT(!refSection.isNull());
    CPPUNIT_ASSERT(refSection->isValid());

    ts::BinaryTable refTable;
    refTable.addSection(refSection);
    CPPUNIT_ASSERT(refTable.isValid());
    CPPUNIT_ASSERT_EQUAL(size_t(1), refTable.sectionCount());

    ts::XML xml(CERR);
    ts::XML::Document doc;
    ts::XML::Element* root = xml.initializeDocument(&doc, "test");
    CPPUNIT_ASSERT(root != 0);
    CPPUNIT_ASSERT(ts::XMLTables::ToGenericTable(xml, root, refTable) != 0);

    ts::UString text(xml.toString(doc));
    utest::Out() << "XMLTablesTest::testGenericShortTable: " << text << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<test>\n"
        "  <generic_short_table table_id=\"0xAB\" private=\"false\">\n"
        "    01 02 03 04 05 06\n"
        "  </generic_short_table>\n"
        "</test>\n",
        text);

    ts::XML::Document doc2;
    CPPUNIT_ASSERT(xml.parseDocument(doc2, text));
    root = doc2.RootElement();
    CPPUNIT_ASSERT(root != 0);
    CPPUNIT_ASSERT(root->Name() != 0);
    CPPUNIT_ASSERT_STRINGS_EQUAL("test", root->Name());

    ts::XML::ElementVector children;
    CPPUNIT_ASSERT(xml.getChildren(children, root, "GENERIC_SHORT_TABLE", 1, 1));
    CPPUNIT_ASSERT_EQUAL(size_t(1), children.size());

    ts::BinaryTablePtr tab(ts::XMLTables::FromGenericTableXML(xml, children[0]));
    CPPUNIT_ASSERT(!tab.isNull());
    CPPUNIT_ASSERT(tab->isValid());
    CPPUNIT_ASSERT(tab->isShortSection());
    CPPUNIT_ASSERT_EQUAL(ts::TID(0xAB), tab->tableId());
    CPPUNIT_ASSERT_EQUAL(size_t(1), tab->sectionCount());

    ts::SectionPtr sec(tab->sectionAt(0));
    CPPUNIT_ASSERT(!sec.isNull());
    CPPUNIT_ASSERT(sec->isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(0xAB), sec->tableId());
    CPPUNIT_ASSERT(sec->isShortSection());
    CPPUNIT_ASSERT(!sec->isPrivateSection());
    CPPUNIT_ASSERT_EQUAL(size_t(6), sec->payloadSize());
    CPPUNIT_ASSERT(ts::ByteBlock(sec->payload(), sec->payloadSize()) == ts::ByteBlock(refData, sizeof(refData)));
}

void XMLTablesTest::testGenericLongTable()
{
    static const uint8_t refData0[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    static const uint8_t refData1[] = {0x11, 0x12, 0x13, 0x14};

    ts::BinaryTable refTable;
    refTable.addSection(new ts::Section(0xCD, true, 0x1234, 7, true, 0, 0, refData0, sizeof(refData0)));
    refTable.addSection(new ts::Section(0xCD, true, 0x1234, 7, true, 1, 1, refData1, sizeof(refData1)));
    CPPUNIT_ASSERT(refTable.isValid());
    CPPUNIT_ASSERT(!refTable.isShortSection());
    CPPUNIT_ASSERT_EQUAL(ts::TID(0xCD), refTable.tableId());
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x1234), refTable.tableIdExtension());
    CPPUNIT_ASSERT_EQUAL(size_t(2), refTable.sectionCount());

    ts::XML xml(CERR);
    ts::XML::Document doc;
    ts::XML::Element* root = xml.initializeDocument(&doc, "test");
    CPPUNIT_ASSERT(root != 0);
    CPPUNIT_ASSERT(ts::XMLTables::ToGenericTable(xml, root, refTable) != 0);

    ts::UString text(xml.toString(doc));
    utest::Out() << "XMLTablesTest::testGenericLongTable: " << text << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<test>\n"
        "  <generic_long_table table_id=\"0xCD\" table_id_ext=\"0x1234\" version=\"7\" current=\"true\" private=\"true\">\n"
        "    <section>\n"
        "      01 02 03 04 05\n"
        "    </section>\n"
        "    <section>\n"
        "      11 12 13 14\n"
        "    </section>\n"
        "  </generic_long_table>\n"
        "</test>\n",
        text);

    ts::XML::Document doc2;
    CPPUNIT_ASSERT(xml.parseDocument(doc2, text));
    root = doc2.RootElement();
    CPPUNIT_ASSERT(root != 0);
    CPPUNIT_ASSERT(root->Name() != 0);
    CPPUNIT_ASSERT_STRINGS_EQUAL("test", root->Name());

    ts::XML::ElementVector children;
    CPPUNIT_ASSERT(xml.getChildren(children, root, "GENERIC_long_TABLE", 1, 1));
    CPPUNIT_ASSERT_EQUAL(size_t(1), children.size());

    ts::BinaryTablePtr tab(ts::XMLTables::FromGenericTableXML(xml, children[0]));
    CPPUNIT_ASSERT(!tab.isNull());
    CPPUNIT_ASSERT(tab->isValid());
    CPPUNIT_ASSERT(!tab->isShortSection());
    CPPUNIT_ASSERT_EQUAL(ts::TID(0xCD), tab->tableId());
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x1234), tab->tableIdExtension());
    CPPUNIT_ASSERT_EQUAL(size_t(2), tab->sectionCount());

    ts::SectionPtr sec(tab->sectionAt(0));
    CPPUNIT_ASSERT(!sec.isNull());
    CPPUNIT_ASSERT(sec->isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(0xCD), sec->tableId());
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x1234), sec->tableIdExtension());
    CPPUNIT_ASSERT_EQUAL(uint8_t(7), sec->version());
    CPPUNIT_ASSERT(!sec->isShortSection());
    CPPUNIT_ASSERT(sec->isPrivateSection());
    CPPUNIT_ASSERT(sec->isCurrent());
    CPPUNIT_ASSERT_EQUAL(sizeof(refData0), sec->payloadSize());
    CPPUNIT_ASSERT(ts::ByteBlock(sec->payload(), sec->payloadSize()) == ts::ByteBlock(refData0, sizeof(refData0)));

    sec = tab->sectionAt(1);
    CPPUNIT_ASSERT(!sec.isNull());
    CPPUNIT_ASSERT(sec->isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(0xCD), sec->tableId());
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x1234), sec->tableIdExtension());
    CPPUNIT_ASSERT_EQUAL(uint8_t(7), sec->version());
    CPPUNIT_ASSERT(!sec->isShortSection());
    CPPUNIT_ASSERT(sec->isPrivateSection());
    CPPUNIT_ASSERT(sec->isCurrent());
    CPPUNIT_ASSERT_EQUAL(sizeof(refData1), sec->payloadSize());
    CPPUNIT_ASSERT(ts::ByteBlock(sec->payload(), sec->payloadSize()) == ts::ByteBlock(refData1, sizeof(refData1)));
}
