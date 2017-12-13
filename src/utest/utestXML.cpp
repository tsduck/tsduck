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
//  CppUnit test suite for embedded TinyXML
//
//----------------------------------------------------------------------------

#include "tsXML.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"
#include "tsCerrReport.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class XMLTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testDocument();
    void testInvalid();
    void testValidation();
    void testCreation();

    CPPUNIT_TEST_SUITE(XMLTest);
    CPPUNIT_TEST(testDocument);
    CPPUNIT_TEST(testInvalid);
    CPPUNIT_TEST(testValidation);
    CPPUNIT_TEST(testCreation);
    CPPUNIT_TEST_SUITE_END();

private:
    ts::Report& report();
};

CPPUNIT_TEST_SUITE_REGISTRATION(XMLTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void XMLTest::setUp()
{
}

// Test suite cleanup method.
void XMLTest::tearDown()
{
}

ts::Report& XMLTest::report()
{
    if (utest::DebugMode()) {
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
    CPPUNIT_ASSERT(doc.parse(document));
    CPPUNIT_ASSERT(doc.hasChildren());
    CPPUNIT_ASSERT_EQUAL(size_t(2), doc.childrenCount());

    ts::xml::Element* root = doc.rootElement();
    CPPUNIT_ASSERT(root != 0);
    CPPUNIT_ASSERT(root->hasChildren());
    CPPUNIT_ASSERT_EQUAL(size_t(4), root->childrenCount());
    CPPUNIT_ASSERT(root->hasAttribute(u"attr1"));
    CPPUNIT_ASSERT(root->hasAttribute(u"AttR1"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"root", root->name());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"val1", root->attribute(u"attr1").value());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"val1", root->attribute(u"AtTr1").value());
    CPPUNIT_ASSERT(!root->hasAttribute(u"nonexistent"));
    CPPUNIT_ASSERT(!root->attribute(u"nonexistent").isValid());
    CPPUNIT_ASSERT(root->attribute(u"nonexistent").value().empty());
    CPPUNIT_ASSERT(root->attribute(u"nonexistent").name().empty());

    ts::xml::Element* elem = root->firstChildElement();
    CPPUNIT_ASSERT(elem != 0);
    CPPUNIT_ASSERT(elem->hasChildren());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"node1", elem->name());
    CPPUNIT_ASSERT(elem->hasAttribute(u"a1"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"v1", elem->attribute(u"a1").value());
    CPPUNIT_ASSERT(elem->hasAttribute(u"a2"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"v2", elem->attribute(u"a2").value());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Text in node1", elem->text());

    elem = elem->nextSiblingElement();
    CPPUNIT_ASSERT(elem != 0);
    CPPUNIT_ASSERT(elem->hasChildren());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"node2", elem->name());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"x1", elem->attribute(u"b1").value());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Text in node2", elem->text());

    elem = elem->nextSiblingElement();
    CPPUNIT_ASSERT(elem != 0);
    CPPUNIT_ASSERT(!elem->hasChildren());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"node3", elem->name());
    CPPUNIT_ASSERT(elem->hasAttribute(u"foo"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"bar", elem->attribute(u"foo").value());
    CPPUNIT_ASSERT(elem->text().empty());

    elem = elem->nextSiblingElement();
    CPPUNIT_ASSERT(elem != 0);
    CPPUNIT_ASSERT(!elem->hasChildren());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"node4", elem->name());
    CPPUNIT_ASSERT(!elem->hasAttribute(u"foo"));
    CPPUNIT_ASSERT(elem->text().empty());

    elem = elem->nextSiblingElement();
    CPPUNIT_ASSERT(elem == 0);
}

void XMLTest::testInvalid()
{
    // Incorrect XML document
    static const char* xmlContent =
        "<?xml version='1.0' encoding='UTF-8'?>\n"
        "<foo>\n"
        "</bar>";

    ts::XML::Document doc;
    CPPUNIT_ASSERT_EQUAL(tinyxml2::XML_ERROR_MISMATCHED_ELEMENT, doc.Parse(xmlContent));
    CPPUNIT_ASSERT_STRINGS_EQUAL("foo", doc.GetErrorStr1());
    CPPUNIT_ASSERT_STRINGS_EQUAL("", doc.GetErrorStr2());
}

void XMLTest::testValidation()
{
    ts::XML xml(CERR);

    ts::XML::Document model;
    CPPUNIT_ASSERT(xml.loadDocument(model, u"tsduck.xml"));

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

    ts::XML::Document doc;
    CPPUNIT_ASSERT(xml.parseDocument(doc, xmlContent));
    CPPUNIT_ASSERT(xml.validateDocument(model, doc));
}

void XMLTest::testCreation()
{
    ts::XML xml(CERR);
    ts::XML::Document doc;
    ts::XML::Element* e1 = 0;
    ts::XML::Element* e2 = 0;

    ts::XML::Element* root = xml.initializeDocument(&doc, u"theRoot");
    CPPUNIT_ASSERT(root != 0);
    CPPUNIT_ASSERT_EQUAL(0, ts::XML::NodeDepth(&doc));
    CPPUNIT_ASSERT_EQUAL(1, ts::XML::NodeDepth(root));

    CPPUNIT_ASSERT((e1 = xml.addElement(root, u"child1")) != 0);
    CPPUNIT_ASSERT_EQUAL(2, ts::XML::NodeDepth(e1));
    e1->SetAttribute("str", "a string");
    e1->SetAttribute("int", -47);
    CPPUNIT_ASSERT(xml.addElement(e1, u"subChild1") != 0);
    CPPUNIT_ASSERT((e2 = xml.addElement(e1, u"subChild2")) != 0);
    e2->SetAttribute("int64", TS_CONST64(0x7FFFFFFFFFFFFFFF));
    CPPUNIT_ASSERT((e2 = xml.addElement(root, u"child2")) != 0);
    CPPUNIT_ASSERT(xml.addElement(e2, u"fooBar") != 0);

    ts::UString text(xml.toString(doc));
    utest::Out() << "XMLTest::testCreation: " << text << std::endl;

    CPPUNIT_ASSERT_USTRINGS_EQUAL(
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<theRoot>\n"
        u"  <child1 str=\"a string\" int=\"-47\">\n"
        u"    <subChild1/>\n"
        u"    <subChild2 int64=\"9223372036854775807\"/>\n"
        u"  </child1>\n"
        u"  <child2>\n"
        u"    <fooBar/>\n"
        u"  </child2>\n"
        u"</theRoot>\n",
        text);
}
