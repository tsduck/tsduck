//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::Grid
//
//----------------------------------------------------------------------------

#include "tsGrid.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class GridTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Narrow);
    TSUNIT_DECLARE_TEST(Default);
    TSUNIT_DECLARE_TEST(Layout);

private:
    static void cleanupEndLines(std::string& text);
};

TSUNIT_REGISTER(GridTest);


//----------------------------------------------------------------------------
// Cleanup end of lines.
//----------------------------------------------------------------------------

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

TSUNIT_DEFINE_TEST(Narrow)
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
        "\n"
        "============\n"
        "| FOO      |\n"
        "|==========|\n"
        "| Bar      |\n"
        "|----------|\n"
        "| The end  |\n"
        "============\n"
        "\n"
        "\n"
        "============\n"
        "| Last sec |\n"
        "| Last     |\n"
        "| section  |\n"
        "| azertyui |\n"
        "| opqsdfgh |\n"
        "| jklm     |\n"
        "| line     |\n"
        "============\n"
        "\n";

    std::string buffer(out.str());
    cleanupEndLines(buffer);

    debug() << "GridTest::testNarrow: " << std::endl << buffer << std::endl;
    TSUNIT_EQUAL(reference, buffer);
}

TSUNIT_DEFINE_TEST(Default)
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
        "\n"
        "================================================================================\n"
        "|  FOO                                                                         |\n"
        "|==============================================================================|\n"
        "|  abcd                                                                   xyz  |\n"
        "|  abcdefghijklmnopqrstuvwxyzabcdefghij  QRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ  |\n"
        "|  abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz                        |\n"
        "|                        ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ  |\n"
        "================================================================================\n"
        "\n";

    std::string buffer(out.str());
    cleanupEndLines(buffer);

    debug() << "GridTest::testDefault: " << std::endl << buffer << std::endl;
    TSUNIT_EQUAL(reference, buffer);
}

TSUNIT_DEFINE_TEST(Layout)
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
        "\n"
        "==================================================\n"
        "|  ab ........ cd  ef *********  |  -------- ij  |\n"
        "|  ABCDEF  KLMNOP  abcdefghijkl  |   mnopqrstuv  |\n"
        "|  ab ...........  ef *********  |               |\n"
        "|  ab ...........  ef *********  |  -------- ij  |\n"
        "==================================================\n"
        "\n"
        "\n"
        "==============================\n"
        "|  ab  cd  ef ***  |  -- ij  |\n"
        "|  AB  OP  abcdef  |  rstuv  |\n"
        "|  ab ...  ef ***  |         |\n"
        "|  ab ...  ef ***  |  -- ij  |\n"
        "==============================\n"
        "\n"
        "\n"
        "====================\n"
        "|      ef   |  ij  |\n"
        "|      abc  |  uv  |\n"
        "|   .  ef   |      |\n"
        "|   .  ef   |  ij  |\n"
        "====================\n"
        "\n"
        "\n"
        "==========\n"
        "|    ef  |\n"
        "|    abc |\n"
        "|  . ef  |\n"
        "|  . ef  |\n"
        "==========\n"
        "\n";

    std::string buffer(out.str());
    cleanupEndLines(buffer);

    debug() << "GridTest::testLayout: " << std::endl << buffer << std::endl;
    TSUNIT_EQUAL(reference, buffer);
}
