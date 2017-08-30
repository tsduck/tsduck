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
#include "tsStringUtils.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class XMLTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testDocument();
    void testVisitor();

    CPPUNIT_TEST_SUITE(XMLTest);
    CPPUNIT_TEST(testDocument);
    CPPUNIT_TEST(testVisitor);
    CPPUNIT_TEST_SUITE_END();
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


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void XMLTest::testDocument()
{
    static const char* const document =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<root attr1=\"val1\">\n"
        "  <node1 a1=\"v1\" a2=\"v2\">Text in node1</node1>\n"
        "  <node2 b1=\"x1\">Text in node2</node2>\n"
        "  <node3 foo=\"bar\"/>\n"
        "  <node4/>\n"
        "</root>\n";

    tinyxml2::XMLDocument doc;
    CPPUNIT_ASSERT_EQUAL(tinyxml2::XML_SUCCESS, doc.Parse(document));

    tinyxml2::XMLElement* root = doc.RootElement();
    CPPUNIT_ASSERT(root != 0);
    CPPUNIT_ASSERT(root->Name() != 0);
    CPPUNIT_ASSERT(!root->NoChildren());
    CPPUNIT_ASSERT_STRINGS_EQUAL("root", root->Name());
    CPPUNIT_ASSERT(root->Attribute("attr1") != 0);
    CPPUNIT_ASSERT_STRINGS_EQUAL("val1", root->Attribute("attr1"));
    CPPUNIT_ASSERT(root->Attribute("nonexistent") == 0);

    tinyxml2::XMLElement* elem = root->FirstChildElement();
    CPPUNIT_ASSERT(elem != 0);
    CPPUNIT_ASSERT(elem->Name() != 0);
    CPPUNIT_ASSERT(!elem->NoChildren());
    CPPUNIT_ASSERT_STRINGS_EQUAL("node1", elem->Name());
    CPPUNIT_ASSERT(elem->Attribute("a1") != 0);
    CPPUNIT_ASSERT_STRINGS_EQUAL("v1", elem->Attribute("a1"));
    CPPUNIT_ASSERT(elem->Attribute("a2") != 0);
    CPPUNIT_ASSERT_STRINGS_EQUAL("v2", elem->Attribute("a2"));
    CPPUNIT_ASSERT(elem->GetText() != 0);
    CPPUNIT_ASSERT_STRINGS_EQUAL("Text in node1", elem->GetText());

    elem = elem->NextSiblingElement();
    CPPUNIT_ASSERT(elem != 0);
    CPPUNIT_ASSERT(elem->Name() != 0);
    CPPUNIT_ASSERT(!elem->NoChildren());
    CPPUNIT_ASSERT_STRINGS_EQUAL("node2", elem->Name());
    CPPUNIT_ASSERT(elem->Attribute("b1") != 0);
    CPPUNIT_ASSERT_STRINGS_EQUAL("x1", elem->Attribute("b1"));
    CPPUNIT_ASSERT(elem->GetText() != 0);
    CPPUNIT_ASSERT_STRINGS_EQUAL("Text in node2", elem->GetText());

    elem = elem->NextSiblingElement();
    CPPUNIT_ASSERT(elem != 0);
    CPPUNIT_ASSERT(elem->Name() != 0);
    CPPUNIT_ASSERT(elem->NoChildren());
    CPPUNIT_ASSERT_STRINGS_EQUAL("node3", elem->Name());
    CPPUNIT_ASSERT(elem->Attribute("foo") != 0);
    CPPUNIT_ASSERT_STRINGS_EQUAL("bar", elem->Attribute("foo"));
    CPPUNIT_ASSERT(elem->GetText() == 0);

    elem = elem->NextSiblingElement();
    CPPUNIT_ASSERT(elem != 0);
    CPPUNIT_ASSERT(elem->Name() != 0);
    CPPUNIT_ASSERT(elem->NoChildren());
    CPPUNIT_ASSERT_STRINGS_EQUAL("node4", elem->Name());
    CPPUNIT_ASSERT(elem->Attribute("foo") == 0);
    CPPUNIT_ASSERT(elem->GetText() == 0);

    elem = elem->NextSiblingElement();
    CPPUNIT_ASSERT(elem == 0);
}

namespace {
    class Visitor : public tinyxml2::XMLVisitor
    {
    private:
        ts::StringList _ref;
        ts::StringList::const_iterator _iter;

        void AssertNext(const char* name, const char* value)
        {
            CPPUNIT_ASSERT(_iter != _ref.end());
            CPPUNIT_ASSERT_STRINGS_EQUAL(*_iter++, name);
            CPPUNIT_ASSERT(_iter != _ref.end());
            CPPUNIT_ASSERT_STRINGS_EQUAL(*_iter++, value);
        }

    public:
        Visitor(const char* str, ...) :
            _ref(),
            _iter()
        {
            va_list ap;
            va_start(ap, str);
            while (str != 0) {
                _ref.push_back(str);
                str = va_arg(ap, const char*);
            }
            _iter = _ref.begin();
        }

        bool AtEnd()
        {
            return _iter == _ref.end();
        }

        virtual bool VisitEnter(const tinyxml2::XMLDocument& doc)
        {
            utest::Out() << "XMLTest::Visitor::VisitEnter (document)" << std::endl;
            AssertNext("EnterDocument", "");
            return true;
        }

        virtual bool VisitExit(const tinyxml2::XMLDocument& doc) 
        {
            utest::Out() << "XMLTest::Visitor::VisitExit (document)" << std::endl;
            AssertNext("ExitDocument", "");
            return true;
        }

        virtual bool VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* firstAttribute)
        {
            utest::Out() << "XMLTest::Visitor::VisitEnter (element) name='" << element.Name() << "'" << std::endl;
            AssertNext("EnterElement", element.Name());
            return true;
        }

        virtual bool VisitExit(const tinyxml2::XMLElement& element)
        {
            utest::Out() << "XMLTest::Visitor::VisitExit (element) name='" << element.Name() << "'" << std::endl;
            AssertNext("ExitElement", element.Name());
            return true;
        }

        virtual bool Visit(const tinyxml2::XMLDeclaration& declaration)
        {
            utest::Out() << "XMLTest::Visitor::Visit (declaration) value='" << declaration.Value() << "'" << std::endl;
            AssertNext("Declaration", declaration.Value());
            return true;
        }

        virtual bool Visit(const tinyxml2::XMLText& text)
        {
            utest::Out() << "XMLTest::Visitor::Visit (text) value='" << text.Value() << "'" << std::endl;
            AssertNext("Text", text.Value());
            return true;
        }

        virtual bool Visit(const tinyxml2::XMLComment& comment)
        {
            utest::Out() << "XMLTest::Visitor::Visit (comment) value='" << comment.Value() << "'" << std::endl;
            AssertNext("Comment", comment.Value());
            return true;
        }

        virtual bool Visit(const tinyxml2::XMLUnknown& unknown)
        {
            utest::Out() << "XMLTest::Visitor::Visit (unknown) value='" << unknown.Value() << "'" << std::endl;
            AssertNext("Unknown", unknown.Value());
            return true;
        }
    };
}

void XMLTest::testVisitor()
{
    static const char* const document =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<root attr1=\"val1\">\n"
        "  <node1 a1=\"v1\" a2=\"v2\">Text in node1</node1>\n"
        "  <node2 foo=\"bar\"/>\n"
        "  <node3/>\n"
        "</root>\n";

    Visitor visitor(
        "EnterDocument", "",
        "Declaration", "xml version=\"1.0\" encoding=\"UTF-8\"",
        "EnterElement", "root",
        "EnterElement", "node1",
        "Text", "Text in node1",
        "ExitElement", "node1",
        "EnterElement", "node2",
        "ExitElement", "node2",
        "EnterElement", "node3",
        "ExitElement", "node3",
        "ExitElement", "root",
        "ExitDocument", "",
        TS_NULL
    );

    tinyxml2::XMLDocument doc;
    CPPUNIT_ASSERT_EQUAL(tinyxml2::XML_SUCCESS, doc.Parse(document));

    CPPUNIT_ASSERT(doc.Accept(&visitor));
    CPPUNIT_ASSERT(visitor.AtEnd());
}
