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
//  CppUnit test suite for the String class.
//
//----------------------------------------------------------------------------

#include "tsUString.h"
#include "tsHexa.h"
#include "tsDecimal.h"
#include "tsFormat.h"
#include "tsToInteger.h"
#include "tsByteBlock.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;

//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class UStringTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testIsSpace();
    void testUTF();
    void testTrim();
    void testLetterCase();
    void testAccent();
    void testHTML();
    void testRemove();
    void testSubstitute();
    void testSplit();
    void testJoin();
    void testBreakLines();
    void testRemovePrefix();
    void testRemoveSuffix();
    void testStart();
    void testEnd();
    void testJustifyLeft();
    void testJustifyRight();
    void testJustifyCentered();
    void testJustify();
    void testYesNo();
    void testTrueFalse();
    void testOnOff();
    void testSimilarStrings();

    CPPUNIT_TEST_SUITE(UStringTest);
    CPPUNIT_TEST(testIsSpace);
    CPPUNIT_TEST(testUTF);
    CPPUNIT_TEST(testTrim);
    CPPUNIT_TEST(testLetterCase);
    CPPUNIT_TEST(testAccent);
    CPPUNIT_TEST(testHTML);
    CPPUNIT_TEST(testRemove);
    CPPUNIT_TEST(testSubstitute);
    CPPUNIT_TEST(testSplit);
    CPPUNIT_TEST(testJoin);
    CPPUNIT_TEST(testBreakLines);
    CPPUNIT_TEST(testRemovePrefix);
    CPPUNIT_TEST(testRemoveSuffix);
    CPPUNIT_TEST(testStart);
    CPPUNIT_TEST(testEnd);
    CPPUNIT_TEST(testJustifyLeft);
    CPPUNIT_TEST(testJustifyRight);
    CPPUNIT_TEST(testJustifyCentered);
    CPPUNIT_TEST(testJustify);
    CPPUNIT_TEST(testYesNo);
    CPPUNIT_TEST(testTrueFalse);
    CPPUNIT_TEST(testOnOff);
    CPPUNIT_TEST(testSimilarStrings);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(UStringTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void UStringTest::setUp()
{
}

// Test suite cleanup method.
void UStringTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void UStringTest::testIsSpace()
{
    CPPUNIT_ASSERT(ts::IsSpace(ts::SPACE));
    CPPUNIT_ASSERT(ts::IsSpace(ts::LINE_FEED));
    CPPUNIT_ASSERT(ts::IsSpace(ts::CARRIAGE_RETURN));
    CPPUNIT_ASSERT(ts::IsSpace(ts::HORIZONTAL_TABULATION));
    CPPUNIT_ASSERT(ts::IsSpace(ts::VERTICAL_TABULATION));
    CPPUNIT_ASSERT(ts::IsSpace(ts::FORM_FEED));
    CPPUNIT_ASSERT(!ts::IsSpace(ts::LATIN_CAPITAL_LETTER_A));
    CPPUNIT_ASSERT(!ts::IsSpace(ts::COLON));
    CPPUNIT_ASSERT(!ts::IsSpace(ts::CHAR_NULL));
}

void UStringTest::testUTF()
{
    // Reference UTF-8 text.
    // Was entered manually in Notepad++ and separately saved in UTF-8 and UTF-16.
    static const uint8_t utf8_bytes[] = {
        0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x66, 0x6b, 0x6c,
        0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7a, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x46,
        0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
        0x57, 0x58, 0x59, 0x5a, 0x0a, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
        0x37, 0x38, 0x39, 0x2f, 0x5c, 0x2d, 0x5f, 0x3d, 0x2b, 0x28, 0x29, 0x5b,
        0x5d, 0x7b, 0x7d, 0x7e, 0x26, 0xc2, 0xb2, 0xc2, 0xb0, 0x27, 0x22, 0x23,
        0xc3, 0xa9, 0xc3, 0xa8, 0xc3, 0xaa, 0xc3, 0xa0, 0xc3, 0xa2, 0xc3, 0xb9,
        0xc3, 0xbb, 0xc3, 0xa7, 0xe2, 0x82, 0xac, 0xc3, 0xa4, 0x5e, 0xc2, 0xa8,
        0xc2, 0xa3, 0xc2, 0xa4, 0xc3, 0x89, 0xc3, 0x88, 0xc3, 0x8a, 0xc3, 0x80,
        0xc3, 0x82, 0xc3, 0x99, 0xc3, 0x9b, 0xc3, 0x87, 0xe2, 0x82, 0xac, 0xc3,
        0x84, 0x5e, 0xc2, 0xa8, 0xc2, 0xa3, 0xc2, 0xa4, 0x0a, 0xc3, 0x80, 0xc3,
        0x81, 0xc3, 0x82, 0xc3, 0x83, 0xc3, 0x84, 0xc3, 0x85, 0xc3, 0x86, 0xc3,
        0x87, 0xc3, 0x88, 0xc3, 0x89, 0xc3, 0x8a, 0xc3, 0x8b, 0xc3, 0x8c, 0xc3,
        0x8d, 0xc3, 0x8e, 0xc3, 0x8f, 0xc3, 0x90, 0xc3, 0x93, 0xc3, 0x94, 0xc3,
        0x96, 0xc3, 0x97, 0xc3, 0x98, 0xc3, 0x9c, 0xc3, 0x9d, 0xc3, 0x9e, 0xc3,
        0x9f, 0xc3, 0xa0, 0xc3, 0xa1, 0xc3, 0xa2, 0xc3, 0xa3, 0xc3, 0xa5, 0xc3,
        0xa5, 0xc3, 0xa6, 0xc3, 0xa7, 0xc3, 0xa8, 0xc3, 0xa9, 0xc3, 0xaa, 0xc3,
        0xab, 0xc3, 0xb0, 0xc3, 0xb1, 0xc3, 0xb5, 0x0a,
        // Null terminated for tests.
        0x00
    };

    // Corresponding UTF-16 values.
    static const uint16_t utf16_values[] = {
        0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069,
        0x0066, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072,
        0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x0041,
        0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x0046,
        0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0x0053,
        0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x000a, 0x0030,
        0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039,
        0x002f, 0x005c, 0x002d, 0x005f, 0x003d, 0x002b, 0x0028, 0x0029, 0x005b,
        0x005d, 0x007b, 0x007d, 0x007e, 0x0026, 0x00b2, 0x00b0, 0x0027, 0x0022,
        0x0023, 0x00e9, 0x00e8, 0x00ea, 0x00e0, 0x00e2, 0x00f9, 0x00fb, 0x00e7,
        0x20ac, 0x00e4, 0x005e, 0x00a8, 0x00a3, 0x00a4, 0x00c9, 0x00c8, 0x00ca,
        0x00c0, 0x00c2, 0x00d9, 0x00db, 0x00c7, 0x20ac, 0x00c4, 0x005e, 0x00a8,
        0x00a3, 0x00a4, 0x000a, 0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5,
        0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce,
        0x00cf, 0x00d0, 0x00d3, 0x00d4, 0x00d6, 0x00d7, 0x00d8, 0x00dc, 0x00dd,
        0x00de, 0x00df, 0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e5, 0x00e5, 0x00e6,
        0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00f0, 0x00f1, 0x00f5, 0x000a,
        // Null terminated for tests.
        0x0000
    };

    const size_t utf16_count = sizeof(utf16_values) / sizeof(utf16_values[0]) - 1;
    const size_t utf8_count = sizeof(utf8_bytes) / sizeof(utf8_bytes[0]) - 1;

    ts::UString s1(reinterpret_cast<const ts::UChar*>(utf16_values));
    ts::UString s2(reinterpret_cast<const ts::UChar*>(utf16_values), utf16_count);
    ts::UString s3(reinterpret_cast<const char*>(utf8_bytes));
    ts::UString s4(reinterpret_cast<const char*>(utf8_bytes), utf8_count);

    utest::Out() << "UStringTest::testUTF: utf16_count = " << utf16_count << ", s1.length() = " << s1.length() << std::endl;

    CPPUNIT_ASSERT_EQUAL(s1.length(), s1.size());
    CPPUNIT_ASSERT_EQUAL(s2.length(), s2.size());
    CPPUNIT_ASSERT_EQUAL(s3.length(), s3.size());
    CPPUNIT_ASSERT_EQUAL(s4.length(), s4.size());

    CPPUNIT_ASSERT_EQUAL(s1.length(), utf16_count);
    CPPUNIT_ASSERT_EQUAL(s2.length(), utf16_count);
    CPPUNIT_ASSERT_EQUAL(s3.length(), utf16_count);
    CPPUNIT_ASSERT_EQUAL(s4.length(), utf16_count);

    CPPUNIT_ASSERT(s1 == s2);
    CPPUNIT_ASSERT(s1 == s3);
    CPPUNIT_ASSERT(s1 == s4);
}

void UStringTest::testTrim()
{
    ts::UString s;

    s = "  abc  ";
    s.trim();
    CPPUNIT_ASSERT(s == "abc");

    s = "  abc  ";
    s.trim(true, false);
    CPPUNIT_ASSERT(s == "abc  ");

    s = "  abc  ";
    s.trim(false, true);
    CPPUNIT_ASSERT(s == "  abc");

    s = "  abc  ";
    s.trim(false, false);
    CPPUNIT_ASSERT(s == "  abc  ");

    s = "abc";
    s.trim();
    CPPUNIT_ASSERT(s == "abc");

    s = "abc";
    s.trim(true, false);
    CPPUNIT_ASSERT(s == "abc");

    s = "abc";
    s.trim(false, true);
    CPPUNIT_ASSERT(s == "abc");

    s = "abc";
    s.trim(false, false);
    CPPUNIT_ASSERT(s == "abc");

    s = "  abc  ";
    CPPUNIT_ASSERT(s.toTrimmed() == "abc");
    CPPUNIT_ASSERT(s.toTrimmed(true, false) == "abc  ");
    CPPUNIT_ASSERT(s.toTrimmed(false, true) == "  abc");
    CPPUNIT_ASSERT(s.toTrimmed(false, false) == "  abc  ");

    s = "abc";
    CPPUNIT_ASSERT(s.toTrimmed() == "abc");
    CPPUNIT_ASSERT(s.toTrimmed(true, false) == "abc");
    CPPUNIT_ASSERT(s.toTrimmed(false, true) == "abc");
    CPPUNIT_ASSERT(s.toTrimmed(false, false) == "abc");
}

void UStringTest::testLetterCase()
{
    CPPUNIT_ASSERT(!ts::IsLower(ts::COMMA));
    CPPUNIT_ASSERT(!ts::IsUpper(ts::COMMA));

    CPPUNIT_ASSERT_EQUAL(ts::COMMA, ts::ToLower(ts::COMMA));
    CPPUNIT_ASSERT_EQUAL(ts::COMMA, ts::ToUpper(ts::COMMA));

    struct UpperLower {
        ts::UChar upper;
        ts::UChar lower;
    };
    static const UpperLower tab[] = {
        {ts::LATIN_CAPITAL_LETTER_A, ts::LATIN_SMALL_LETTER_A},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_GRAVE, ts::LATIN_SMALL_LETTER_A_WITH_GRAVE},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_ACUTE, ts::LATIN_SMALL_LETTER_A_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_W_WITH_DIAERESIS, ts::LATIN_SMALL_LETTER_W_WITH_DIAERESIS},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_GRAVE, ts::LATIN_SMALL_LETTER_Y_WITH_GRAVE},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS, ts::LATIN_SMALL_LETTER_Y_WITH_DIAERESIS},
        {ts::GREEK_CAPITAL_LETTER_IOTA_WITH_DIALYTIKA, ts::GREEK_SMALL_LETTER_IOTA_WITH_DIALYTIKA},
        {ts::GREEK_CAPITAL_LETTER_UPSILON_WITH_DIALYTIKA, ts::GREEK_SMALL_LETTER_UPSILON_WITH_DIALYTIKA},
        {ts::GREEK_CAPITAL_LETTER_EPSILON, ts::GREEK_SMALL_LETTER_EPSILON},
        {ts::GREEK_CAPITAL_LETTER_ALPHA, ts::GREEK_SMALL_LETTER_ALPHA},
        {ts::GREEK_CAPITAL_LETTER_OMICRON_WITH_TONOS, ts::GREEK_SMALL_LETTER_OMICRON_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_UPSILON_WITH_TONOS, ts::GREEK_SMALL_LETTER_UPSILON_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_OMEGA_WITH_TONOS, ts::GREEK_SMALL_LETTER_OMEGA_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_EPSILON_WITH_TONOS, ts::GREEK_SMALL_LETTER_EPSILON_WITH_TONOS},
        {ts::CYRILLIC_CAPITAL_LETTER_BE, ts::CYRILLIC_SMALL_LETTER_BE},
        {ts::CYRILLIC_CAPITAL_LETTER_HARD_SIGN, ts::CYRILLIC_SMALL_LETTER_HARD_SIGN},
        {ts::CYRILLIC_CAPITAL_LETTER_SHORT_U, ts::CYRILLIC_SMALL_LETTER_SHORT_U},
        {ts::CYRILLIC_CAPITAL_LETTER_DZHE, ts::CYRILLIC_SMALL_LETTER_DZHE},
    };
    static const size_t tabSize = sizeof(tab) / sizeof(tab[0]);

    for (size_t i = 0; i < tabSize; ++i) {
        CPPUNIT_ASSERT(ts::IsUpper(tab[i].upper));
        CPPUNIT_ASSERT(!ts::IsLower(tab[i].upper));
        CPPUNIT_ASSERT(ts::IsLower(tab[i].lower));
        CPPUNIT_ASSERT(!ts::IsUpper(tab[i].lower));
        CPPUNIT_ASSERT_EQUAL(tab[i].lower, ts::ToLower(tab[i].lower));
        CPPUNIT_ASSERT_EQUAL(tab[i].lower, ts::ToLower(tab[i].upper));
        CPPUNIT_ASSERT_EQUAL(tab[i].upper, ts::ToUpper(tab[i].lower));
        CPPUNIT_ASSERT_EQUAL(tab[i].upper, ts::ToUpper(tab[i].upper));
    }

    ts::UString s1("AbCdEf,%*=UiT");
    CPPUNIT_ASSERT(s1.toLower() == "abcdef,%*=uit");
    CPPUNIT_ASSERT(s1.toUpper() == "ABCDEF,%*=UIT");

    s1 = "AbCdEf,%*=UiT";
    CPPUNIT_ASSERT(s1 == "AbCdEf,%*=UiT");
    s1.convertToLower();
    CPPUNIT_ASSERT(s1 == "abcdef,%*=uit");

    s1 = "AbCdEf,%*=UiT";
    CPPUNIT_ASSERT(s1 == "AbCdEf,%*=UiT");
    s1.convertToUpper();
    CPPUNIT_ASSERT(s1 == "ABCDEF,%*=UIT");
}

void UStringTest::testAccent()
{
    CPPUNIT_ASSERT(!ts::IsAccented('A'));
    CPPUNIT_ASSERT(!ts::IsAccented(':'));
    CPPUNIT_ASSERT(ts::IsAccented(ts::LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS));
    CPPUNIT_ASSERT(ts::IsAccented(ts::LATIN_CAPITAL_LETTER_C_WITH_CIRCUMFLEX));
    CPPUNIT_ASSERT(ts::IsAccented(ts::BLACKLETTER_CAPITAL_I));
    CPPUNIT_ASSERT(ts::IsAccented(ts::SCRIPT_CAPITAL_P));
    CPPUNIT_ASSERT(ts::IsAccented(ts::BLACKLETTER_CAPITAL_R));
    CPPUNIT_ASSERT(ts::IsAccented(ts::LATIN_CAPITAL_LIGATURE_OE));

    CPPUNIT_ASSERT("X" == ts::RemoveAccent('X'));
    CPPUNIT_ASSERT("," == ts::RemoveAccent(','));
    CPPUNIT_ASSERT("E" == ts::RemoveAccent(ts::LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS));
    CPPUNIT_ASSERT("c" == ts::RemoveAccent(ts::LATIN_SMALL_LETTER_C_WITH_ACUTE));
    CPPUNIT_ASSERT("C" == ts::RemoveAccent(ts::LATIN_CAPITAL_LETTER_C_WITH_CIRCUMFLEX));
    CPPUNIT_ASSERT("f" == ts::RemoveAccent(ts::LATIN_SMALL_F_WITH_HOOK));
    CPPUNIT_ASSERT("I" == ts::RemoveAccent(ts::BLACKLETTER_CAPITAL_I));
    CPPUNIT_ASSERT("P" == ts::RemoveAccent(ts::SCRIPT_CAPITAL_P));
    CPPUNIT_ASSERT("R" == ts::RemoveAccent(ts::BLACKLETTER_CAPITAL_R));
    CPPUNIT_ASSERT("OE" == ts::RemoveAccent(ts::LATIN_CAPITAL_LIGATURE_OE));
    CPPUNIT_ASSERT("oe" == ts::RemoveAccent(ts::LATIN_SMALL_LIGATURE_OE));
}

void UStringTest::testHTML()
{
    CPPUNIT_ASSERT(ts::ToHTML('A') == "A");
    CPPUNIT_ASSERT(ts::ToHTML(':') == ":");
    CPPUNIT_ASSERT(ts::ToHTML(ts::QUOTATION_MARK) == "&quot;");
    CPPUNIT_ASSERT(ts::ToHTML(ts::AMPERSAND) == "&amp;");
    CPPUNIT_ASSERT(ts::ToHTML(ts::LESS_THAN_SIGN) == "&lt;");
    CPPUNIT_ASSERT(ts::ToHTML(ts::GREATER_THAN_SIGN) == "&gt;");
    CPPUNIT_ASSERT(ts::ToHTML(ts::NO_BREAK_SPACE) == "&nbsp;");
    CPPUNIT_ASSERT(ts::ToHTML(ts::LEFT_DOUBLE_QUOTATION_MARK) == "&ldquo;");
    CPPUNIT_ASSERT(ts::ToHTML(ts::BLACK_DIAMOND_SUIT) == "&diams;");

    CPPUNIT_ASSERT(ts::UString("").toHTML() == "");
    CPPUNIT_ASSERT(ts::UString("abcdefgh = xyz:").toHTML() == "abcdefgh = xyz:");
    CPPUNIT_ASSERT(ts::UString("<abcd> = \"&").toHTML() == "&lt;abcd&gt; = &quot;&amp;");
}

void UStringTest::testRemove()
{
    ts::UString s;

    s = "az zef cer ";
    s.remove(" ");
    CPPUNIT_ASSERT(s == "azzefcer");

    s = "fooAZfoo==fooBARfoo";
    s.remove("foo");
    CPPUNIT_ASSERT(s == "AZ==BAR");

    s = "fooAZfoo==fooBARfoo";
    const ts::UString foo1("foo");
    s.remove(foo1);
    CPPUNIT_ASSERT(s == "AZ==BAR");

    s = "fooAZfoo==fooBARfoo";
    s.remove("NOTTHERE");
    CPPUNIT_ASSERT(s == "fooAZfoo==fooBARfoo");

    s = "";
    s.remove("foo");
    CPPUNIT_ASSERT(s == "");

    s = "fooAZfoo==fooBARfoo";
    s.remove("");
    CPPUNIT_ASSERT(s == "fooAZfoo==fooBARfoo");

    s = "fooAZfoo==fooBARfoo";
    s.remove("o");
    CPPUNIT_ASSERT(s == "fAZf==fBARf");

    s = "fooAZfoo==fooBARfoo";
    s.remove("z");
    CPPUNIT_ASSERT(s == "fooAZfoo==fooBARfoo");

    s = "az zef cer ";
    CPPUNIT_ASSERT(s.toRemoved(" ") == "azzefcer");

    CPPUNIT_ASSERT(ts::UString("fooAZfoo==fooBARfoo").toRemoved("foo") == "AZ==BAR");

    s = "fooAZfoo==fooBARfoo";
    const ts::UString foo2("foo");
    CPPUNIT_ASSERT(s.toRemoved(foo2) == "AZ==BAR");
    CPPUNIT_ASSERT(s.toRemoved("NOTTHERE") == "fooAZfoo==fooBARfoo");

    s = "";
    CPPUNIT_ASSERT(s.toRemoved("foo") == "");

    s = "fooAZfoo==fooBARfoo";
    CPPUNIT_ASSERT(s.toRemoved("")  == "fooAZfoo==fooBARfoo");
    CPPUNIT_ASSERT(s.toRemoved("o") == "fAZf==fBARf");
    CPPUNIT_ASSERT(s.toRemoved("z") == "fooAZfoo==fooBARfoo");
    CPPUNIT_ASSERT(s.toRemoved('z') == "fooAZfoo==fooBARfoo");
    CPPUNIT_ASSERT(s.toRemoved(ts::UChar('z')) == "fooAZfoo==fooBARfoo");
    CPPUNIT_ASSERT(s.toRemoved('o') == "fAZf==fBARf");
    CPPUNIT_ASSERT(s.toRemoved(ts::UChar('o')) == "fAZf==fBARf");
}

void UStringTest::testSubstitute()
{
    CPPUNIT_ASSERT(ts::UString("").toSubstituted("", "") == "");
    CPPUNIT_ASSERT(ts::UString("abcdefabcdef").toSubstituted("ab", "xyz") == "xyzcdefxyzcdef");
    CPPUNIT_ASSERT(ts::UString("abcdefabcdef").toSubstituted("ef", "xyz") == "abcdxyzabcdxyz");
    CPPUNIT_ASSERT(ts::UString("abcdba").toSubstituted("b", "bb") == "abbcdbba");
    CPPUNIT_ASSERT(ts::UString("abcdefabcdef").toSubstituted("ef", "") == "abcdabcd");
}

void UStringTest::testSplit()
{
    std::vector<ts::UString> v1;
    ts::UString("az, ,  fr,  ze ,t").split(v1);
    CPPUNIT_ASSERT(v1.size() == 5);
    CPPUNIT_ASSERT(v1[0] == "az");
    CPPUNIT_ASSERT(v1[1] == "");
    CPPUNIT_ASSERT(v1[2] == "fr");
    CPPUNIT_ASSERT(v1[3] == "ze");
    CPPUNIT_ASSERT(v1[4] == "t");

    std::vector<ts::UString> v2;
    const ts::UString s2("az, ,  fr,  ze ,t");
    s2.split(v2);
    CPPUNIT_ASSERT(v2.size() == 5);
    CPPUNIT_ASSERT(v2[0] == "az");
    CPPUNIT_ASSERT(v2[1] == "");
    CPPUNIT_ASSERT(v2[2] == "fr");
    CPPUNIT_ASSERT(v2[3] == "ze");
    CPPUNIT_ASSERT(v2[4] == "t");

    std::vector<ts::UString> v3;
    ts::UString("az, ,  fr,  ze ,t").split(v3, ts::COMMA, false);
    CPPUNIT_ASSERT(v3.size() == 5);
    CPPUNIT_ASSERT(v3[0] == "az");
    CPPUNIT_ASSERT(v3[1] == " ");
    CPPUNIT_ASSERT(v3[2] == "  fr");
    CPPUNIT_ASSERT(v3[3] == "  ze ");
    CPPUNIT_ASSERT(v3[4] == "t");

    std::vector<ts::UString> v4;
    ts::UString("az, ,  fr,  ze ,t").split(v4, ts::UChar('z'), false);
    CPPUNIT_ASSERT(v4.size() == 3);
    CPPUNIT_ASSERT(v4[0] == "a");
    CPPUNIT_ASSERT(v4[1] == ", ,  fr,  ");
    CPPUNIT_ASSERT(v4[2] == "e ,t");
}

void UStringTest::testJoin()
{
    std::vector<ts::UString> v;
    v.push_back("az");
    v.push_back("sd");
    v.push_back("tg");
    CPPUNIT_ASSERT(ts::UString::Join(v) == "az, sd, tg");
    CPPUNIT_ASSERT(ts::UString::Join(++v.begin(), v.end()) == "sd, tg");
}

void UStringTest::testBreakLines()
{
    std::vector<ts::UString> v1;
    ts::UString("aze arf erf r+oih zf").splitLines(v1, 8);
    CPPUNIT_ASSERT(v1.size() == 3);
    CPPUNIT_ASSERT(v1[0] == "aze arf");
    CPPUNIT_ASSERT(v1[1] == "erf");
    CPPUNIT_ASSERT(v1[2] == "r+oih zf");

    std::vector<ts::UString> v2;
    ts::UString("aze arf erf r+oih zf").splitLines(v2, 8, "+");
    CPPUNIT_ASSERT(v2.size() == 3);
    CPPUNIT_ASSERT(v2[0] == "aze arf");
    CPPUNIT_ASSERT(v2[1] == "erf r+");
    CPPUNIT_ASSERT(v2[2] == "oih zf");

    std::vector<ts::UString> v3;
    ts::UString("aze arf erf r+oih zf").splitLines(v3, 8, "", "==");
    CPPUNIT_ASSERT(v3.size() == 4);
    CPPUNIT_ASSERT(v3[0] == "aze arf");
    CPPUNIT_ASSERT(v3[1] == "==erf");
    CPPUNIT_ASSERT(v3[2] == "==r+oih");
    CPPUNIT_ASSERT(v3[3] == "==zf");

    std::vector<ts::UString> v4;
    ts::UString("aze arf dkvyfngofnb ff").splitLines(v4, 8);
    CPPUNIT_ASSERT(v4.size() == 3);
    CPPUNIT_ASSERT(v4[0] == "aze arf");
    CPPUNIT_ASSERT(v4[1] == "dkvyfngofnb");
    CPPUNIT_ASSERT(v4[2] == "ff");

    std::vector<ts::UString> v5;
    ts::UString("aze arf dkvyfngofnb ff").splitLines(v5, 8, "", "", true);
    CPPUNIT_ASSERT(v5.size() == 3);
    CPPUNIT_ASSERT(v5[0] == "aze arf");
    CPPUNIT_ASSERT(v5[1] == "dkvyfngo");
    CPPUNIT_ASSERT(v5[2] == "fnb ff");
}

void UStringTest::testRemovePrefix()
{
    ts::UString s;

    s = "abcdef";
    s.removePrefix("ab");
    CPPUNIT_ASSERT(s == "cdef");

    s = "abcdef";
    s.removePrefix("xy");
    CPPUNIT_ASSERT(s == "abcdef");

    s = "abcdef";
    s.removePrefix("");
    CPPUNIT_ASSERT(s == "abcdef");

    s = "";
    s.removePrefix("ab");
    CPPUNIT_ASSERT(s == "");

    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedPrefix("ab") == "cdef");
    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedPrefix("xy") == "abcdef");
    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedPrefix("") == "abcdef");
    CPPUNIT_ASSERT(ts::UString("").toRemovedPrefix("ab") == "");

    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedPrefix("AB") == "abcdef");
    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedPrefix("AB", ts::CASE_SENSITIVE) == "abcdef");
    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedPrefix("AB", ts::CASE_INSENSITIVE) == "cdef");
}

void UStringTest::testRemoveSuffix()
{
    ts::UString s;

    s = "abcdef";
    s.removeSuffix("ef");
    CPPUNIT_ASSERT(s == "abcd");

    s = "abcdef";
    s.removeSuffix("xy");
    CPPUNIT_ASSERT(s == "abcdef");

    s = "abcdef";
    s.removeSuffix("");
    CPPUNIT_ASSERT(s == "abcdef");

    s = "";
    s.removeSuffix("ef");
    CPPUNIT_ASSERT(s == "");

    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedSuffix("ef") == "abcd");
    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedSuffix("xy") == "abcdef");
    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedSuffix("") == "abcdef");
    CPPUNIT_ASSERT(ts::UString("").toRemovedSuffix("ef") == "");

    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedSuffix("EF") == "abcdef");
    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedSuffix("EF", ts::CASE_SENSITIVE) == "abcdef");
    CPPUNIT_ASSERT(ts::UString("abcdef").toRemovedSuffix("EF", ts::CASE_INSENSITIVE) == "abcd");
}

void UStringTest::testStart()
{
    CPPUNIT_ASSERT(ts::UString("azertyuiop").startWith("azer"));
    CPPUNIT_ASSERT(!ts::UString("azertyuiop").startWith("aZer"));
    CPPUNIT_ASSERT(!ts::UString("azertyuiop").startWith("azeR"));

    CPPUNIT_ASSERT(ts::UString("azertyuiop").startWith("azer", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(ts::UString("azertyuiop").startWith("aZer", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(ts::UString("azertyuiop").startWith("azeR", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString("azertyuiop").startWith("azerq", ts::CASE_INSENSITIVE));

    CPPUNIT_ASSERT(ts::UString("azertyuiop").startWith(""));
    CPPUNIT_ASSERT(!ts::UString("azertyuiop").startWith("azertyuiopqsdf"));

    CPPUNIT_ASSERT(ts::UString("azertyuiop").startWith("", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString("azertyuiop").startWith("azertyuiopqsdf", ts::CASE_INSENSITIVE));

    CPPUNIT_ASSERT(ts::UString("").startWith(""));
    CPPUNIT_ASSERT(!ts::UString("").startWith("abcd"));

    CPPUNIT_ASSERT(ts::UString("").startWith("", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString("").startWith("abcd", ts::CASE_INSENSITIVE));
}

void UStringTest::testEnd()
{
    CPPUNIT_ASSERT(ts::UString("azertyuiop").endWith("uiop"));
    CPPUNIT_ASSERT(!ts::UString("azertyuiop").endWith("uiOp"));
    CPPUNIT_ASSERT(!ts::UString("azertyuiop").endWith("Uiop"));

    CPPUNIT_ASSERT(ts::UString("azertyuiop").endWith("uiop", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(ts::UString("azertyuiop").endWith("uiOp", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(ts::UString("azertyuiop").endWith("Uiop", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString("azertyuiop").endWith("wuiop", ts::CASE_INSENSITIVE));

    CPPUNIT_ASSERT(ts::UString("azertyuiop").endWith(""));
    CPPUNIT_ASSERT(!ts::UString("azertyuiop").endWith("qsazertyuiop"));

    CPPUNIT_ASSERT(ts::UString("azertyuiop").endWith("", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString("azertyuiop").endWith("qsazertyuiop", ts::CASE_INSENSITIVE));

    CPPUNIT_ASSERT(ts::UString("").endWith(""));
    CPPUNIT_ASSERT(!ts::UString("").endWith("abcd"));

    CPPUNIT_ASSERT(ts::UString("").endWith("", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString("").endWith("abcd", ts::CASE_INSENSITIVE));
}

void UStringTest::testJustifyLeft()
{
    CPPUNIT_ASSERT(ts::UString("abc").toJustifiedLeft(8) == "abc     ");
    CPPUNIT_ASSERT(ts::UString("abc").toJustifiedLeft(8, '.') == "abc.....");
    CPPUNIT_ASSERT(ts::UString("abcdefghij").toJustifiedLeft(8) == "abcdefghij");
    CPPUNIT_ASSERT(ts::UString("abcdefghij").toJustifiedLeft(8, ' ', true) == "abcdefgh");
}

void UStringTest::testJustifyRight()
{
    CPPUNIT_ASSERT(ts::UString("abc").toJustifiedRight(8) == "     abc");
    CPPUNIT_ASSERT(ts::UString("abc").toJustifiedRight(8, '.') == ".....abc");
    CPPUNIT_ASSERT(ts::UString("abcdefghij").toJustifiedRight(8) == "abcdefghij");
    CPPUNIT_ASSERT(ts::UString("abcdefghij").toJustifiedRight(8, ' ', true) == "cdefghij");
}

void UStringTest::testJustifyCentered()
{
    CPPUNIT_ASSERT(ts::UString("abc").toJustifiedCentered(8) == "  abc   ");
    CPPUNIT_ASSERT(ts::UString("abc").toJustifiedCentered(8, '.') == "..abc...");
    CPPUNIT_ASSERT(ts::UString("abcdefghij").toJustifiedCentered(8) == "abcdefghij");
    CPPUNIT_ASSERT(ts::UString("abcdefghij").toJustifiedCentered(8, ' ', true) == "abcdefgh");
}

void UStringTest::testJustify()
{
    CPPUNIT_ASSERT(ts::UString("abc").toJustified("def", 8) == "abc  def");
    CPPUNIT_ASSERT(ts::UString("abc").toJustified("def", 8, '.') == "abc..def");
    CPPUNIT_ASSERT(ts::UString("abcd").toJustified("efgh", 8) == "abcdefgh");
    CPPUNIT_ASSERT(ts::UString("abcde").toJustified("fghij", 8) == "abcdefghij");
}

void UStringTest::testYesNo()
{
    CPPUNIT_ASSERT(ts::UString::YesNo(true) == "yes");
    CPPUNIT_ASSERT(ts::UString::YesNo(false) == "no");
}

void UStringTest::testTrueFalse()
{
    CPPUNIT_ASSERT(ts::UString::TrueFalse(true) == "true");
    CPPUNIT_ASSERT(ts::UString::TrueFalse(false) == "false");
}

void UStringTest::testOnOff()
{
    CPPUNIT_ASSERT(ts::UString::OnOff(true) == "on");
    CPPUNIT_ASSERT(ts::UString::OnOff(false) == "off");
}

void UStringTest::testSimilarStrings()
{
    CPPUNIT_ASSERT(ts::UString("").similar(""));
    CPPUNIT_ASSERT(ts::UString("aZer tY").similar("  AZE R T Y    "));
    CPPUNIT_ASSERT(ts::UString("  AZE R T Y    ").similar("aZer tY"));
    CPPUNIT_ASSERT(!ts::UString("").similar("az"));
    CPPUNIT_ASSERT(!ts::UString("az").similar(""));
}
