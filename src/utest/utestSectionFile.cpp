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
//  CppUnit test suite for SectionFile (binary and XML).
//
//----------------------------------------------------------------------------

#include "tsSectionFile.h"
#include "tsPAT.h"
#include "tsTDT.h"
#include "tsSysUtils.h"
#include "tsBinaryTable.h"
#include "tsCerrReport.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;

#include "tables/psi_pat1_xml.h"
#include "tables/psi_pat1_sections.h"
#include "tables/psi_pmt_scte35_xml.h"
#include "tables/psi_pmt_scte35_sections.h"
#include "tables/psi_all_xml.h"
#include "tables/psi_all_sections.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SectionFileTest: public CppUnit::TestFixture
{
public:
    SectionFileTest();

    virtual void setUp() override;
    virtual void tearDown() override;

    void testConfigurationFile();
    void testGenericDescriptor();
    void testGenericShortTable();
    void testGenericLongTable();
    void testPAT1();
    void testSCTE35();
    void testAllTables();
    void testBuildSections();

    CPPUNIT_TEST_SUITE(SectionFileTest);
    CPPUNIT_TEST(testConfigurationFile);
    CPPUNIT_TEST(testGenericDescriptor);
    CPPUNIT_TEST(testGenericShortTable);
    CPPUNIT_TEST(testGenericLongTable);
    CPPUNIT_TEST(testPAT1);
    CPPUNIT_TEST(testSCTE35);
    CPPUNIT_TEST(testAllTables);
    CPPUNIT_TEST(testBuildSections);
    CPPUNIT_TEST_SUITE_END();

private:
    // Unitary test for one table.
    void testTable(const char* name, const ts::UChar* ref_xml, const uint8_t* ref_sections, size_t ref_sections_size);
    ts::Report& report();
    ts::UString _tempFileNameBin;
    ts::UString _tempFileNameXML;
};

CPPUNIT_TEST_SUITE_REGISTRATION(SectionFileTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
SectionFileTest::SectionFileTest() :
    _tempFileNameBin(ts::TempFile(u".tmp.bin")),
    _tempFileNameXML(ts::TempFile(u".tmp.xml"))
{
}

// Test suite initialization method.
void SectionFileTest::setUp()
{
    ts::DeleteFile(_tempFileNameBin);
    ts::DeleteFile(_tempFileNameXML);
}

// Test suite cleanup method.
void SectionFileTest::tearDown()
{
    ts::DeleteFile(_tempFileNameBin);
    ts::DeleteFile(_tempFileNameXML);
}

ts::Report& SectionFileTest::report()
{
    if (utest::DebugMode()) {
        return CERR;
    }
    else {
        return NULLREP;
    }
}


//----------------------------------------------------------------------------
// Unitary tests from XML tables.
//----------------------------------------------------------------------------

#define TESTTABLE(name, data)                                                                     \
    void SectionFileTest::test##name()                                                            \
    {                                                                                             \
        testTable(#name, psi_##data##_xml, psi_##data##_sections, sizeof(psi_##data##_sections)); \
    }

TESTTABLE(PAT1, pat1)
TESTTABLE(SCTE35, pmt_scte35)
TESTTABLE(AllTables, all)

void SectionFileTest::testTable(const char* name, const ts::UChar* ref_xml, const uint8_t* ref_sections, size_t ref_sections_size)
{
    utest::Out() << "SectionFileTest: Testing " << name << std::endl;

    // Convert XML reference content to binary tables.
    ts::SectionFile xml;
    CPPUNIT_ASSERT(xml.parseXML(ref_xml, CERR));

    // Serialize binary tables to section data.
    std::ostringstream strm;
    CPPUNIT_ASSERT(xml.saveBinary(strm, CERR));

    // Compare serialized section data to reference section data.
    const std::string sections(strm.str());
    CPPUNIT_ASSERT_EQUAL(ref_sections_size, sections.size());
    CPPUNIT_ASSERT_EQUAL(0, ::memcmp(ref_sections, sections.data(), ref_sections_size));

    // Convert binary tables to XML.
    CPPUNIT_ASSERT_USTRINGS_EQUAL(ref_xml, xml.toXML(CERR));
}


//----------------------------------------------------------------------------
// Other unitary tests.
//----------------------------------------------------------------------------

void SectionFileTest::testConfigurationFile()
{
    const ts::UString conf(ts::SearchConfigurationFile(u"tsduck.xml"));
    utest::Out() << "SectionFileTest::testConfigurationFile: " << conf << std::endl;
    CPPUNIT_ASSERT(ts::FileExists(conf));
}

void SectionFileTest::testGenericDescriptor()
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

    ts::xml::Document doc(report());
    ts::xml::Element* root = doc.initialize(u"test");
    CPPUNIT_ASSERT(root != nullptr);
    CPPUNIT_ASSERT(desc.toXML(root, 0, ts::TID_NULL, true) != nullptr);

    ts::UString text(doc.toString());
    utest::Out() << "SectionFileTest::testGenericDescriptor: " << text << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<test>\n"
        u"  <generic_descriptor tag=\"0x72\">\n"
        u"    01 02 03 04 05 06 07\n"
        u"  </generic_descriptor>\n"
        u"</test>\n",
        text);

    ts::xml::Document doc2(report());
    CPPUNIT_ASSERT(doc2.parse(text));
    root = doc2.rootElement();
    CPPUNIT_ASSERT(root != nullptr);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"test", root->name());

    ts::xml::ElementVector children;
    CPPUNIT_ASSERT(root->getChildren(children, u"generic_descriptor", 1, 1));
    CPPUNIT_ASSERT_EQUAL(size_t(1), children.size());

    ts::ByteBlock payload;
    CPPUNIT_ASSERT(children[0]->getHexaText(payload));
    CPPUNIT_ASSERT_EQUAL(size_t(7), payload.size());
    CPPUNIT_ASSERT(payload == ts::ByteBlock(descData + 2, sizeof(descData) - 2));

    ts::Descriptor desc2;
    CPPUNIT_ASSERT(desc2.fromXML(children[0]));
    CPPUNIT_ASSERT_EQUAL(ts::DID(0x72), desc2.tag());
    CPPUNIT_ASSERT_EQUAL(size_t(7), desc2.payloadSize());
    CPPUNIT_ASSERT(ts::ByteBlock(desc2.payload(), desc2.payloadSize()) == ts::ByteBlock(descData + 2, sizeof(descData) - 2));
}

void SectionFileTest::testGenericShortTable()
{
    static const uint8_t refData[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    const ts::SectionPtr refSection(new ts::Section(0xAB, false, refData, sizeof(refData)));
    CPPUNIT_ASSERT(!refSection.isNull());
    CPPUNIT_ASSERT(refSection->isValid());

    ts::BinaryTable refTable;
    refTable.addSection(refSection);
    CPPUNIT_ASSERT(refTable.isValid());
    CPPUNIT_ASSERT_EQUAL(size_t(1), refTable.sectionCount());

    ts::xml::Document doc(report());
    ts::xml::Element* root = doc.initialize(u"test");
    CPPUNIT_ASSERT(root != nullptr);
    CPPUNIT_ASSERT(refTable.toXML(root, true) != nullptr);

    ts::UString text(doc.toString());
    utest::Out() << "SectionFileTest::testGenericShortTable: " << text << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<test>\n"
        u"  <generic_short_table table_id=\"0xAB\" private=\"false\">\n"
        u"    01 02 03 04 05 06\n"
        u"  </generic_short_table>\n"
        u"</test>\n",
        text);

    ts::xml::Document doc2(report());
    CPPUNIT_ASSERT(doc2.parse(text));
    root = doc2.rootElement();
    CPPUNIT_ASSERT(root != nullptr);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"test", root->name());

    ts::xml::ElementVector children;
    CPPUNIT_ASSERT(root->getChildren(children, u"GENERIC_SHORT_TABLE", 1, 1));
    CPPUNIT_ASSERT_EQUAL(size_t(1), children.size());

    ts::BinaryTable tab;
    CPPUNIT_ASSERT(tab.fromXML(children[0]));
    CPPUNIT_ASSERT(tab.isValid());
    CPPUNIT_ASSERT(tab.isShortSection());
    CPPUNIT_ASSERT_EQUAL(ts::TID(0xAB), tab.tableId());
    CPPUNIT_ASSERT_EQUAL(size_t(1), tab.sectionCount());

    ts::SectionPtr sec(tab.sectionAt(0));
    CPPUNIT_ASSERT(!sec.isNull());
    CPPUNIT_ASSERT(sec->isValid());
    CPPUNIT_ASSERT_EQUAL(ts::TID(0xAB), sec->tableId());
    CPPUNIT_ASSERT(sec->isShortSection());
    CPPUNIT_ASSERT(!sec->isPrivateSection());
    CPPUNIT_ASSERT_EQUAL(size_t(6), sec->payloadSize());
    CPPUNIT_ASSERT(ts::ByteBlock(sec->payload(), sec->payloadSize()) == ts::ByteBlock(refData, sizeof(refData)));
}

void SectionFileTest::testGenericLongTable()
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

    ts::xml::Document doc(report());
    ts::xml::Element* root = doc.initialize(u"test");
    CPPUNIT_ASSERT(root != nullptr);
    CPPUNIT_ASSERT(refTable.toXML(root, true) != nullptr);

    ts::UString text(doc.toString());
    utest::Out() << "SectionFileTest::testGenericLongTable: " << text << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<test>\n"
        u"  <generic_long_table table_id=\"0xCD\" table_id_ext=\"0x1234\" version=\"7\" current=\"true\" private=\"true\">\n"
        u"    <section>\n"
        u"      01 02 03 04 05\n"
        u"    </section>\n"
        u"    <section>\n"
        u"      11 12 13 14\n"
        u"    </section>\n"
        u"  </generic_long_table>\n"
        u"</test>\n",
        text);

    ts::xml::Document doc2(report());
    CPPUNIT_ASSERT(doc2.parse(text));
    root = doc2.rootElement();
    CPPUNIT_ASSERT(root != nullptr);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"test", root->name());

    ts::xml::ElementVector children;
    CPPUNIT_ASSERT(root->getChildren(children, u"GENERIC_long_TABLE", 1, 1));
    CPPUNIT_ASSERT_EQUAL(size_t(1), children.size());

    ts::BinaryTable tab;
    CPPUNIT_ASSERT(tab.fromXML(children[0]));
    CPPUNIT_ASSERT(tab.isValid());
    CPPUNIT_ASSERT(!tab.isShortSection());
    CPPUNIT_ASSERT_EQUAL(ts::TID(0xCD), tab.tableId());
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x1234), tab.tableIdExtension());
    CPPUNIT_ASSERT_EQUAL(size_t(2), tab.sectionCount());

    ts::SectionPtr sec(tab.sectionAt(0));
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

    sec = tab.sectionAt(1);
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

void SectionFileTest::testBuildSections()
{
    // Build a PAT with 2 sections.
    ts::PAT pat(7, true, 0x1234);
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_NIT), pat.nit_pid);
    for (uint16_t srv = 3; srv < ts::MAX_PSI_LONG_SECTION_PAYLOAD_SIZE / 4 + 16; ++srv) {
        pat.pmts[srv] = ts::PID(srv + 2);
    }

    // Serialize the PAT.
    ts::BinaryTablePtr patBin(new(ts::BinaryTable));
    CPPUNIT_ASSERT(!patBin.isNull());
    pat.serialize(*patBin);
    CPPUNIT_ASSERT(patBin->isValid());
    CPPUNIT_ASSERT_EQUAL(size_t(2), patBin->sectionCount());

    // Build a section file.
    ts::SectionFile file;
    file.add(patBin);
    CPPUNIT_ASSERT_EQUAL(size_t(1), file.tables().size());
    CPPUNIT_ASSERT_EQUAL(size_t(2), file.sections().size());
    CPPUNIT_ASSERT_EQUAL(size_t(0), file.orphanSections().size());

    file.add(patBin->sectionAt(0));
    CPPUNIT_ASSERT_EQUAL(size_t(1), file.tables().size());
    CPPUNIT_ASSERT_EQUAL(size_t(3), file.sections().size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), file.orphanSections().size());

    file.add(patBin->sectionAt(1));
    CPPUNIT_ASSERT_EQUAL(size_t(2), file.tables().size());
    CPPUNIT_ASSERT_EQUAL(size_t(4), file.sections().size());
    CPPUNIT_ASSERT_EQUAL(size_t(0), file.orphanSections().size());

    // Build a TDT (short section).
    const ts::Time tdtTime(ts::Time::Fields(2017, 12, 25, 14, 55, 27));
    ts::TDT tdt(tdtTime);

    ts::BinaryTablePtr tdtBin(new(ts::BinaryTable));
    CPPUNIT_ASSERT(!tdtBin.isNull());
    tdt.serialize(*tdtBin);
    CPPUNIT_ASSERT(tdtBin->isValid());
    CPPUNIT_ASSERT_EQUAL(size_t(1), tdtBin->sectionCount());

    file.add(tdtBin);
    CPPUNIT_ASSERT_EQUAL(size_t(3), file.tables().size());
    CPPUNIT_ASSERT_EQUAL(size_t(5), file.sections().size());
    CPPUNIT_ASSERT_EQUAL(size_t(0), file.orphanSections().size());

    // Save files.
    utest::Out() << "SectionFileTest::testBuildSections: saving " << _tempFileNameBin << std::endl;
    CPPUNIT_ASSERT(!ts::FileExists(_tempFileNameBin));
    CPPUNIT_ASSERT(file.saveBinary(_tempFileNameBin, report()));
    CPPUNIT_ASSERT(ts::FileExists(_tempFileNameBin));

    utest::Out() << "SectionFileTest::testBuildSections: saving " << _tempFileNameXML << std::endl;
    CPPUNIT_ASSERT(!ts::FileExists(_tempFileNameXML));
    CPPUNIT_ASSERT(file.saveXML(_tempFileNameXML, report()));
    CPPUNIT_ASSERT(ts::FileExists(_tempFileNameXML));

    // Reload files.
    ts::SectionFile binFile;
    binFile.setCRCValidation(ts::CRC32::CHECK);
    CPPUNIT_ASSERT(binFile.loadBinary(_tempFileNameBin, report()));
    CPPUNIT_ASSERT_EQUAL(size_t(3), binFile.tables().size());
    CPPUNIT_ASSERT_EQUAL(size_t(5), binFile.sections().size());
    CPPUNIT_ASSERT_EQUAL(size_t(0), binFile.orphanSections().size());

    ts::SectionFile xmlFile;
    CPPUNIT_ASSERT(xmlFile.loadXML(_tempFileNameXML, report()));
    CPPUNIT_ASSERT_EQUAL(size_t(3), xmlFile.tables().size());
    CPPUNIT_ASSERT_EQUAL(size_t(5), xmlFile.sections().size());
    CPPUNIT_ASSERT_EQUAL(size_t(0), xmlFile.orphanSections().size());

    for (size_t i = 0; i < file.tables().size(); ++i) {
        CPPUNIT_ASSERT(*file.tables()[i] == *binFile.tables()[i]);
        CPPUNIT_ASSERT(*file.tables()[i] == *xmlFile.tables()[i]);
    }
    for (size_t i = 0; i < file.sections().size(); ++i) {
        CPPUNIT_ASSERT(*file.sections()[i] == *binFile.sections()[i]);
        CPPUNIT_ASSERT(*file.sections()[i] == *xmlFile.sections()[i]);
    }

    ts::PAT binPAT(*binFile.tables()[0]);
    CPPUNIT_ASSERT(binPAT.isValid());
    CPPUNIT_ASSERT_EQUAL(uint8_t(7), binPAT.version);
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x1234), binPAT.ts_id);
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_NIT), binPAT.nit_pid);
    CPPUNIT_ASSERT(binPAT.pmts == pat.pmts);

    ts::PAT xmlPAT(*xmlFile.tables()[0]);
    CPPUNIT_ASSERT(xmlPAT.isValid());
    CPPUNIT_ASSERT_EQUAL(uint8_t(7), xmlPAT.version);
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x1234), xmlPAT.ts_id);
    CPPUNIT_ASSERT_EQUAL(ts::PID(ts::PID_NIT), xmlPAT.nit_pid);
    CPPUNIT_ASSERT(xmlPAT.pmts == pat.pmts);

    ts::TDT binTDT(*binFile.tables()[2]);
    CPPUNIT_ASSERT(tdtTime == binTDT.utc_time);

    ts::TDT xmlTDT(*xmlFile.tables()[2]);
    CPPUNIT_ASSERT(tdtTime == xmlTDT.utc_time);
}
