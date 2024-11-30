//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::TextTable
//
//----------------------------------------------------------------------------

#include "tsTextTable.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TextTableTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Table);

private:
    static std::string result(std::ostringstream& out);
};

TSUNIT_REGISTER(TextTableTest);


//----------------------------------------------------------------------------
// Cleanup end of lines.
//----------------------------------------------------------------------------

// Cleanup end of lines.
std::string TextTableTest::result(std::ostringstream& out)
{
    std::string text(out.str());
    for (size_t start = 0; start < text.size(); ) {
        start = text.find('\r', start);
        if (start != std::string::npos) {
            text.erase(start, 1);
        }
    }
    out.str(std::string());
    return text;
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Table)
{
    std::ostringstream out;
    ts::TextTable tt;

    enum Id {A, B, C, D};
    tt.addColumn(A, u"Header A", ts::TextTable::Align::RIGHT);
    tt.addColumn(B, u"Header B", ts::TextTable::Align::LEFT);
    tt.addColumn(C, u"Header C", ts::TextTable::Align::RIGHT);
    tt.addColumn(D, u"Header D", ts::TextTable::Align::LEFT);

    // First line: implicitly created.
    tt.setCell(B, u"Foo");
    tt.setCell(A, u"Bar");

    // Second line: explicit.
    tt.newLine();
    tt.setCell(D, u"Wxcvbn");
    tt.setCell(B, u"Qsdfghjklm");
    tt.setCell(A, u"Azerty");

    // Change current line.
    tt.setCurrentLine(6);
    tt.setCell(C, u"");
    tt.setCell(D, u"aqwzsx");
    tt.newLine();
    tt.setCell(C, u"");

    // Random lines and cells.
    tt.setCell(4, B, u"12345");
    tt.setCell(5, C, u"");
    tt.setCell(4, D, u"789");

    tt.output(out, ts::TextTable::Headers::UNDERLINED);

    TSUNIT_EQUAL("Header A Header B   Header C Header D\n"
                 "-------- ---------- -------- --------\n"
                 "     Bar Foo\n"
                 "  Azerty Qsdfghjklm          Wxcvbn\n"
                 "\n"
                 "\n"
                 "         12345               789\n"
                 "\n"
                 "                             aqwzsx\n"
                 "\n",
                 result(out));

    tt.output(out, ts::TextTable::Headers::TEXT);

    TSUNIT_EQUAL("Header A Header B   Header C Header D\n"
                 "     Bar Foo\n"
                 "  Azerty Qsdfghjklm          Wxcvbn\n"
                 "\n"
                 "\n"
                 "         12345               789\n"
                 "\n"
                 "                             aqwzsx\n"
                 "\n",
                 result(out));

    tt.output(out, ts::TextTable::Headers::NONE);

    TSUNIT_EQUAL("   Bar Foo\n"
                 "Azerty Qsdfghjklm  Wxcvbn\n"
                 "\n"
                 "\n"
                 "       12345       789\n"
                 "\n"
                 "                   aqwzsx\n"
                 "\n",
                 result(out));

    tt.output(out, ts::TextTable::Headers::TEXT, false, u">>", u" | ");

    TSUNIT_EQUAL(">>Header A | Header B   | Header C | Header D\n"
                 ">>     Bar | Foo        |          |\n"
                 ">>  Azerty | Qsdfghjklm |          | Wxcvbn\n"
                 ">>         |            |          |\n"
                 ">>         |            |          |\n"
                 ">>         | 12345      |          | 789\n"
                 ">>         |            |          |\n"
                 ">>         |            |          | aqwzsx\n"
                 ">>         |            |          |\n",
                 result(out));

    tt.output(out, ts::TextTable::Headers::TEXT, true, u">>", u" | ");

    TSUNIT_EQUAL(">>Header A | Header B   | Header D\n"
                 ">>     Bar | Foo        |\n"
                 ">>  Azerty | Qsdfghjklm | Wxcvbn\n"
                 ">>         | 12345      | 789\n"
                 ">>         |            | aqwzsx\n",
                 result(out));
}
