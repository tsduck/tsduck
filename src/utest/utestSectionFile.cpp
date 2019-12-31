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
//  TSUnit test suite for SectionFile (binary and XML).
//
//----------------------------------------------------------------------------

#include "tsSectionFile.h"
#include "tsPAT.h"
#include "tsTDT.h"
#include "tsSysUtils.h"
#include "tsBinaryTable.h"
#include "tsCerrReport.h"
#include "tsunit.h"
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

class SectionFileTest: public tsunit::Test
{
public:
    SectionFileTest();

    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testConfigurationFile();
    void testGenericDescriptor();
    void testGenericShortTable();
    void testGenericLongTable();
    void testPAT1();
    void testSCTE35();
    void testAllTables();
    void testBuildSections();

    TSUNIT_TEST_BEGIN(SectionFileTest);
    TSUNIT_TEST(testConfigurationFile);
    TSUNIT_TEST(testGenericDescriptor);
    TSUNIT_TEST(testGenericShortTable);
    TSUNIT_TEST(testGenericLongTable);
    TSUNIT_TEST(testPAT1);
    TSUNIT_TEST(testSCTE35);
    TSUNIT_TEST(testAllTables);
    TSUNIT_TEST(testBuildSections);
    TSUNIT_TEST_END();

private:
    // Unitary test for one table.
    void testTable(const char* name, const ts::UChar* ref_xml, const uint8_t* ref_sections, size_t ref_sections_size);
    ts::Report& report();
    ts::UString _tempFileNameBin;
    ts::UString _tempFileNameXML;
};

TSUNIT_REGISTER(SectionFileTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
SectionFileTest::SectionFileTest() :
    _tempFileNameBin(),
    _tempFileNameXML()
{
}

// Test suite initialization method.
void SectionFileTest::beforeTest()
{
    if (_tempFileNameBin.empty() || _tempFileNameXML.empty()) {
        _tempFileNameBin = ts::TempFile(u".tmp.bin");
        _tempFileNameXML = ts::TempFile(u".tmp.xml");
    }
    ts::DeleteFile(_tempFileNameBin);
    ts::DeleteFile(_tempFileNameXML);
}

// Test suite cleanup method.
void SectionFileTest::afterTest()
{
    ts::DeleteFile(_tempFileNameBin);
    ts::DeleteFile(_tempFileNameXML);
}

ts::Report& SectionFileTest::report()
{
    if (tsunit::Test::debugMode()) {
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
    debug() << "SectionFileTest: Testing " << name << std::endl;

    // Convert XML reference content to binary tables.
    ts::DuckContext duck;
    ts::SectionFile xml(duck);
    TSUNIT_ASSERT(xml.parseXML(ref_xml, CERR));

    // Serialize binary tables to section data.
    std::ostringstream strm;
    TSUNIT_ASSERT(xml.saveBinary(strm, CERR));

    // Compare serialized section data to reference section data.
    const std::string sections(strm.str());
    TSUNIT_EQUAL(ref_sections_size, sections.size());
    TSUNIT_EQUAL(0, ::memcmp(ref_sections, sections.data(), ref_sections_size));

    // Convert binary tables to XML.
    TSUNIT_EQUAL(ref_xml, xml.toXML(CERR));
}


//----------------------------------------------------------------------------
// Other unitary tests.
//----------------------------------------------------------------------------

void SectionFileTest::testConfigurationFile()
{
    const ts::UString conf(ts::SearchConfigurationFile(TS_XML_TABLES_MODEL));
    debug() << "SectionFileTest::testConfigurationFile: " << conf << std::endl;
    TSUNIT_ASSERT(ts::FileExists(conf));
}

void SectionFileTest::testGenericDescriptor()
{
    static const uint8_t descData[] = {
        0x72,   // tag
        0x07,   // length
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };
    ts::Descriptor desc(descData, sizeof(descData));
    TSUNIT_ASSERT(desc.isValid());
    TSUNIT_EQUAL(0x72, desc.tag());
    TSUNIT_EQUAL(9, desc.size());
    TSUNIT_EQUAL(7, desc.payloadSize());

    ts::DuckContext duck;
    ts::xml::Document doc(report());
    ts::xml::Element* root = doc.initialize(u"test");
    TSUNIT_ASSERT(root != nullptr);
    TSUNIT_ASSERT(desc.toXML(duck, root, 0, ts::TID_NULL, true) != nullptr);

    ts::UString text(doc.toString());
    debug() << "SectionFileTest::testGenericDescriptor: " << text << std::endl;
    TSUNIT_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<test>\n"
        u"  <generic_descriptor tag=\"0x72\">\n"
        u"    01 02 03 04 05 06 07\n"
        u"  </generic_descriptor>\n"
        u"</test>\n",
        text);

    ts::xml::Document doc2(report());
    TSUNIT_ASSERT(doc2.parse(text));
    root = doc2.rootElement();
    TSUNIT_ASSERT(root != nullptr);
    TSUNIT_EQUAL(u"test", root->name());

    ts::xml::ElementVector children;
    TSUNIT_ASSERT(root->getChildren(children, u"generic_descriptor", 1, 1));
    TSUNIT_EQUAL(1, children.size());

    ts::ByteBlock payload;
    TSUNIT_ASSERT(children[0]->getHexaText(payload));
    TSUNIT_EQUAL(7, payload.size());
    TSUNIT_ASSERT(payload == ts::ByteBlock(descData + 2, sizeof(descData) - 2));

    ts::Descriptor desc2;
    TSUNIT_ASSERT(desc2.fromXML(duck, children[0]));
    TSUNIT_EQUAL(0x72, desc2.tag());
    TSUNIT_EQUAL(7, desc2.payloadSize());
    TSUNIT_ASSERT(ts::ByteBlock(desc2.payload(), desc2.payloadSize()) == ts::ByteBlock(descData + 2, sizeof(descData) - 2));
}

void SectionFileTest::testGenericShortTable()
{
    static const uint8_t refData[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    const ts::SectionPtr refSection(new ts::Section(0xAB, false, refData, sizeof(refData)));
    TSUNIT_ASSERT(!refSection.isNull());
    TSUNIT_ASSERT(refSection->isValid());

    ts::DuckContext duck;
    ts::BinaryTable refTable;
    refTable.addSection(refSection);
    TSUNIT_ASSERT(refTable.isValid());
    TSUNIT_EQUAL(1, refTable.sectionCount());

    ts::xml::Document doc(report());
    ts::xml::Element* root = doc.initialize(u"test");
    TSUNIT_ASSERT(root != nullptr);
    TSUNIT_ASSERT(refTable.toXML(duck, root, true) != nullptr);

    ts::UString text(doc.toString());
    debug() << "SectionFileTest::testGenericShortTable: " << text << std::endl;
    TSUNIT_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<test>\n"
        u"  <generic_short_table table_id=\"0xAB\" private=\"false\">\n"
        u"    01 02 03 04 05 06\n"
        u"  </generic_short_table>\n"
        u"</test>\n",
        text);

    ts::xml::Document doc2(report());
    TSUNIT_ASSERT(doc2.parse(text));
    root = doc2.rootElement();
    TSUNIT_ASSERT(root != nullptr);
    TSUNIT_EQUAL(u"test", root->name());

    ts::xml::ElementVector children;
    TSUNIT_ASSERT(root->getChildren(children, u"GENERIC_SHORT_TABLE", 1, 1));
    TSUNIT_EQUAL(1, children.size());

    ts::BinaryTable tab;
    TSUNIT_ASSERT(tab.fromXML(duck, children[0]));
    TSUNIT_ASSERT(tab.isValid());
    TSUNIT_ASSERT(tab.isShortSection());
    TSUNIT_EQUAL(0xAB, tab.tableId());
    TSUNIT_EQUAL(1, tab.sectionCount());

    ts::SectionPtr sec(tab.sectionAt(0));
    TSUNIT_ASSERT(!sec.isNull());
    TSUNIT_ASSERT(sec->isValid());
    TSUNIT_EQUAL(0xAB, sec->tableId());
    TSUNIT_ASSERT(sec->isShortSection());
    TSUNIT_ASSERT(!sec->isPrivateSection());
    TSUNIT_EQUAL(6, sec->payloadSize());
    TSUNIT_ASSERT(ts::ByteBlock(sec->payload(), sec->payloadSize()) == ts::ByteBlock(refData, sizeof(refData)));
}

void SectionFileTest::testGenericLongTable()
{
    static const uint8_t refData0[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    static const uint8_t refData1[] = {0x11, 0x12, 0x13, 0x14};

    ts::DuckContext duck;
    ts::BinaryTable refTable;
    refTable.addSection(new ts::Section(0xCD, true, 0x1234, 7, true, 0, 0, refData0, sizeof(refData0)));
    refTable.addSection(new ts::Section(0xCD, true, 0x1234, 7, true, 1, 1, refData1, sizeof(refData1)));
    TSUNIT_ASSERT(refTable.isValid());
    TSUNIT_ASSERT(!refTable.isShortSection());
    TSUNIT_EQUAL(0xCD, refTable.tableId());
    TSUNIT_EQUAL(0x1234, refTable.tableIdExtension());
    TSUNIT_EQUAL(2, refTable.sectionCount());

    ts::xml::Document doc(report());
    ts::xml::Element* root = doc.initialize(u"test");
    TSUNIT_ASSERT(root != nullptr);
    TSUNIT_ASSERT(refTable.toXML(duck, root, true) != nullptr);

    ts::UString text(doc.toString());
    debug() << "SectionFileTest::testGenericLongTable: " << text << std::endl;
    TSUNIT_EQUAL(
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
    TSUNIT_ASSERT(doc2.parse(text));
    root = doc2.rootElement();
    TSUNIT_ASSERT(root != nullptr);
    TSUNIT_EQUAL(u"test", root->name());

    ts::xml::ElementVector children;
    TSUNIT_ASSERT(root->getChildren(children, u"GENERIC_long_TABLE", 1, 1));
    TSUNIT_EQUAL(1, children.size());

    ts::BinaryTable tab;
    TSUNIT_ASSERT(tab.fromXML(duck, children[0]));
    TSUNIT_ASSERT(tab.isValid());
    TSUNIT_ASSERT(!tab.isShortSection());
    TSUNIT_EQUAL(0xCD, tab.tableId());
    TSUNIT_EQUAL(0x1234, tab.tableIdExtension());
    TSUNIT_EQUAL(2, tab.sectionCount());

    ts::SectionPtr sec(tab.sectionAt(0));
    TSUNIT_ASSERT(!sec.isNull());
    TSUNIT_ASSERT(sec->isValid());
    TSUNIT_EQUAL(0xCD, sec->tableId());
    TSUNIT_EQUAL(0x1234, sec->tableIdExtension());
    TSUNIT_EQUAL(7, sec->version());
    TSUNIT_ASSERT(!sec->isShortSection());
    TSUNIT_ASSERT(sec->isPrivateSection());
    TSUNIT_ASSERT(sec->isCurrent());
    TSUNIT_EQUAL(sizeof(refData0), sec->payloadSize());
    TSUNIT_ASSERT(ts::ByteBlock(sec->payload(), sec->payloadSize()) == ts::ByteBlock(refData0, sizeof(refData0)));

    sec = tab.sectionAt(1);
    TSUNIT_ASSERT(!sec.isNull());
    TSUNIT_ASSERT(sec->isValid());
    TSUNIT_EQUAL(0xCD, sec->tableId());
    TSUNIT_EQUAL(0x1234, sec->tableIdExtension());
    TSUNIT_EQUAL(7, sec->version());
    TSUNIT_ASSERT(!sec->isShortSection());
    TSUNIT_ASSERT(sec->isPrivateSection());
    TSUNIT_ASSERT(sec->isCurrent());
    TSUNIT_EQUAL(sizeof(refData1), sec->payloadSize());
    TSUNIT_ASSERT(ts::ByteBlock(sec->payload(), sec->payloadSize()) == ts::ByteBlock(refData1, sizeof(refData1)));
}

void SectionFileTest::testBuildSections()
{
    ts::DuckContext duck;

    // Build a PAT with 2 sections.
    ts::PAT pat(7, true, 0x1234);
    TSUNIT_EQUAL(ts::PID_NIT, pat.nit_pid);
    for (uint16_t srv = 3; srv < ts::MAX_PSI_LONG_SECTION_PAYLOAD_SIZE / 4 + 16; ++srv) {
        pat.pmts[srv] = ts::PID(srv + 2);
    }

    // Serialize the PAT.
    ts::BinaryTablePtr patBin(new(ts::BinaryTable));
    TSUNIT_ASSERT(!patBin.isNull());
    pat.serialize(duck, *patBin);
    TSUNIT_ASSERT(patBin->isValid());
    TSUNIT_EQUAL(2, patBin->sectionCount());

    // Build a section file.
    ts::SectionFile file(duck);
    file.add(patBin);
    TSUNIT_EQUAL(1, file.tables().size());
    TSUNIT_EQUAL(2, file.sections().size());
    TSUNIT_EQUAL(0, file.orphanSections().size());

    file.add(patBin->sectionAt(0));
    TSUNIT_EQUAL(1, file.tables().size());
    TSUNIT_EQUAL(3, file.sections().size());
    TSUNIT_EQUAL(1, file.orphanSections().size());

    file.add(patBin->sectionAt(1));
    TSUNIT_EQUAL(2, file.tables().size());
    TSUNIT_EQUAL(4, file.sections().size());
    TSUNIT_EQUAL(0, file.orphanSections().size());

    // Build a TDT (short section).
    const ts::Time tdtTime(ts::Time::Fields(2017, 12, 25, 14, 55, 27));
    ts::TDT tdt(tdtTime);

    ts::BinaryTablePtr tdtBin(new(ts::BinaryTable));
    TSUNIT_ASSERT(!tdtBin.isNull());
    tdt.serialize(duck, *tdtBin);
    TSUNIT_ASSERT(tdtBin->isValid());
    TSUNIT_EQUAL(1, tdtBin->sectionCount());

    file.add(tdtBin);
    TSUNIT_EQUAL(3, file.tables().size());
    TSUNIT_EQUAL(5, file.sections().size());
    TSUNIT_EQUAL(0, file.orphanSections().size());

    // Save files.
    debug() << "SectionFileTest::testBuildSections: saving " << _tempFileNameBin << std::endl;
    TSUNIT_ASSERT(!ts::FileExists(_tempFileNameBin));
    TSUNIT_ASSERT(file.saveBinary(_tempFileNameBin, report()));
    TSUNIT_ASSERT(ts::FileExists(_tempFileNameBin));

    debug() << "SectionFileTest::testBuildSections: saving " << _tempFileNameXML << std::endl;
    TSUNIT_ASSERT(!ts::FileExists(_tempFileNameXML));
    TSUNIT_ASSERT(file.saveXML(_tempFileNameXML, report()));
    TSUNIT_ASSERT(ts::FileExists(_tempFileNameXML));

    // Reload files.
    ts::SectionFile binFile(duck);
    binFile.setCRCValidation(ts::CRC32::CHECK);
    TSUNIT_ASSERT(binFile.loadBinary(_tempFileNameBin, report()));
    TSUNIT_EQUAL(3, binFile.tables().size());
    TSUNIT_EQUAL(5, binFile.sections().size());
    TSUNIT_EQUAL(0, binFile.orphanSections().size());

    ts::SectionFile xmlFile(duck);
    TSUNIT_ASSERT(xmlFile.loadXML(_tempFileNameXML, report()));
    TSUNIT_EQUAL(3, xmlFile.tables().size());
    TSUNIT_EQUAL(5, xmlFile.sections().size());
    TSUNIT_EQUAL(0, xmlFile.orphanSections().size());

    for (size_t i = 0; i < file.tables().size(); ++i) {
        TSUNIT_ASSERT(*file.tables()[i] == *binFile.tables()[i]);
        TSUNIT_ASSERT(*file.tables()[i] == *xmlFile.tables()[i]);
    }
    for (size_t i = 0; i < file.sections().size(); ++i) {
        TSUNIT_ASSERT(*file.sections()[i] == *binFile.sections()[i]);
        TSUNIT_ASSERT(*file.sections()[i] == *xmlFile.sections()[i]);
    }

    ts::PAT binPAT(duck, *binFile.tables()[0]);
    TSUNIT_ASSERT(binPAT.isValid());
    TSUNIT_EQUAL(7, binPAT.version);
    TSUNIT_EQUAL(0x1234, binPAT.ts_id);
    TSUNIT_EQUAL(ts::PID_NIT, binPAT.nit_pid);
    TSUNIT_ASSERT(binPAT.pmts == pat.pmts);

    ts::PAT xmlPAT(duck, *xmlFile.tables()[0]);
    TSUNIT_ASSERT(xmlPAT.isValid());
    TSUNIT_EQUAL(7, xmlPAT.version);
    TSUNIT_EQUAL(0x1234, xmlPAT.ts_id);
    TSUNIT_EQUAL(ts::PID_NIT, xmlPAT.nit_pid);
    TSUNIT_ASSERT(xmlPAT.pmts == pat.pmts);

    ts::TDT binTDT(duck, *binFile.tables()[2]);
    TSUNIT_ASSERT(tdtTime == binTDT.utc_time);

    ts::TDT xmlTDT(duck, *xmlFile.tables()[2]);
    TSUNIT_ASSERT(tdtTime == xmlTDT.utc_time);
}
