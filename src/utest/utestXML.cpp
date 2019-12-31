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
//  TSUnit test suite for XML classes.
//
//----------------------------------------------------------------------------

#include "tsxmlDocument.h"
#include "tsxmlElement.h"
#include "tsSectionFile.h"
#include "tsTextFormatter.h"
#include "tsCerrReport.h"
#include "tsReportBuffer.h"
#include "tsSysUtils.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class XMLTest: public tsunit::Test
{
public:
    XMLTest();

    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testDocument();
    void testInvalid();
    void testFileBOM();
    void testValidation();
    void testCreation();
    void testKeepOpen();
    void testEscape();
    void testTweaks();
    void testChannels();

    TSUNIT_TEST_BEGIN(XMLTest);
    TSUNIT_TEST(testDocument);
    TSUNIT_TEST(testInvalid);
    TSUNIT_TEST(testFileBOM);
    TSUNIT_TEST(testValidation);
    TSUNIT_TEST(testCreation);
    TSUNIT_TEST(testKeepOpen);
    TSUNIT_TEST(testEscape);
    TSUNIT_TEST(testTweaks);
    TSUNIT_TEST(testChannels);
    TSUNIT_TEST_END();

private:
    ts::UString _tempFileName;
    ts::Report& report();
};

TSUNIT_REGISTER(XMLTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
XMLTest::XMLTest() :
    _tempFileName()
{
}

// Test suite initialization method.
void XMLTest::beforeTest()
{
    if (_tempFileName.empty()) {
        _tempFileName = ts::TempFile(u".tmp.xml");
    }
    ts::DeleteFile(_tempFileName);
}

// Test suite cleanup method.
void XMLTest::afterTest()
{
    ts::DeleteFile(_tempFileName);
}

ts::Report& XMLTest::report()
{
    if (tsunit::Test::debugMode()) {
        return CERR;
    }
    else {
        return NULLREP;
    }
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void XMLTest::testDocument()
{
    static const ts::UChar* const document =
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<root attr1=\"val1\">\n"
        u"  <node1 a1=\"v1\" a2=\"v2\">Text in node1</node1>\n"
        u"  <node2 b1=\"x1\">Text in node2</node2>\n"
        u"  <node3 foo=\"bar\"/>\n"
        u"  <node4/>\n"
        u"</root>\n";

    ts::xml::Document doc(report());
    TSUNIT_ASSERT(doc.parse(document));
    TSUNIT_ASSERT(doc.hasChildren());
    TSUNIT_EQUAL(2, doc.childrenCount());

    ts::xml::Element* root = doc.rootElement();
    TSUNIT_ASSERT(root != nullptr);
    TSUNIT_ASSERT(root->hasChildren());
    TSUNIT_EQUAL(4, root->childrenCount());
    TSUNIT_ASSERT(root->hasAttribute(u"attr1"));
    TSUNIT_ASSERT(root->hasAttribute(u"AttR1"));
    TSUNIT_EQUAL(u"root", root->name());
    TSUNIT_EQUAL(u"val1", root->attribute(u"attr1").value());
    TSUNIT_EQUAL(u"val1", root->attribute(u"AtTr1").value());
    TSUNIT_ASSERT(!root->hasAttribute(u"nonexistent"));
    TSUNIT_ASSERT(!root->attribute(u"nonexistent", true).isValid());
    TSUNIT_ASSERT(root->attribute(u"nonexistent", true).value().empty());
    TSUNIT_ASSERT(root->attribute(u"nonexistent", true).name().empty());

    ts::xml::Element* elem = root->firstChildElement();
    TSUNIT_ASSERT(elem != nullptr);
    TSUNIT_ASSERT(elem->hasChildren());
    TSUNIT_EQUAL(u"node1", elem->name());
    TSUNIT_ASSERT(elem->hasAttribute(u"a1"));
    TSUNIT_EQUAL(u"v1", elem->attribute(u"a1").value());
    TSUNIT_ASSERT(elem->hasAttribute(u"a2"));
    TSUNIT_EQUAL(u"v2", elem->attribute(u"a2").value());
    TSUNIT_EQUAL(u"Text in node1", elem->text());

    elem = elem->nextSiblingElement();
    TSUNIT_ASSERT(elem != nullptr);
    TSUNIT_ASSERT(elem->hasChildren());
    TSUNIT_EQUAL(u"node2", elem->name());
    TSUNIT_EQUAL(u"x1", elem->attribute(u"b1").value());
    TSUNIT_EQUAL(u"Text in node2", elem->text());

    elem = elem->nextSiblingElement();
    TSUNIT_ASSERT(elem != nullptr);
    TSUNIT_ASSERT(!elem->hasChildren());
    TSUNIT_EQUAL(u"node3", elem->name());
    TSUNIT_ASSERT(elem->hasAttribute(u"foo"));
    TSUNIT_EQUAL(u"bar", elem->attribute(u"foo").value());
    TSUNIT_ASSERT(elem->text().empty());

    elem = elem->nextSiblingElement();
    TSUNIT_ASSERT(elem != nullptr);
    TSUNIT_ASSERT(!elem->hasChildren());
    TSUNIT_EQUAL(u"node4", elem->name());
    TSUNIT_ASSERT(!elem->hasAttribute(u"foo"));
    TSUNIT_ASSERT(elem->text().empty());

    elem = elem->nextSiblingElement();
    TSUNIT_ASSERT(elem == nullptr);
}

void XMLTest::testInvalid()
{
    // Incorrect XML document
    static const ts::UChar* xmlContent =
        u"<?xml version='1.0' encoding='UTF-8'?>\n"
        u"<foo>\n"
        u"</bar>";

    ts::ReportBuffer<> rep;
    ts::xml::Document doc(rep);
    TSUNIT_ASSERT(!doc.parse(xmlContent));
    TSUNIT_EQUAL(u"Error: line 3: parsing error, expected </foo> to match <foo> at line 2", rep.getMessages());
}

void XMLTest::testFileBOM()
{
    // Binary content of XML file with BOM, accented characters and HTML entities.
    const ts::ByteBlock fileData({
        0xEF, 0xBB, 0xBF, 0x3C, 0x3F, 0x78, 0x6D, 0x6C, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E,
        0x3D, 0x27, 0x31, 0x2E, 0x30, 0x27, 0x20, 0x65, 0x6E, 0x63, 0x6F, 0x64, 0x69, 0x6E, 0x67, 0x3D,
        0x27, 0x55, 0x54, 0x46, 0x2D, 0x38, 0x27, 0x3F, 0x3E, 0x0A, 0x3C, 0x66, 0x6F, 0x6F, 0x3E, 0x0A,
        0x20, 0x20, 0x3C, 0x62, 0xC3, 0xA0, 0x41, 0xC3, 0xA7, 0x20, 0x66, 0xC3, 0xB9, 0x3D, 0x22, 0x63,
        0xC3, 0xA9, 0x22, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x66, 0x26, 0x6C, 0x74, 0x3B, 0x26, 0x67,
        0x74, 0x3B, 0x0A, 0x20, 0x20, 0x3C, 0x2F, 0x42, 0xC3, 0x80, 0x41, 0xC3, 0x87, 0x3E, 0x0A, 0x3C,
        0x2F, 0x66, 0x6F, 0x6F, 0x3E, 0x0A,
    });

    const ts::UString rootName(u"foo");
    const ts::UString childName({u'b', ts::LATIN_SMALL_LETTER_A_WITH_GRAVE, u'A', ts::LATIN_SMALL_LETTER_C_WITH_CEDILLA});
    const ts::UString childAttrName({u'f', ts::LATIN_SMALL_LETTER_U_WITH_GRAVE});
    const ts::UString childAttrValue({u'c', ts::LATIN_SMALL_LETTER_E_WITH_ACUTE});
    const ts::UString childText1(u"\n    f<>\n  ");
    const ts::UString childText2(u"f<>");

    TSUNIT_ASSERT(fileData.saveToFile(_tempFileName, &report()));

    ts::xml::Document doc(report());
    TSUNIT_ASSERT(doc.load(_tempFileName));

    ts::xml::Element* root = doc.rootElement();
    TSUNIT_EQUAL(2, doc.childrenCount());
    TSUNIT_ASSERT(root != nullptr);
    TSUNIT_EQUAL(1, root->childrenCount());
    TSUNIT_EQUAL(rootName, root->name());

    ts::xml::Element* elem = root->firstChildElement();
    TSUNIT_ASSERT(elem != nullptr);
    TSUNIT_EQUAL(childName, elem->name());
    TSUNIT_EQUAL(childAttrName, elem->attribute(childAttrName).name());
    TSUNIT_EQUAL(childAttrValue, elem->attribute(childAttrName).value());
    TSUNIT_EQUAL(childText1, elem->text(false));
    TSUNIT_EQUAL(childText2, elem->text(true));

    TSUNIT_EQUAL(ts::SYS_SUCCESS, ts::DeleteFile(_tempFileName));
}

void XMLTest::testValidation()
{
    ts::xml::Document model(report());
    TSUNIT_ASSERT(model.load(TS_XML_TABLES_MODEL));

    const ts::UString xmlContent(
        u"<?xml version='1.0' encoding='UTF-8'?>\n"
        u"<tsduck>\n"
        u"  <PAT version='2' transport_stream_id='27'>\n"
        u"    <service service_id='1' program_map_PID='1000'/>\n"
        u"    <service service_id='2' program_map_PID='2000'/>\n"
        u"    <service service_id='3' program_map_PID='3000'/>\n"
        u"  </PAT>\n"
        u"  <PMT version='3' service_id='789' PCR_PID='3004'>\n"
        u"    <CA_descriptor CA_system_id='500' CA_PID='3005'>\n"
        u"      <private_data>00 01 02 03 04</private_data>\n"
        u"    </CA_descriptor>\n"
        u"    <component stream_type='0x04' elementary_PID='3006'>\n"
        u"      <ca_descriptor ca_system_id='500' ca_PID='3007'>\n"
        u"        <private_data>10 11 12 13 14 15</private_data>\n"
        u"      </ca_descriptor>\n"
        u"    </component>\n"
        u"  </PMT>\n"
        u"</tsduck>");

    ts::xml::Document doc(report());
    TSUNIT_ASSERT(doc.parse(xmlContent));
    TSUNIT_ASSERT(doc.validate(model));
}

void XMLTest::testCreation()
{
    ts::xml::Document doc(report());
    ts::xml::Element* child1 = nullptr;
    ts::xml::Element* child2 = nullptr;
    ts::xml::Element* subchild2 = nullptr;

    ts::xml::Element* root = doc.initialize(u"theRoot");
    TSUNIT_ASSERT(root != nullptr);
    TSUNIT_EQUAL(0, doc.depth());
    TSUNIT_EQUAL(1, root->depth());

    TSUNIT_ASSERT((child1 = root->addElement(u"child1")) != nullptr);
    TSUNIT_EQUAL(2, child1->depth());
    child1->setAttribute(u"str", u"a string");
    child1->setIntAttribute(u"int", -47);
    TSUNIT_ASSERT(child1->addElement(u"subChild1") != nullptr);
    TSUNIT_ASSERT((subchild2 = child1->addElement(u"subChild2")) != nullptr);
    subchild2->setIntAttribute(u"int64", TS_CONST64(0x7FFFFFFFFFFFFFFF));

    TSUNIT_ASSERT((child2 = root->addElement(u"child2")) != nullptr);
    TSUNIT_ASSERT(child2->addElement(u"fooBar") != nullptr);

    ts::UString str;
    TSUNIT_ASSERT(child1->getAttribute(str, u"str", true));
    TSUNIT_EQUAL(u"a string", str);

    int i;
    TSUNIT_ASSERT(child1->getIntAttribute(i, u"int", true));
    TSUNIT_EQUAL(-47, i);

    int64_t i64;
    TSUNIT_ASSERT(subchild2->getIntAttribute(i64, u"int64", true));
    TSUNIT_EQUAL(TS_CONST64(0x7FFFFFFFFFFFFFFF), i64);

    TSUNIT_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<theRoot>\n"
        u"  <child1 str=\"a string\" int=\"-47\">\n"
        u"    <subChild1/>\n"
        u"    <subChild2 int64=\"9,223,372,036,854,775,807\"/>\n"
        u"  </child1>\n"
        u"  <child2>\n"
        u"    <fooBar/>\n"
        u"  </child2>\n"
        u"</theRoot>\n",
        doc.toString());
}

void XMLTest::testKeepOpen()
{
    static const ts::UChar* const document =
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<root attr1=\"val1\">\n"
        u"  <node1>  Text in node1  </node1>\n"
        u"  <node2>\n"
        u"    <node21>\n"
        u"      <node211/>\n"
        u"    </node21>\n"
        u"    <node22/>\n"
        u"  </node2>\n"
        u"  <node3 foo=\"bar\"/>\n"
        u"  <node4/>\n"
        u"</root>\n";

    ts::xml::Document doc(report());
    TSUNIT_ASSERT(doc.parse(document));

    ts::xml::Element* root = doc.rootElement();
    TSUNIT_ASSERT(root != nullptr);

    ts::xml::Element* node2 = root->findFirstChild(u"NODE2");
    TSUNIT_ASSERT(node2 != nullptr);
    TSUNIT_EQUAL(u"node2", node2->name());

    ts::TextFormatter out(report());
    node2->print(out.setString());
    TSUNIT_EQUAL(
        u"<node2>\n"
        u"  <node21>\n"
        u"    <node211/>\n"
        u"  </node21>\n"
        u"  <node22/>\n"
        u"</node2>",
        out.toString());

    node2->print(out.setString(), true);
    TSUNIT_EQUAL(
        u"<node2>\n"
        u"  <node21>\n"
        u"    <node211/>\n"
        u"  </node21>\n"
        u"  <node22/>\n",
        out.toString());

    node2->printClose(out, 1);
    TSUNIT_EQUAL(
        u"<node2>\n"
        u"  <node21>\n"
        u"    <node211/>\n"
        u"  </node21>\n"
        u"  <node22/>\n"
        u"</node2>\n",
        out.toString());
}

void XMLTest::testEscape()
{
    ts::xml::Document doc(report());
    ts::xml::Element* child1 = nullptr;
    ts::xml::Element* child2 = nullptr;

    ts::xml::Element* root = doc.initialize(u"theRoot");
    TSUNIT_ASSERT(root != nullptr);
    TSUNIT_EQUAL(0, doc.depth());
    TSUNIT_EQUAL(1, root->depth());

    TSUNIT_ASSERT((child1 = root->addElement(u"child1")) != nullptr);
    TSUNIT_EQUAL(2, child1->depth());
    child1->setAttribute(u"str", u"ab&<>'\"cd");

    TSUNIT_ASSERT((child2 = root->addElement(u"child2")) != nullptr);
    TSUNIT_ASSERT(child2->addText(u"text<&'\">text") != nullptr);

    const ts::UString text(doc.toString());
    TSUNIT_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<theRoot>\n"
        u"  <child1 str=\"ab&amp;&lt;&gt;&apos;&quot;cd\"/>\n"
        u"  <child2>text&lt;&amp;'\"&gt;text</child2>\n"
        u"</theRoot>\n",
        text);

    ts::xml::Document doc2(report());
    TSUNIT_ASSERT(doc2.parse(text));
    TSUNIT_ASSERT(doc2.hasChildren());
    TSUNIT_EQUAL(2, doc2.childrenCount());

    ts::xml::Element* root2 = doc2.rootElement();
    TSUNIT_ASSERT(root2 != nullptr);
    TSUNIT_ASSERT(root2->hasChildren());
    TSUNIT_EQUAL(2, root2->childrenCount());
    TSUNIT_EQUAL(u"theRoot", root2->name());

    ts::xml::Element* elem = root2->firstChildElement();
    TSUNIT_ASSERT(elem != nullptr);
    TSUNIT_EQUAL(u"child1", elem->name());
    TSUNIT_ASSERT(elem->hasAttribute(u"str"));
    TSUNIT_EQUAL(u"ab&<>'\"cd", elem->attribute(u"str").value());

    elem = elem->nextSiblingElement();
    TSUNIT_ASSERT(elem != nullptr);
    TSUNIT_ASSERT(elem->hasChildren());
    TSUNIT_EQUAL(u"child2", elem->name());
    TSUNIT_EQUAL(u"text<&'\">text", elem->text());
}

void XMLTest::testTweaks()
{
    ts::xml::Document doc(report());
    ts::xml::Element* root = doc.initialize(u"root");
    TSUNIT_ASSERT(root != nullptr);
    root->setAttribute(u"a1", u"foo");
    root->setAttribute(u"a2", u"ab&<>'\"cd");
    root->setAttribute(u"a3", u"ef\"gh");
    root->setAttribute(u"a4", u"ij'kl");
    TSUNIT_ASSERT(root->addText(u"text<&'\">text") != nullptr);

    ts::xml::Tweaks tweaks; // default values
    doc.setTweaks(tweaks);

    TSUNIT_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<root a1=\"foo\" a2=\"ab&amp;&lt;&gt;&apos;&quot;cd\" a3=\"ef&quot;gh\" a4=\"ij&apos;kl\">text&lt;&amp;'\"&gt;text</root>\n",
        doc.toString());

    tweaks.strictAttributeFormatting = true;
    tweaks.strictTextNodeFormatting = true;
    doc.setTweaks(tweaks);

    TSUNIT_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<root a1=\"foo\" a2=\"ab&amp;&lt;&gt;&apos;&quot;cd\" a3=\"ef&quot;gh\" a4=\"ij&apos;kl\">text&lt;&amp;&apos;&quot;&gt;text</root>\n",
        doc.toString());

    tweaks.strictAttributeFormatting = false;
    tweaks.strictTextNodeFormatting = true;
    doc.setTweaks(tweaks);

    TSUNIT_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<root a1=\"foo\" a2=\"ab&amp;<>'&quot;cd\" a3='ef\"gh' a4=\"ij'kl\">text&lt;&amp;&apos;&quot;&gt;text</root>\n",
        doc.toString());

    tweaks.strictAttributeFormatting = false;
    tweaks.strictTextNodeFormatting = false;
    doc.setTweaks(tweaks);

    TSUNIT_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<root a1=\"foo\" a2=\"ab&amp;<>'&quot;cd\" a3='ef\"gh' a4=\"ij'kl\">text&lt;&amp;'\"&gt;text</root>\n",
        doc.toString());
}

void XMLTest::testChannels()
{
    ts::xml::Document model(report());
    TSUNIT_ASSERT(model.load(TS_XML_TABLES_MODEL));
}
