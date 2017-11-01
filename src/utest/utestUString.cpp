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
#include "tsSysUtils.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;

//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class UStringTest: public CppUnit::TestFixture
{
public:
    UStringTest();
    void setUp();
    void tearDown();
    void testIsSpace();
    void testUTF();
    void testDiacritical();
    void testSurrogate();
    void testWidth();
    void testDisplayPosition();
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
    void testLoadSave();
    void testToDigit();
    void testToInteger();
    void testHexaDecode();
    void testAppendContainer();
    void testAssignContainer();
    void testDecimal();
    void testHexa();

    CPPUNIT_TEST_SUITE(UStringTest);
    CPPUNIT_TEST(testIsSpace);
    CPPUNIT_TEST(testUTF);
    CPPUNIT_TEST(testDiacritical);
    CPPUNIT_TEST(testSurrogate);
    CPPUNIT_TEST(testWidth);
    CPPUNIT_TEST(testDisplayPosition);
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
    CPPUNIT_TEST(testLoadSave);
    CPPUNIT_TEST(testToInteger);
    CPPUNIT_TEST(testHexaDecode);
    CPPUNIT_TEST(testAppendContainer);
    CPPUNIT_TEST(testAssignContainer);
    CPPUNIT_TEST(testDecimal);
    CPPUNIT_TEST(testHexa);
    CPPUNIT_TEST_SUITE_END();

private:
    std::string _tempFilePrefix;
    int _nextFileIndex;
    std::string temporaryFileName(int) const;
    std::string newTemporaryFileName();

    // Two sample Unicode characters from the supplementary planes:
    //   U+1D538: MATHEMATICAL DOUBLE-STRUCK CAPITAL A
    //   U+1D539: MATHEMATICAL DOUBLE-STRUCK CAPITAL B
    static const ts::UChar MATH_A1 = ts::UChar(0xD800 | (0x1D538 >> 10));
    static const ts::UChar MATH_A2 = ts::UChar(0xDC00 | (0x1D538 & 0x03FF));
    static const ts::UChar MATH_B1 = ts::UChar(0xD800 | (0x1D539 >> 10));
    static const ts::UChar MATH_B2 = ts::UChar(0xDC00 | (0x1D539 & 0x03FF));
};

CPPUNIT_TEST_SUITE_REGISTRATION(UStringTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
UStringTest::UStringTest() :
    _tempFilePrefix(),
    _nextFileIndex(0)
{
}

// Test suite initialization method.
void UStringTest::setUp()
{
    // Select the directory name and prefix for temporary files
    _tempFilePrefix = ts::TempFile(".");

    // Next file will use suffix "000"
    _nextFileIndex = 0;
}

// Test suite cleanup method.
void UStringTest::tearDown()
{
    // Delete all temporary files
    std::vector<std::string> tempFiles;
    ts::ExpandWildcard(tempFiles, _tempFilePrefix + "*");
    for (std::vector<std::string>::const_iterator i = tempFiles.begin(); i != tempFiles.end(); ++i) {
        utest::Out() << "UStringTest: deleting temporary file \"" << *i << "\"" << std::endl;
        ts::DeleteFile(*i);
    }
    _nextFileIndex = 0;
}

// Get the name of a temporary file from an index
std::string UStringTest::temporaryFileName (int index) const
{
    const std::string name(_tempFilePrefix + ts::Format("%03d", index));
    return name;
}

// Get the name of the next temporary file
std::string UStringTest::newTemporaryFileName()
{
    return temporaryFileName(_nextFileIndex++);
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

    CPPUNIT_ASSERT_USTRINGS_EQUAL(s1, s2);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(s1, s3);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(s1, s4);
}

void UStringTest::testDiacritical()
{
    CPPUNIT_ASSERT(!ts::IsCombiningDiacritical(ts::UChar('a')));
    CPPUNIT_ASSERT(ts::IsCombiningDiacritical(ts::ACUTE_ACCENT));
    CPPUNIT_ASSERT(ts::IsCombiningDiacritical(ts::ARABIC_KASRA));
    CPPUNIT_ASSERT(ts::IsCombiningDiacritical(ts::RIGHT_TO_LEFT_MARK));
}

void UStringTest::testSurrogate()
{
    const ts::UString ab({MATH_A1, MATH_A2, MATH_B1, MATH_B2});
    // Displayed string may be screwed up, depending of the terminal...
    utest::Out() << "UStringTest::testSurrogate: '" << ab << "'" << std::endl;

    CPPUNIT_ASSERT(!ts::IsLeadingSurrogate(ts::UChar('A')));
    CPPUNIT_ASSERT(!ts::IsTrailingSurrogate(ts::UChar('A')));

    CPPUNIT_ASSERT(ts::IsLeadingSurrogate(MATH_A1));
    CPPUNIT_ASSERT(!ts::IsTrailingSurrogate(MATH_A1));
    CPPUNIT_ASSERT(!ts::IsLeadingSurrogate(MATH_A2));
    CPPUNIT_ASSERT(ts::IsTrailingSurrogate(MATH_A2));
    CPPUNIT_ASSERT(ts::IsLeadingSurrogate(MATH_B1));
    CPPUNIT_ASSERT(!ts::IsTrailingSurrogate(MATH_B1));
    CPPUNIT_ASSERT(!ts::IsLeadingSurrogate(MATH_B2));
    CPPUNIT_ASSERT(ts::IsTrailingSurrogate(MATH_B2));
}

void UStringTest::testWidth()
{
    CPPUNIT_ASSERT_EQUAL(size_t(0), ts::UString().width());
    CPPUNIT_ASSERT_EQUAL(size_t(3), ts::UString(u"ABC").width());
    CPPUNIT_ASSERT_EQUAL(size_t(3), (u"A" + ts::UString({ts::ACUTE_ACCENT}) + u"BC").width());
    CPPUNIT_ASSERT_EQUAL(size_t(2), ts::UString({MATH_A1, MATH_A2, MATH_B1, MATH_B2}).width());
}

void UStringTest::testDisplayPosition()
{
    CPPUNIT_ASSERT_EQUAL(size_t(0), ts::UString().displayPosition(0));
    CPPUNIT_ASSERT_EQUAL(size_t(0), ts::UString().displayPosition(5));
    CPPUNIT_ASSERT_EQUAL(size_t(0), ts::UString().displayPosition(6, 7, ts::RIGHT_TO_LEFT));
    CPPUNIT_ASSERT_EQUAL(size_t(2), ts::UString(u"ABCDE").displayPosition(2));

    const ts::UString s({u'A', ts::ACUTE_ACCENT, u'B', u'C', u'D', u'E'});
    CPPUNIT_ASSERT_EQUAL(size_t(6), s.size());
    CPPUNIT_ASSERT_EQUAL(size_t(6), s.length());
    CPPUNIT_ASSERT_EQUAL(size_t(5), s.width());
    CPPUNIT_ASSERT_EQUAL(size_t(3), s.displayPosition(2));
    CPPUNIT_ASSERT_EQUAL(size_t(2), s.displayPosition(1));
    CPPUNIT_ASSERT_EQUAL(size_t(0), s.displayPosition(0));
    CPPUNIT_ASSERT_EQUAL(size_t(5), s.displayPosition(4));
    CPPUNIT_ASSERT_EQUAL(size_t(6), s.displayPosition(5));
    CPPUNIT_ASSERT_EQUAL(size_t(6), s.displayPosition(6));
    CPPUNIT_ASSERT_EQUAL(size_t(6), s.displayPosition(7));
    CPPUNIT_ASSERT_EQUAL(size_t(6), s.displayPosition(8));
    CPPUNIT_ASSERT_EQUAL(size_t(6), s.displayPosition(0, s.length(), ts::RIGHT_TO_LEFT));
    CPPUNIT_ASSERT_EQUAL(size_t(5), s.displayPosition(1, s.length(), ts::RIGHT_TO_LEFT));
    CPPUNIT_ASSERT_EQUAL(size_t(4), s.displayPosition(2, s.length(), ts::RIGHT_TO_LEFT));
    CPPUNIT_ASSERT_EQUAL(size_t(3), s.displayPosition(3, s.length(), ts::RIGHT_TO_LEFT));
    CPPUNIT_ASSERT_EQUAL(size_t(2), s.displayPosition(4, s.length(), ts::RIGHT_TO_LEFT));
    CPPUNIT_ASSERT_EQUAL(size_t(0), s.displayPosition(5, s.length(), ts::RIGHT_TO_LEFT));
    CPPUNIT_ASSERT_EQUAL(size_t(0), s.displayPosition(6, s.length(), ts::RIGHT_TO_LEFT));
    CPPUNIT_ASSERT_EQUAL(size_t(0), s.displayPosition(7, s.length(), ts::RIGHT_TO_LEFT));
    CPPUNIT_ASSERT_EQUAL(size_t(0), s.displayPosition(8, s.length(), ts::RIGHT_TO_LEFT));
}

void UStringTest::testTrim()
{
    ts::UString s;

    s = u"  abc  ";
    s.trim();
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", s);

    s = u"  abc  ";
    s.trim(true, false);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc  ", s);

    s = u"  abc  ";
    s.trim(false, true);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"  abc", s);

    s = u"  abc  ";
    s.trim(false, false);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"  abc  ", s);

    s = u"abc";
    s.trim();
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", s);

    s = u"abc";
    s.trim(true, false);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", s);

    s = u"abc";
    s.trim(false, true);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", s);

    s = u"abc";
    s.trim(false, false);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", s);

    s = u"  abc  ";
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", s.toTrimmed());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc  ", s.toTrimmed(true, false));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"  abc", s.toTrimmed(false, true));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"  abc  ", s.toTrimmed(false, false));

    s = u"abc";
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", s.toTrimmed());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", s.toTrimmed(true, false));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", s.toTrimmed(false, true));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", s.toTrimmed(false, false));
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

    ts::UString s1(u"AbCdEf,%*=UiT");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef,%*=uit", s1.toLower());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"ABCDEF,%*=UIT", s1.toUpper());

    s1 = u"AbCdEf,%*=UiT";
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"AbCdEf,%*=UiT", s1);
    s1.convertToLower();
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef,%*=uit", s1);

    s1 = u"AbCdEf,%*=UiT";
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"AbCdEf,%*=UiT", s1);
    s1.convertToUpper();
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"ABCDEF,%*=UIT", s1);
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

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"X", ts::RemoveAccent('X'));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u",", ts::RemoveAccent(','));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"E", ts::RemoveAccent(ts::LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"c", ts::RemoveAccent(ts::LATIN_SMALL_LETTER_C_WITH_ACUTE));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"C", ts::RemoveAccent(ts::LATIN_CAPITAL_LETTER_C_WITH_CIRCUMFLEX));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"f", ts::RemoveAccent(ts::LATIN_SMALL_F_WITH_HOOK));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"I", ts::RemoveAccent(ts::BLACKLETTER_CAPITAL_I));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"P", ts::RemoveAccent(ts::SCRIPT_CAPITAL_P));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"R", ts::RemoveAccent(ts::BLACKLETTER_CAPITAL_R));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"OE", ts::RemoveAccent(ts::LATIN_CAPITAL_LIGATURE_OE));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"oe", ts::RemoveAccent(ts::LATIN_SMALL_LIGATURE_OE));
}

void UStringTest::testHTML()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"A", ts::ToHTML('A'));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u":", ts::ToHTML(':'));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"&quot;", ts::ToHTML(ts::QUOTATION_MARK));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"&amp;", ts::ToHTML(ts::AMPERSAND));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"&lt;", ts::ToHTML(ts::LESS_THAN_SIGN));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"&gt;", ts::ToHTML(ts::GREATER_THAN_SIGN));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"&nbsp;", ts::ToHTML(ts::NO_BREAK_SPACE));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"&ldquo;", ts::ToHTML(ts::LEFT_DOUBLE_QUOTATION_MARK));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"&diams;", ts::ToHTML(ts::BLACK_DIAMOND_SUIT));

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", ts::UString().toHTML());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdefgh = xyz:", ts::UString(u"abcdefgh = xyz:").toHTML());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"&lt;abcd&gt; = &quot;&amp;", ts::UString(u"<abcd> = \"&").toHTML());
}

void UStringTest::testRemove()
{
    ts::UString s;

    s = u"az zef cer ";
    s.remove(u" ");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"azzefcer", s);

    s = u"fooAZfoo==fooBARfoo";
    s.remove(u"foo");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"AZ==BAR", s);

    s = u"fooAZfoo==fooBARfoo";
    const ts::UString foo1(u"foo");
    s.remove(foo1);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"AZ==BAR", s);

    s = u"fooAZfoo==fooBARfoo";
    s.remove(u"NOTTHERE");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fooAZfoo==fooBARfoo", s);

    s = u"";
    s.remove(u"foo");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", s);

    s = u"fooAZfoo==fooBARfoo";
    s.remove(u"");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fooAZfoo==fooBARfoo", s);

    s = u"fooAZfoo==fooBARfoo";
    s.remove(u"o");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fAZf==fBARf", s);

    s = u"fooAZfoo==fooBARfoo";
    s.remove(u"z");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fooAZfoo==fooBARfoo", s);

    s = u"az zef cer ";
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"azzefcer", s.toRemoved(u" "));

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"AZ==BAR", ts::UString(u"fooAZfoo==fooBARfoo").toRemoved(u"foo"));

    s = u"fooAZfoo==fooBARfoo";
    const ts::UString foo2(u"foo");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"AZ==BAR", s.toRemoved(foo2));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fooAZfoo==fooBARfoo", s.toRemoved(u"NOTTHERE"));

    s = u"";
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", s.toRemoved(u"foo"));

    s = u"fooAZfoo==fooBARfoo";
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fooAZfoo==fooBARfoo", s.toRemoved(u""));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fAZf==fBARf", s.toRemoved(u"o"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fooAZfoo==fooBARfoo", s.toRemoved(u"z"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fooAZfoo==fooBARfoo", s.toRemoved('z'));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fooAZfoo==fooBARfoo", s.toRemoved(ts::UChar('z')));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fAZf==fBARf", s.toRemoved('o'));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fAZf==fBARf", s.toRemoved(ts::UChar('o')));
}

void UStringTest::testSubstitute()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", ts::UString(u"").toSubstituted(u"", u""));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"xyzcdefxyzcdef", ts::UString(u"abcdefabcdef").toSubstituted(u"ab", u"xyz"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdxyzabcdxyz", ts::UString(u"abcdefabcdef").toSubstituted(u"ef", u"xyz"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abbcdbba", ts::UString(u"abcdba").toSubstituted(u"b", u"bb"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdabcd", ts::UString(u"abcdefabcdef").toSubstituted(u"ef", u""));
}

void UStringTest::testSplit()
{
    std::vector<ts::UString> v1;
    ts::UString(u"az, ,  fr,  ze ,t").split(v1);
    CPPUNIT_ASSERT_EQUAL(size_t(5), v1.size());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"az", v1[0]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", v1[1]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fr", v1[2]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"ze", v1[3]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"t", v1[4]);

    std::vector<ts::UString> v2;
    const ts::UString s2(u"az, ,  fr,  ze ,t");
    s2.split(v2);
    CPPUNIT_ASSERT_EQUAL(size_t(5), v2.size());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"az", v2[0]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", v2[1]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fr", v2[2]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"ze", v2[3]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"t", v2[4]);

    std::vector<ts::UString> v3;
    ts::UString(u"az, ,  fr,  ze ,t").split(v3, ts::COMMA, false);
    CPPUNIT_ASSERT_EQUAL(size_t(5), v3.size());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"az", v3[0]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u" ", v3[1]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"  fr", v3[2]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"  ze ", v3[3]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"t", v3[4]);

    std::vector<ts::UString> v4;
    ts::UString(u"az, ,  fr,  ze ,t").split(v4, ts::UChar('z'), false);
    CPPUNIT_ASSERT_EQUAL(size_t(3), v4.size());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"a", v4[0]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u", ,  fr,  ", v4[1]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"e ,t", v4[2]);
}

void UStringTest::testJoin()
{
    std::vector<ts::UString> v;
    v.push_back(u"az");
    v.push_back(u"sd");
    v.push_back(u"tg");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"az, sd, tg", ts::UString::Join(v));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"sd, tg", ts::UString::Join(++v.begin(), v.end()));
}

void UStringTest::testBreakLines()
{
    std::vector<ts::UString> v1;
    ts::UString(u"aze arf erf r+oih zf").splitLines(v1, 8);
    CPPUNIT_ASSERT(v1.size() == 3);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"aze arf", v1[0]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"erf", v1[1]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"r+oih zf", v1[2]);

    std::vector<ts::UString> v2;
    ts::UString(u"aze arf erf r+oih zf").splitLines(v2, 8, "+");
    CPPUNIT_ASSERT(v2.size() == 3);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"aze arf", v2[0]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"erf r+", v2[1]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"oih zf", v2[2]);

    std::vector<ts::UString> v3;
    ts::UString(u"aze arf erf r+oih zf").splitLines(v3, 8, "", "==");
    CPPUNIT_ASSERT(v3.size() == 4);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"aze arf", v3[0]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"==erf", v3[1]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"==r+oih", v3[2]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"==zf", v3[3]);

    std::vector<ts::UString> v4;
    ts::UString(u"aze arf dkvyfngofnb ff").splitLines(v4, 8);
    CPPUNIT_ASSERT(v4.size() == 3);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"aze arf", v4[0]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"dkvyfngofnb", v4[1]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"ff", v4[2]);

    std::vector<ts::UString> v5;
    ts::UString(u"aze arf dkvyfngofnb ff").splitLines(v5, 8, "", "", true);
    CPPUNIT_ASSERT(v5.size() == 3);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"aze arf", v5[0]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"dkvyfngo", v5[1]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fnb ff", v5[2]);
}

void UStringTest::testRemovePrefix()
{
    ts::UString s;

    s = u"abcdef";
    s.removePrefix(u"ab");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"cdef", s);

    s = u"abcdef";
    s.removePrefix(u"xy");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", s);

    s = u"abcdef";
    s.removePrefix(u"");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", s);

    s = u"";
    s.removePrefix(u"ab");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", s);

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"cdef", ts::UString(u"abcdef").toRemovedPrefix(u"ab"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedPrefix(u"xy"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedPrefix(u""));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", ts::UString(u"").toRemovedPrefix(u"ab"));

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedPrefix(u"AB"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedPrefix(u"AB", ts::CASE_SENSITIVE));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"cdef", ts::UString(u"abcdef").toRemovedPrefix(u"AB", ts::CASE_INSENSITIVE));
}

void UStringTest::testRemoveSuffix()
{
    ts::UString s;

    s = u"abcdef";
    s.removeSuffix(u"ef");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcd", s);

    s = u"abcdef";
    s.removeSuffix(u"xy");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", s);

    s = u"abcdef";
    s.removeSuffix(u"");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", s);

    s = u"";
    s.removeSuffix(u"ef");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", s);

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcd", ts::UString(u"abcdef").toRemovedSuffix(u"ef"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedSuffix(u"xy"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedSuffix(u""));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", ts::UString(u"").toRemovedSuffix(u"ef"));

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedSuffix(u"EF"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedSuffix(u"EF", ts::CASE_SENSITIVE));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcd", ts::UString(u"abcdef").toRemovedSuffix(u"EF", ts::CASE_INSENSITIVE));
}

void UStringTest::testStart()
{
    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").startWith(u"azer"));
    CPPUNIT_ASSERT(!ts::UString(u"azertyuiop").startWith(u"aZer"));
    CPPUNIT_ASSERT(!ts::UString(u"azertyuiop").startWith(u"azeR"));

    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").startWith(u"azer", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").startWith(u"aZer", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").startWith(u"azeR", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString(u"azertyuiop").startWith(u"azerq", ts::CASE_INSENSITIVE));

    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").startWith(u""));
    CPPUNIT_ASSERT(!ts::UString(u"azertyuiop").startWith(u"azertyuiopqsdf"));

    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").startWith(u"", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString(u"azertyuiop").startWith(u"azertyuiopqsdf", ts::CASE_INSENSITIVE));

    CPPUNIT_ASSERT(ts::UString(u"").startWith(u""));
    CPPUNIT_ASSERT(!ts::UString(u"").startWith(u"abcd"));

    CPPUNIT_ASSERT(ts::UString(u"").startWith(u"", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString(u"").startWith(u"abcd", ts::CASE_INSENSITIVE));
}

void UStringTest::testEnd()
{
    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").endWith(u"uiop"));
    CPPUNIT_ASSERT(!ts::UString(u"azertyuiop").endWith(u"uiOp"));
    CPPUNIT_ASSERT(!ts::UString(u"azertyuiop").endWith(u"Uiop"));

    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").endWith(u"uiop", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").endWith(u"uiOp", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").endWith(u"Uiop", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString(u"azertyuiop").endWith(u"wuiop", ts::CASE_INSENSITIVE));

    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").endWith(u""));
    CPPUNIT_ASSERT(!ts::UString(u"azertyuiop").endWith(u"qsazertyuiop"));

    CPPUNIT_ASSERT(ts::UString(u"azertyuiop").endWith(u"", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString(u"azertyuiop").endWith(u"qsazertyuiop", ts::CASE_INSENSITIVE));

    CPPUNIT_ASSERT(ts::UString(u"").endWith(u""));
    CPPUNIT_ASSERT(!ts::UString(u"").endWith(u"abcd"));

    CPPUNIT_ASSERT(ts::UString(u"").endWith(u"", ts::CASE_INSENSITIVE));
    CPPUNIT_ASSERT(!ts::UString(u"").endWith(u"abcd", ts::CASE_INSENSITIVE));
}

void UStringTest::testJustifyLeft()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc     ", ts::UString(u"abc").toJustifiedLeft(8));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc.....", ts::UString(u"abc").toJustifiedLeft(8, '.'));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdefghij", ts::UString(u"abcdefghij").toJustifiedLeft(8));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdefgh", ts::UString(u"abcdefghij").toJustifiedLeft(8, ' ', true));
}

void UStringTest::testJustifyRight()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"     abc", ts::UString(u"abc").toJustifiedRight(8));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u".....abc", ts::UString(u"abc").toJustifiedRight(8, '.'));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdefghij", ts::UString(u"abcdefghij").toJustifiedRight(8));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"cdefghij", ts::UString(u"abcdefghij").toJustifiedRight(8, ' ', true));
}

void UStringTest::testJustifyCentered()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"  abc   ", ts::UString(u"abc").toJustifiedCentered(8));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"..abc...", ts::UString(u"abc").toJustifiedCentered(8, '.'));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdefghij", ts::UString(u"abcdefghij").toJustifiedCentered(8));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdefgh", ts::UString(u"abcdefghij").toJustifiedCentered(8, ' ', true));
}

void UStringTest::testJustify()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc  def", ts::UString(u"abc").toJustified(u"def", 8));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc..def", ts::UString(u"abc").toJustified(u"def", 8, '.'));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdefgh", ts::UString(u"abcd").toJustified(u"efgh", 8));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdefghij", ts::UString(u"abcde").toJustified(u"fghij", 8));
}

void UStringTest::testYesNo()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"yes", ts::UString::YesNo(true));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"no", ts::UString::YesNo(false));
}

void UStringTest::testTrueFalse()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"true", ts::UString::TrueFalse(true));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"false", ts::UString::TrueFalse(false));
}

void UStringTest::testOnOff()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"on", ts::UString::OnOff(true));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"off", ts::UString::OnOff(false));
}

void UStringTest::testSimilarStrings()
{
    CPPUNIT_ASSERT(ts::UString(u"").similar(u""));
    CPPUNIT_ASSERT(ts::UString(u"aZer tY").similar(u"  AZE R T Y    "));
    CPPUNIT_ASSERT(ts::UString(u"  AZE R T Y    ").similar(u"aZer tY"));
    CPPUNIT_ASSERT(!ts::UString(u"").similar(u"az"));
    CPPUNIT_ASSERT(!ts::UString(u"az").similar(u""));
}

void UStringTest::testLoadSave()
{
    std::list<ts::UString> ref;

    ts::UChar c = ts::LATIN_CAPITAL_LETTER_A_WITH_MACRON;
    for (int i = 1; i <= 20; ++i) {
        ref.push_back(ts::UString(2, c++) + ts::Format(", line %d", i));
    }
    CPPUNIT_ASSERT(ref.size() == 20);

    const std::string file1(newTemporaryFileName());
    CPPUNIT_ASSERT(ts::UString::Save(ref, file1));

    std::list<ts::UString> load1;
    CPPUNIT_ASSERT(ts::UString::Load(load1, file1));
    CPPUNIT_ASSERT(load1.size() == 20);
    CPPUNIT_ASSERT(load1 == ref);

    const std::list<ts::UString>::const_iterator refFirst = ++(ref.begin());
    const std::list<ts::UString>::const_iterator refLast = --(ref.end());

    const std::string file2(newTemporaryFileName());
    CPPUNIT_ASSERT(ts::UString::Save(refFirst, refLast, file2));

    std::list<ts::UString> ref2(refFirst, refLast);
    CPPUNIT_ASSERT(ref2.size() == 18);

    std::list<ts::UString> load2;
    CPPUNIT_ASSERT(ts::UString::Load(load2, file2));
    CPPUNIT_ASSERT(load2.size() == 18);
    CPPUNIT_ASSERT(load2 == ref2);

    std::list<ts::UString> ref3;
    ref3.push_back(u"abcdef");
    ref3.insert(ref3.end(), refFirst, refLast);
    CPPUNIT_ASSERT(ref3.size() == 19);

    std::list<ts::UString> load3;
    load3.push_back(u"abcdef");
    CPPUNIT_ASSERT(ts::UString::LoadAppend(load3, file2));
    CPPUNIT_ASSERT(load3.size() == 19);
    CPPUNIT_ASSERT(load3 == ref3);
}

void UStringTest::testToDigit()
{
    CPPUNIT_ASSERT_EQUAL(0,  ts::ToDigit(ts::UChar('0')));
    CPPUNIT_ASSERT_EQUAL(9,  ts::ToDigit(ts::UChar('9')));
    CPPUNIT_ASSERT_EQUAL(-1, ts::ToDigit(ts::UChar('a')));
    CPPUNIT_ASSERT_EQUAL(-1, ts::ToDigit(ts::UChar('f')));
    CPPUNIT_ASSERT_EQUAL(-1, ts::ToDigit(ts::UChar('z')));
    CPPUNIT_ASSERT_EQUAL(10, ts::ToDigit(ts::UChar('a'), 16));
    CPPUNIT_ASSERT_EQUAL(15, ts::ToDigit(ts::UChar('f'), 16));
    CPPUNIT_ASSERT_EQUAL(-1, ts::ToDigit(ts::UChar('z'), 16));
    CPPUNIT_ASSERT_EQUAL(10, ts::ToDigit(ts::UChar('a'), 36));
    CPPUNIT_ASSERT_EQUAL(15, ts::ToDigit(ts::UChar('f'), 36));
    CPPUNIT_ASSERT_EQUAL(35, ts::ToDigit(ts::UChar('z'), 36));
    CPPUNIT_ASSERT_EQUAL(10, ts::ToDigit(ts::UChar('A'), 16));
    CPPUNIT_ASSERT_EQUAL(15, ts::ToDigit(ts::UChar('F'), 16));
    CPPUNIT_ASSERT_EQUAL(-1, ts::ToDigit(ts::UChar('Z'), 16));
    CPPUNIT_ASSERT_EQUAL(10, ts::ToDigit(ts::UChar('A'), 36));
    CPPUNIT_ASSERT_EQUAL(15, ts::ToDigit(ts::UChar('F'), 36));
    CPPUNIT_ASSERT_EQUAL(35, ts::ToDigit(ts::UChar('Z'), 36));
    CPPUNIT_ASSERT_EQUAL(-1, ts::ToDigit(ts::UChar('?')));
    CPPUNIT_ASSERT_EQUAL(-2, ts::ToDigit(ts::UChar('?'), 10, -2));
}

void UStringTest::testToInteger()
{
    int      i;
    uint32_t ui32;
    uint64_t ui64;
    int64_t  i64;

    CPPUNIT_ASSERT(ts::UString(u"1").toInteger(i));
    CPPUNIT_ASSERT_EQUAL(1, i);

    CPPUNIT_ASSERT(ts::UString(u"-001").toInteger(i));
    CPPUNIT_ASSERT_EQUAL(-1, i);

    CPPUNIT_ASSERT(ts::UString(u"   -0xA0  ").toInteger(i));
    CPPUNIT_ASSERT_EQUAL(-160, i);

    CPPUNIT_ASSERT(!ts::UString(u"").toInteger(i));
    CPPUNIT_ASSERT_EQUAL(0, i);

    CPPUNIT_ASSERT(ts::UString(u"123").toInteger(ui32));
    CPPUNIT_ASSERT_EQUAL(uint32_t(123), ui32);

    CPPUNIT_ASSERT(!ts::UString(u"-123").toInteger(ui32));
    CPPUNIT_ASSERT_EQUAL(uint32_t(0), ui32);

    CPPUNIT_ASSERT(ts::UString(u"0").toInteger(ui64));
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0), ui64);

    CPPUNIT_ASSERT(ts::UString(u"0xffffffffFFFFFFFF").toInteger(ui64));
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0XFFFFFFFFFFFFFFFF), ui64);

    CPPUNIT_ASSERT(ts::UString(u"0x7fffffffFFFFFFFF").toInteger(ui64));
    CPPUNIT_ASSERT_EQUAL(TS_UCONST64(0X7FFFFFFFFFFFFFFF), ui64);

    CPPUNIT_ASSERT(ts::UString(u"0").toInteger(i64));
    CPPUNIT_ASSERT_EQUAL(TS_CONST64(0), i64);

    CPPUNIT_ASSERT(ts::UString(u"0x7fffffffFFFFFFFF").toInteger(i64));
    CPPUNIT_ASSERT_EQUAL(TS_CONST64(0X7FFFFFFFFFFFFFFF), i64);

    CPPUNIT_ASSERT(ts::UString(u" 12,345").toInteger(i, u",."));
    CPPUNIT_ASSERT_EQUAL(12345, i);

    CPPUNIT_ASSERT(ts::UString(u" -12.345").toInteger(i, u",."));
    CPPUNIT_ASSERT_EQUAL(-12345, i);

    CPPUNIT_ASSERT(!ts::UString(u" -12;345").toInteger(i, u",."));
    CPPUNIT_ASSERT_EQUAL(-12, i);

    std::list<int32_t> i32List;
    std::list<int32_t> i32Ref;

    i32Ref.clear();
    i32Ref.push_back(-12345);
    i32Ref.push_back(256);
    i32Ref.push_back(0);
    i32Ref.push_back(7);

    CPPUNIT_ASSERT(ts::UString(u"-12345 0x100 0 7").toIntegers(i32List));
    CPPUNIT_ASSERT(i32Ref == i32List);

    CPPUNIT_ASSERT(ts::UString(u" , -12345    0x100 ,  0,  7  ").toIntegers(i32List));
    CPPUNIT_ASSERT(i32Ref == i32List);

    CPPUNIT_ASSERT(!ts::UString(u" , -12345    0x100 ,  0,  7  xxx 45").toIntegers(i32List));
    CPPUNIT_ASSERT(i32Ref == i32List);
}

void UStringTest::testHexaDecode()
{
    ts::ByteBlock bytes;

    CPPUNIT_ASSERT(ts::UString(u"0123456789ABCDEF").hexaDecode(bytes));
    CPPUNIT_ASSERT(bytes == ts::ByteBlock({0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}));

    CPPUNIT_ASSERT(ts::UString(u" 0 1234 56 789 ABC DEF ").hexaDecode(bytes));
    CPPUNIT_ASSERT(bytes == ts::ByteBlock({0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}));

    CPPUNIT_ASSERT(!ts::UString(u" 0 1234 56 - 789 ABC DEF ").hexaDecode(bytes));
    CPPUNIT_ASSERT(bytes == ts::ByteBlock({0x01, 0x23, 0x45}));

    CPPUNIT_ASSERT(!ts::UString(u"X 0 1234 56 - 789 ABC DEF ").hexaDecode(bytes));
    CPPUNIT_ASSERT(bytes.empty());
}

void UStringTest::testAppendContainer()
{
    const char* arr1[] = {"ab", "cde", "", "fghi"};
    ts::UStringList var;
    ts::UStringList ref;

    var.push_back(u"begin");

    ref.push_back(u"begin");
    ref.push_back(u"ab");
    ref.push_back(u"cde");
    ref.push_back(u"");
    ref.push_back(u"fghi");

    CPPUNIT_ASSERT(ts::UString::Append(var, 4, arr1) == ref);

    char* arr2[] = {(char*)"ab", (char*)"cde", (char*)"", (char*)"fghi"};

    var.clear();
    var.push_back(u"begin");
    CPPUNIT_ASSERT(ts::UString::Append(var, 4, arr2) == ref);
}

void UStringTest::testAssignContainer()
{
    const char* arr1[] = {"ab", "cde", "", "fghi"};
    ts::UStringList var;
    ts::UStringList ref;

    var.push_back(u"previous");

    ref.push_back(u"ab");
    ref.push_back(u"cde");
    ref.push_back(u"");
    ref.push_back(u"fghi");

    CPPUNIT_ASSERT(ts::UString::Assign(var, 4, arr1) == ref);

    char* arr2[] = {(char*)"ab", (char*)"cde", (char*)"", (char*)"fghi"};

    var.clear();
    var.push_back(u"other");
    CPPUNIT_ASSERT(ts::UString::Assign(var, 4, arr2) == ref);
}

void UStringTest::testDecimal()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0", ts::UString::Decimal(0));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0", ts::UString::Decimal(0L));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0", ts::UString::Decimal(-0));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0", ts::UString::Decimal(TS_CONST64(0)));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"1,234", ts::UString::Decimal(1234));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"     1,234", ts::UString::Decimal(1234, 10));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"     1,234", ts::UString::Decimal(1234, 10, true));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"1,234     ", ts::UString::Decimal(1234, 10, false));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"      1234", ts::UString::Decimal(1234, 10, true, u""));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"  1()234()567()890", ts::UString::Decimal(1234567890, 18, true, u"()"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"    +1,234", ts::UString::Decimal(1234, 10, true, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, true));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"    -1,234", ts::UString::Decimal(-1234, 10, true, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, true));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"    -1,234", ts::UString::Decimal(-1234, 10, true, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, false));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"-1,234,567,890,123,456", ts::UString::Decimal(TS_CONST64(-1234567890123456)));
}

void UStringTest::testHexa()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0x00", ts::UString::Hexa<uint8_t>(0));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0x00000123", ts::UString::Hexa<uint32_t>(0x123));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0x0000000000000123", ts::UString::Hexa(TS_UCONST64(0x123)));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0xFFFFFFFFFFFFFFFD", ts::UString::Hexa(TS_CONST64(-3)));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0xfffffffffffffffd", ts::UString::Hexa(TS_CONST64(-3), 0, ts::UString(), true, false));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0x002", ts::UString::Hexa<uint16_t>(0x02, 3));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0x000002", ts::UString::Hexa<uint16_t>(0x02, 6));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0x0000<>0123", ts::UString::Hexa<uint32_t>(0x123, 0, u"<>"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0000,0123", ts::UString::Hexa<uint32_t>(0x123, 0, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, false));
}
