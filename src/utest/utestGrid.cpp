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
//  CppUnit test suite for class ts::Grid
//
//----------------------------------------------------------------------------

#include "tsGrid.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class GridTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testNarrow();
    void testDefault();
    void testLayout();

    CPPUNIT_TEST_SUITE(GridTest);
    CPPUNIT_TEST(testNarrow);
    CPPUNIT_TEST(testDefault);
    CPPUNIT_TEST(testLayout);
    CPPUNIT_TEST_SUITE_END();

private:
    static void cleanupEndLines(std::string& text);
};

CPPUNIT_TEST_SUITE_REGISTRATION(GridTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void GridTest::setUp()
{
}

// Test suite cleanup method.
void GridTest::tearDown()
{
}

// Cleanup end of lines.
void GridTest::cleanupEndLines(std::string& text)
{
    for (size_t start = 0; start < text.size(); ) {
        start = text.find('\r', start);
        if (start != std::string::npos) {
            text.erase(start, 1);
        }
    }
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void GridTest::testNarrow()
{
    std::ostringstream out;
    ts::Grid gr(out);

    gr.setLineWidth(12);

    gr.openTable();
    gr.putLine(u"FOO");
    gr.section();
    gr.putLine(u"Bar");
    gr.subSection();
    gr.putLine(u"The end");
    gr.closeTable();

    gr.openTable();
    gr.putLine(u"Last section");
    gr.putMultiLine(u"Last section azertyuiopqsdfghjklm line");
    gr.closeTable();

    static const char reference[] =
        u"\n"
        u"============\n"
        u"| FOO      |\n"
        u"|==========|\n"
        u"| Bar      |\n"
        u"|----------|\n"
        u"| The end  |\n"
        u"============\n"
        u"\n"
        u"\n"
        u"============\n"
        u"| Last sec |\n"
        u"| Last     |\n"
        u"| section  |\n"
        u"| azertyui |\n"
        u"| opqsdfgh |\n"
        u"| jklm     |\n"
        u"| line     |\n"
        u"============\n"
        u"\n";

    std::string buffer(out.str());
    cleanupEndLines(buffer);

    utest::Out() << "GridTest::testNarrow: " << std::endl << buffer << std::endl;
    CPPUNIT_ASSERT_STRINGS_EQUAL(reference, buffer);
}

void GridTest::testDefault()
{
    std::ostringstream out;
    ts::Grid gr(out);

    gr.openTable();
    gr.putLine(u"FOO");
    gr.section();
    gr.putLine(u"abcd", u"xyz");
    gr.putLine(u"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz", u"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ");
    gr.putLine(u"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz", u"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ", false);
    gr.closeTable();

    static const char reference[] =
        u"\n"
        u"================================================================================\n"
        u"|  FOO                                                                         |\n"
        u"|==============================================================================|\n"
        u"|  abcd                                                                   xyz  |\n"
        u"|  abcdefghijklmnopqrstuvwxyzabcdefghij  QRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ  |\n"
        u"|  abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz                        |\n"
        u"|                        ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ  |\n"
        u"================================================================================\n"
        u"\n";

    std::string buffer(out.str());
    cleanupEndLines(buffer);

    utest::Out() << "GridTest::testDefault: " << std::endl << buffer << std::endl;
    CPPUNIT_ASSERT_STRINGS_EQUAL(reference, buffer);
}

void GridTest::testLayout()
{
    std::ostringstream out;
    ts::Grid gr(out);

    gr.setLayout({gr.both(10, u'.'), gr.left(8, u'*'), gr.border(), gr.right(6, u'-')});

    gr.setLineWidth(50);
    gr.openTable();
    gr.putLayout({{u"ab", u"cd"}, {u"ef"}, {u"ij"}});
    gr.putLayout({{u"ABCDEFGH", u"IJKLMNOP"}, {u"abcdefghijkl"}, {u"mnopqrstuv"}});
    gr.putLayout({{u"ab"}, {u"ef"}});
    gr.putLayout({{u"ab"}, {u"ef"}, {u"ij"}, {u"kl"}});
    gr.closeTable();

    gr.setLineWidth(30);
    gr.openTable();
    gr.putLayout({{u"ab", u"cd"}, {u"ef"}, {u"ij"}});
    gr.putLayout({{u"ABCDEFGH", u"IJKLMNOP"}, {u"abcdefghijkl"}, {u"mnopqrstuv"}});
    gr.putLayout({{u"ab"}, {u"ef"}});
    gr.putLayout({{u"ab"}, {u"ef"}, {u"ij"}, {u"kl"}});
    gr.closeTable();

    gr.setLineWidth(20);
    gr.openTable();
    gr.putLayout({{u"ab", u"cd"}, {u"ef"}, {u"ij"}});
    gr.putLayout({{u"ABCDEFGH", u"IJKLMNOP"}, {u"abcdefghijkl"}, {u"mnopqrstuv"}});
    gr.putLayout({{u"ab"}, {u"ef"}});
    gr.putLayout({{u"ab"}, {u"ef"}, {u"ij"}, {u"kl"}});
    gr.closeTable();

    gr.setLineWidth(10);
    gr.openTable();
    gr.putLayout({{u"ab", u"cd"}, {u"ef"}, {u"ij"}});
    gr.putLayout({{u"ABCDEFGH", u"IJKLMNOP"}, {u"abcdefghijkl"}, {u"mnopqrstuv"}});
    gr.putLayout({{u"ab"}, {u"ef"}});
    gr.putLayout({{u"ab"}, {u"ef"}, {u"ij"}, {u"kl"}});
    gr.closeTable();

    static const char reference[] =
        u"\n"
        u"==================================================\n"
        u"|  ab ........ cd  ef *********  |  -------- ij  |\n"
        u"|  ABCDEF  KLMNOP  abcdefghijkl  |   mnopqrstuv  |\n"
        u"|  ab ...........  ef *********  |               |\n"
        u"|  ab ...........  ef *********  |  -------- ij  |\n"
        u"==================================================\n"
        u"\n"
        u"\n"
        u"==============================\n"
        u"|  ab  cd  ef ***  |  -- ij  |\n"
        u"|  AB  OP  abcdef  |  rstuv  |\n"
        u"|  ab ...  ef ***  |         |\n"
        u"|  ab ...  ef ***  |  -- ij  |\n"
        u"==============================\n"
        u"\n"
        u"\n"
        u"====================\n"
        u"|      ef   |  ij  |\n"
        u"|      abc  |  uv  |\n"
        u"|   .  ef   |      |\n"
        u"|   .  ef   |  ij  |\n"
        u"====================\n"
        u"\n"
        u"\n"
        u"==========\n"
        u"|    ef  |\n"
        u"|    abc |\n"
        u"|  . ef  |\n"
        u"|  . ef  |\n"
        u"==========\n"
        u"\n";

    std::string buffer(out.str());
    cleanupEndLines(buffer);

    utest::Out() << "GridTest::testLayout: " << std::endl << buffer << std::endl;
    CPPUNIT_ASSERT_STRINGS_EQUAL(reference, buffer);
}
