//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for the UString class.
//
//----------------------------------------------------------------------------

#include "tsUString.h"
#include "tsByteBlock.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsCerrReport.h"
#include "tsIPSocketAddress.h"
#include "tsTS.h"
#include "tsunit.h"

//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class UStringTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(IsSpace);
    TSUNIT_DECLARE_TEST(UTF);
    TSUNIT_DECLARE_TEST(Diacritical);
    TSUNIT_DECLARE_TEST(Surrogate);
    TSUNIT_DECLARE_TEST(FromWChar);
    TSUNIT_DECLARE_TEST(Width);
    TSUNIT_DECLARE_TEST(DisplayPosition);
    TSUNIT_DECLARE_TEST(Trim);
    TSUNIT_DECLARE_TEST(LetterCase);
    TSUNIT_DECLARE_TEST(Accent);
    TSUNIT_DECLARE_TEST(ToHTML);
    TSUNIT_DECLARE_TEST(FromHTML);
    TSUNIT_DECLARE_TEST(ToJSON);
    TSUNIT_DECLARE_TEST(FromJSON);
    TSUNIT_DECLARE_TEST(Remove);
    TSUNIT_DECLARE_TEST(Substitute);
    TSUNIT_DECLARE_TEST(Split);
    TSUNIT_DECLARE_TEST(SplitShellStyle);
    TSUNIT_DECLARE_TEST(Join);
    TSUNIT_DECLARE_TEST(BreakLines);
    TSUNIT_DECLARE_TEST(RemovePrefix);
    TSUNIT_DECLARE_TEST(RemoveSuffix);
    TSUNIT_DECLARE_TEST(Start);
    TSUNIT_DECLARE_TEST(End);
    TSUNIT_DECLARE_TEST(JustifyLeft);
    TSUNIT_DECLARE_TEST(JustifyRight);
    TSUNIT_DECLARE_TEST(JustifyCentered);
    TSUNIT_DECLARE_TEST(Justify);
    TSUNIT_DECLARE_TEST(YesNo);
    TSUNIT_DECLARE_TEST(TrueFalse);
    TSUNIT_DECLARE_TEST(OnOff);
    TSUNIT_DECLARE_TEST(SimilarStrings);
    TSUNIT_DECLARE_TEST(LoadSave);
    TSUNIT_DECLARE_TEST(ToDigit);
    TSUNIT_DECLARE_TEST(ToInteger);
    TSUNIT_DECLARE_TEST(ToTristate);
    TSUNIT_DECLARE_TEST(HexaDecode);
    TSUNIT_DECLARE_TEST(AppendContainer);
    TSUNIT_DECLARE_TEST(AssignContainer);
    TSUNIT_DECLARE_TEST(Decimal);
    TSUNIT_DECLARE_TEST(DecimalList);
    TSUNIT_DECLARE_TEST(Hexa);
    TSUNIT_DECLARE_TEST(HexaDump);
    TSUNIT_DECLARE_TEST(ArgMixIn);
    TSUNIT_DECLARE_TEST(ArgMixOut);
    TSUNIT_DECLARE_TEST(ArgMixOutFloat);
    TSUNIT_DECLARE_TEST(Format);
    TSUNIT_DECLARE_TEST(Scan);
    TSUNIT_DECLARE_TEST(CommonPrefix);
    TSUNIT_DECLARE_TEST(CommonSuffix);
    TSUNIT_DECLARE_TEST(Precombined);
    TSUNIT_DECLARE_TEST(Quoted);
    TSUNIT_DECLARE_TEST(Unquoted);
    TSUNIT_DECLARE_TEST(ToQuotedLine);
    TSUNIT_DECLARE_TEST(FromQuotedLine);
    TSUNIT_DECLARE_TEST(Indent);
    TSUNIT_DECLARE_TEST(ToFloat);
    TSUNIT_DECLARE_TEST(SuperCompare);
    TSUNIT_DECLARE_TEST(ChronoUnit);
    TSUNIT_DECLARE_TEST(Chrono);
    TSUNIT_DECLARE_TEST(Duration);
    TSUNIT_DECLARE_TEST(Percentage);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    fs::path _tempFilePrefix {};
    int _nextFileIndex = 0;
    ts::UString temporaryFileName(int) const;
    ts::UString newTemporaryFileName();

    void testArgMixInCalled1(std::initializer_list<ts::ArgMixIn> list);
    void testArgMixInCalled2(std::initializer_list<ts::ArgMixIn> list);
    void testArgMixOutCalled(std::initializer_list<ts::ArgMixOut> list);

    // Two sample Unicode characters from the supplementary planes:
    //   U+1D538: MATHEMATICAL DOUBLE-STRUCK CAPITAL A
    //   U+1D539: MATHEMATICAL DOUBLE-STRUCK CAPITAL B
    static constexpr ts::UChar MATH_A1 = ts::UChar(0xD800 | (0x1D538 >> 10));
    static constexpr ts::UChar MATH_A2 = ts::UChar(0xDC00 | (0x1D538 & 0x03FF));
    static constexpr ts::UChar MATH_B1 = ts::UChar(0xD800 | (0x1D539 >> 10));
    static constexpr ts::UChar MATH_B2 = ts::UChar(0xDC00 | (0x1D539 & 0x03FF));
};

TSUNIT_REGISTER(UStringTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void UStringTest::beforeTest()
{
    // Select the directory name and prefix for temporary files
    _tempFilePrefix = ts::TempFile(u".");

    // Next file will use suffix "000"
    _nextFileIndex = 0;
}

// Test suite cleanup method.
void UStringTest::afterTest()
{
    // Delete all temporary files
    ts::UStringVector tempFiles;
    ts::ExpandWildcard(tempFiles, _tempFilePrefix + u"*");
    for (const auto& file : tempFiles) {
        debug() << "UStringTest: deleting temporary file \"" << file << "\"" << std::endl;
        fs::remove(file, &ts::ErrCodeReport(CERR, u"error deleting", file));
    }
    _nextFileIndex = 0;
}

// Get the name of a temporary file from an index
ts::UString UStringTest::temporaryFileName (int index) const
{
    return _tempFilePrefix + ts::UString::Format(u"%03d", index);
}

// Get the name of the next temporary file
ts::UString UStringTest::newTemporaryFileName()
{
    return temporaryFileName(_nextFileIndex++);
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(IsSpace)
{
    TSUNIT_ASSERT(ts::IsSpace(ts::SPACE));
    TSUNIT_ASSERT(ts::IsSpace(ts::LINE_FEED));
    TSUNIT_ASSERT(ts::IsSpace(ts::CARRIAGE_RETURN));
    TSUNIT_ASSERT(ts::IsSpace(ts::HORIZONTAL_TABULATION));
    TSUNIT_ASSERT(ts::IsSpace(ts::VERTICAL_TABULATION));
    TSUNIT_ASSERT(ts::IsSpace(ts::FORM_FEED));
    TSUNIT_ASSERT(!ts::IsSpace(ts::LATIN_CAPITAL_LETTER_A));
    TSUNIT_ASSERT(!ts::IsSpace(ts::COLON));
    TSUNIT_ASSERT(!ts::IsSpace(ts::CHAR_NULL));
}

TSUNIT_DEFINE_TEST(UTF)
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
    ts::UString s3(ts::UString::FromUTF8(reinterpret_cast<const char*>(utf8_bytes)));
    ts::UString s4(ts::UString::FromUTF8(reinterpret_cast<const char*>(utf8_bytes), utf8_count));

    debug() << "UStringTest::testUTF: utf16_count = " << utf16_count << ", s1.length() = " << s1.length() << std::endl;

    TSUNIT_EQUAL(s1.length(), s1.size());
    TSUNIT_EQUAL(s2.length(), s2.size());
    TSUNIT_EQUAL(s3.length(), s3.size());
    TSUNIT_EQUAL(s4.length(), s4.size());

    TSUNIT_EQUAL(s1.length(), utf16_count);
    TSUNIT_EQUAL(s2.length(), utf16_count);
    TSUNIT_EQUAL(s3.length(), utf16_count);
    TSUNIT_EQUAL(s4.length(), utf16_count);

    TSUNIT_EQUAL(s1, s2);
    TSUNIT_EQUAL(s1, s3);
    TSUNIT_EQUAL(s1, s4);
}

TSUNIT_DEFINE_TEST(Diacritical)
{
    TSUNIT_ASSERT(!ts::IsCombiningDiacritical(ts::UChar('a')));
    TSUNIT_ASSERT(ts::IsCombiningDiacritical(ts::ACUTE_ACCENT));
    TSUNIT_ASSERT(ts::IsCombiningDiacritical(ts::ARABIC_KASRA));
    TSUNIT_ASSERT(ts::IsCombiningDiacritical(ts::RIGHT_TO_LEFT_MARK));
}

TSUNIT_DEFINE_TEST(Surrogate)
{
    const ts::UString ab({MATH_A1, MATH_A2, MATH_B1, MATH_B2});
    // Displayed string may be screwed up, depending of the terminal...
    debug() << "UStringTest::testSurrogate: '" << ab << "'" << std::endl;

    TSUNIT_ASSERT(!ts::IsLeadingSurrogate(ts::UChar('A')));
    TSUNIT_ASSERT(!ts::IsTrailingSurrogate(ts::UChar('A')));

    TSUNIT_ASSERT(ts::IsLeadingSurrogate(MATH_A1));
    TSUNIT_ASSERT(!ts::IsTrailingSurrogate(MATH_A1));
    TSUNIT_ASSERT(!ts::IsLeadingSurrogate(MATH_A2));
    TSUNIT_ASSERT(ts::IsTrailingSurrogate(MATH_A2));
    TSUNIT_ASSERT(ts::IsLeadingSurrogate(MATH_B1));
    TSUNIT_ASSERT(!ts::IsTrailingSurrogate(MATH_B1));
    TSUNIT_ASSERT(!ts::IsLeadingSurrogate(MATH_B2));
    TSUNIT_ASSERT(ts::IsTrailingSurrogate(MATH_B2));
}

TSUNIT_DEFINE_TEST(FromWChar)
{
    TSUNIT_EQUAL(u"", ts::UString::FromWChar(nullptr));
    TSUNIT_EQUAL(u"", ts::UString::FromWChar(L""));
    TSUNIT_EQUAL(u"abcdef", ts::UString::FromWChar(L"abcdef"));
}

TSUNIT_DEFINE_TEST(Width)
{
    TSUNIT_EQUAL(0, ts::UString().width());
    TSUNIT_EQUAL(3, ts::UString(u"ABC").width());
    TSUNIT_EQUAL(3, (u"A" + ts::UString({ts::ACUTE_ACCENT}) + u"BC").width());
    TSUNIT_EQUAL(2, ts::UString({MATH_A1, MATH_A2, MATH_B1, MATH_B2}).width());
}

TSUNIT_DEFINE_TEST(DisplayPosition)
{
    TSUNIT_EQUAL(0, ts::UString().displayPosition(0));
    TSUNIT_EQUAL(0, ts::UString().displayPosition(5));
    TSUNIT_EQUAL(0, ts::UString().displayPosition(6, 7, ts::RIGHT_TO_LEFT));
    TSUNIT_EQUAL(2, ts::UString(u"ABCDE").displayPosition(2));

    const ts::UString s({u'A', ts::ACUTE_ACCENT, u'B', u'C', u'D', u'E'});
    TSUNIT_EQUAL(6, s.size());
    TSUNIT_EQUAL(6, s.length());
    TSUNIT_EQUAL(5, s.width());
    TSUNIT_EQUAL(3, s.displayPosition(2));
    TSUNIT_EQUAL(2, s.displayPosition(1));
    TSUNIT_EQUAL(0, s.displayPosition(0));
    TSUNIT_EQUAL(5, s.displayPosition(4));
    TSUNIT_EQUAL(6, s.displayPosition(5));
    TSUNIT_EQUAL(6, s.displayPosition(6));
    TSUNIT_EQUAL(6, s.displayPosition(7));
    TSUNIT_EQUAL(6, s.displayPosition(8));
    TSUNIT_EQUAL(6, s.displayPosition(0, s.length(), ts::RIGHT_TO_LEFT));
    TSUNIT_EQUAL(5, s.displayPosition(1, s.length(), ts::RIGHT_TO_LEFT));
    TSUNIT_EQUAL(4, s.displayPosition(2, s.length(), ts::RIGHT_TO_LEFT));
    TSUNIT_EQUAL(3, s.displayPosition(3, s.length(), ts::RIGHT_TO_LEFT));
    TSUNIT_EQUAL(2, s.displayPosition(4, s.length(), ts::RIGHT_TO_LEFT));
    TSUNIT_EQUAL(0, s.displayPosition(5, s.length(), ts::RIGHT_TO_LEFT));
    TSUNIT_EQUAL(0, s.displayPosition(6, s.length(), ts::RIGHT_TO_LEFT));
    TSUNIT_EQUAL(0, s.displayPosition(7, s.length(), ts::RIGHT_TO_LEFT));
    TSUNIT_EQUAL(0, s.displayPosition(8, s.length(), ts::RIGHT_TO_LEFT));
}

TSUNIT_DEFINE_TEST(Trim)
{
    ts::UString s;

    s = u"  abc  ";
    s.trim();
    TSUNIT_EQUAL(u"abc", s);

    s = u"  abc  ";
    s.trim(true, false);
    TSUNIT_EQUAL(u"abc  ", s);

    s = u"  abc  ";
    s.trim(false, true);
    TSUNIT_EQUAL(u"  abc", s);

    s = u"  abc  ";
    s.trim(false, false);
    TSUNIT_EQUAL(u"  abc  ", s);

    s = u"abc";
    s.trim();
    TSUNIT_EQUAL(u"abc", s);

    s = u"abc";
    s.trim(true, false);
    TSUNIT_EQUAL(u"abc", s);

    s = u"abc";
    s.trim(false, true);
    TSUNIT_EQUAL(u"abc", s);

    s = u"abc";
    s.trim(false, false);
    TSUNIT_EQUAL(u"abc", s);

    s = u"  abc  ";
    TSUNIT_EQUAL(u"abc", s.toTrimmed());
    TSUNIT_EQUAL(u"abc  ", s.toTrimmed(true, false));
    TSUNIT_EQUAL(u"  abc", s.toTrimmed(false, true));
    TSUNIT_EQUAL(u"  abc  ", s.toTrimmed(false, false));

    s = u"abc";
    TSUNIT_EQUAL(u"abc", s.toTrimmed());
    TSUNIT_EQUAL(u"abc", s.toTrimmed(true, false));
    TSUNIT_EQUAL(u"abc", s.toTrimmed(false, true));
    TSUNIT_EQUAL(u"abc", s.toTrimmed(false, false));

    s = {ts::SPACE, u'a', ts::CARRIAGE_RETURN, u' ', ts::LINE_FEED, ts::HORIZONTAL_TABULATION, u'b', ts::VERTICAL_TABULATION, u' '};
    const ts::UString s1 = {u'a', ts::CARRIAGE_RETURN, u' ', ts::LINE_FEED, ts::HORIZONTAL_TABULATION, u'b'};
    TSUNIT_EQUAL(s1, s.toTrimmed());
    TSUNIT_EQUAL(u"a b", s.toTrimmed(true, true, true));
}

TSUNIT_DEFINE_TEST(LetterCase)
{
    TSUNIT_ASSERT(!ts::IsLower(ts::COMMA));
    TSUNIT_ASSERT(!ts::IsUpper(ts::COMMA));

    TSUNIT_EQUAL(ts::COMMA, ts::ToLower(ts::COMMA));
    TSUNIT_EQUAL(ts::COMMA, ts::ToUpper(ts::COMMA));

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
    static constexpr size_t tabSize = sizeof(tab) / sizeof(tab[0]);

    for (size_t i = 0; i < tabSize; ++i) {
        TSUNIT_ASSERT(ts::IsUpper(tab[i].upper));
        TSUNIT_ASSERT(!ts::IsLower(tab[i].upper));
        TSUNIT_ASSERT(ts::IsLower(tab[i].lower));
        TSUNIT_ASSERT(!ts::IsUpper(tab[i].lower));
        TSUNIT_EQUAL(tab[i].lower, ts::ToLower(tab[i].lower));
        TSUNIT_EQUAL(tab[i].lower, ts::ToLower(tab[i].upper));
        TSUNIT_EQUAL(tab[i].upper, ts::ToUpper(tab[i].lower));
        TSUNIT_EQUAL(tab[i].upper, ts::ToUpper(tab[i].upper));
    }

    ts::UString s1(u"AbCdEf,%*=UiT");
    TSUNIT_EQUAL(u"abcdef,%*=uit", s1.toLower());
    TSUNIT_EQUAL(u"ABCDEF,%*=UIT", s1.toUpper());

    s1 = u"AbCdEf,%*=UiT";
    TSUNIT_EQUAL(u"AbCdEf,%*=UiT", s1);
    s1.convertToLower();
    TSUNIT_EQUAL(u"abcdef,%*=uit", s1);

    s1 = u"AbCdEf,%*=UiT";
    TSUNIT_EQUAL(u"AbCdEf,%*=UiT", s1);
    s1.convertToUpper();
    TSUNIT_EQUAL(u"ABCDEF,%*=UIT", s1);
}

TSUNIT_DEFINE_TEST(Accent)
{
    TSUNIT_ASSERT(!ts::IsAccented('A'));
    TSUNIT_ASSERT(!ts::IsAccented(':'));
    TSUNIT_ASSERT(ts::IsAccented(ts::LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS));
    TSUNIT_ASSERT(ts::IsAccented(ts::LATIN_CAPITAL_LETTER_C_WITH_CIRCUMFLEX));
    TSUNIT_ASSERT(ts::IsAccented(ts::BLACKLETTER_CAPITAL_I));
    TSUNIT_ASSERT(ts::IsAccented(ts::SCRIPT_CAPITAL_P));
    TSUNIT_ASSERT(ts::IsAccented(ts::BLACKLETTER_CAPITAL_R));
    TSUNIT_ASSERT(ts::IsAccented(ts::LATIN_CAPITAL_LIGATURE_OE));

    TSUNIT_EQUAL(u"X", ts::RemoveAccent('X'));
    TSUNIT_EQUAL(u",", ts::RemoveAccent(','));
    TSUNIT_EQUAL(u"E", ts::RemoveAccent(ts::LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS));
    TSUNIT_EQUAL(u"c", ts::RemoveAccent(ts::LATIN_SMALL_LETTER_C_WITH_ACUTE));
    TSUNIT_EQUAL(u"C", ts::RemoveAccent(ts::LATIN_CAPITAL_LETTER_C_WITH_CIRCUMFLEX));
    TSUNIT_EQUAL(u"f", ts::RemoveAccent(ts::LATIN_SMALL_F_WITH_HOOK));
    TSUNIT_EQUAL(u"I", ts::RemoveAccent(ts::BLACKLETTER_CAPITAL_I));
    TSUNIT_EQUAL(u"P", ts::RemoveAccent(ts::SCRIPT_CAPITAL_P));
    TSUNIT_EQUAL(u"R", ts::RemoveAccent(ts::BLACKLETTER_CAPITAL_R));
    TSUNIT_EQUAL(u"OE", ts::RemoveAccent(ts::LATIN_CAPITAL_LIGATURE_OE));
    TSUNIT_EQUAL(u"oe", ts::RemoveAccent(ts::LATIN_SMALL_LIGATURE_OE));
}

TSUNIT_DEFINE_TEST(ToHTML)
{
    TSUNIT_EQUAL(u"A", ts::ToHTML('A'));
    TSUNIT_EQUAL(u":", ts::ToHTML(':'));
    TSUNIT_EQUAL(u"&quot;", ts::ToHTML(ts::QUOTATION_MARK));
    TSUNIT_EQUAL(u"&amp;", ts::ToHTML(ts::AMPERSAND));
    TSUNIT_EQUAL(u"&lt;", ts::ToHTML(ts::LESS_THAN_SIGN));
    TSUNIT_EQUAL(u"&gt;", ts::ToHTML(ts::GREATER_THAN_SIGN));
    TSUNIT_EQUAL(u"&nbsp;", ts::ToHTML(ts::NO_BREAK_SPACE));
    TSUNIT_EQUAL(u"&ldquo;", ts::ToHTML(ts::LEFT_DOUBLE_QUOTATION_MARK));
    TSUNIT_EQUAL(u"&diams;", ts::ToHTML(ts::BLACK_DIAMOND_SUIT));

    TSUNIT_EQUAL(u"", ts::UString().toHTML());
    TSUNIT_EQUAL(u"abcdefgh = xyz:", ts::UString(u"abcdefgh = xyz:").toHTML());
    TSUNIT_EQUAL(u"&lt;abcd&gt; = &quot;&amp;", ts::UString(u"<abcd> = \"&").toHTML());
}

TSUNIT_DEFINE_TEST(FromHTML)
{
    TSUNIT_EQUAL(ts::CHAR_NULL, ts::FromHTML(u"A"));
    TSUNIT_EQUAL(ts::QUOTATION_MARK, ts::FromHTML(u"quot"));
    TSUNIT_EQUAL(ts::AMPERSAND, ts::FromHTML(u"amp"));
    TSUNIT_EQUAL(ts::LESS_THAN_SIGN, ts::FromHTML(u"lt"));
    TSUNIT_EQUAL(ts::GREATER_THAN_SIGN, ts::FromHTML(u"gt"));
    TSUNIT_EQUAL(ts::NO_BREAK_SPACE, ts::FromHTML(u"nbsp"));
    TSUNIT_EQUAL(ts::LEFT_DOUBLE_QUOTATION_MARK, ts::FromHTML(u"ldquo"));
    TSUNIT_EQUAL(ts::BLACK_DIAMOND_SUIT, ts::FromHTML(u"diams"));

    TSUNIT_EQUAL(u"", ts::UString().fromHTML());
    TSUNIT_EQUAL(u"abcdefgh = xyz:", ts::UString(u"abcdefgh = xyz:").fromHTML());
    TSUNIT_EQUAL(u"<abcd> = \"&", ts::UString(u"&lt;abcd&gt; = &quot;&amp;").fromHTML());
}

TSUNIT_DEFINE_TEST(ToJSON)
{
    TSUNIT_EQUAL(u"", ts::UString().toJSON());
    TSUNIT_EQUAL(u"abc", ts::UString(u"abc").toJSON());
    TSUNIT_EQUAL(u"ab\\nfoo\\t\\\"", ts::UString(u"ab\nfoo\t\"").toJSON());
}

TSUNIT_DEFINE_TEST(FromJSON)
{
    TSUNIT_EQUAL(u"", ts::UString().fromJSON());
    TSUNIT_EQUAL(u"abc", ts::UString(u"abc").fromJSON());
    TSUNIT_EQUAL(u"ab\n\"a", ts::UString(u"ab\\n\\\"\\u0061").fromJSON());
}

TSUNIT_DEFINE_TEST(Remove)
{
    ts::UString s;

    s = u"az zef cer ";
    s.remove(u" ");
    TSUNIT_EQUAL(u"azzefcer", s);

    s = u"fooAZfoo==fooBARfoo";
    s.remove(u"foo");
    TSUNIT_EQUAL(u"AZ==BAR", s);

    s = u"fooAZfoo==fooBARfoo";
    const ts::UString foo1(u"foo");
    s.remove(foo1);
    TSUNIT_EQUAL(u"AZ==BAR", s);

    s = u"fooAZfoo==fooBARfoo";
    s.remove(u"NOTTHERE");
    TSUNIT_EQUAL(u"fooAZfoo==fooBARfoo", s);

    s = u"";
    s.remove(u"foo");
    TSUNIT_EQUAL(u"", s);

    s = u"fooAZfoo==fooBARfoo";
    s.remove(u"");
    TSUNIT_EQUAL(u"fooAZfoo==fooBARfoo", s);

    s = u"fooAZfoo==fooBARfoo";
    s.remove(u"o");
    TSUNIT_EQUAL(u"fAZf==fBARf", s);

    s = u"fooAZfoo==fooBARfoo";
    s.remove(u"z");
    TSUNIT_EQUAL(u"fooAZfoo==fooBARfoo", s);

    s = u"az zef cer ";
    TSUNIT_EQUAL(u"azzefcer", s.toRemoved(u" "));

    TSUNIT_EQUAL(u"AZ==BAR", ts::UString(u"fooAZfoo==fooBARfoo").toRemoved(u"foo"));

    s = u"fooAZfoo==fooBARfoo";
    const ts::UString foo2(u"foo");
    TSUNIT_EQUAL(u"AZ==BAR", s.toRemoved(foo2));
    TSUNIT_EQUAL(u"fooAZfoo==fooBARfoo", s.toRemoved(u"NOTTHERE"));

    s = u"";
    TSUNIT_EQUAL(u"", s.toRemoved(u"foo"));

    s = u"fooAZfoo==fooBARfoo";
    TSUNIT_EQUAL(u"fooAZfoo==fooBARfoo", s.toRemoved(u""));
    TSUNIT_EQUAL(u"fAZf==fBARf", s.toRemoved(u"o"));
    TSUNIT_EQUAL(u"fooAZfoo==fooBARfoo", s.toRemoved(u"z"));
    TSUNIT_EQUAL(u"fooAZfoo==fooBARfoo", s.toRemoved('z'));
    TSUNIT_EQUAL(u"fooAZfoo==fooBARfoo", s.toRemoved(ts::UChar('z')));
    TSUNIT_EQUAL(u"fAZf==fBARf", s.toRemoved('o'));
    TSUNIT_EQUAL(u"fAZf==fBARf", s.toRemoved(ts::UChar('o')));
}

TSUNIT_DEFINE_TEST(Substitute)
{
    TSUNIT_EQUAL(u"", ts::UString(u"").toSubstituted(u"", u""));
    TSUNIT_EQUAL(u"xyzcdefxyzcdef", ts::UString(u"abcdefabcdef").toSubstituted(u"ab", u"xyz"));
    TSUNIT_EQUAL(u"abcdxyzabcdxyz", ts::UString(u"abcdefabcdef").toSubstituted(u"ef", u"xyz"));
    TSUNIT_EQUAL(u"abbcdbba", ts::UString(u"abcdba").toSubstituted(u"b", u"bb"));
    TSUNIT_EQUAL(u"abcdabcd", ts::UString(u"abcdefabcdef").toSubstituted(u"ef", u""));

    TSUNIT_EQUAL(u"", ts::UString(u"").toSubstituted(u'a', u'b'));
    TSUNIT_EQUAL(u"bqcrtybdfr", ts::UString(u"aqcrtyadfr").toSubstituted(u'a', u'b'));
    TSUNIT_EQUAL(u"aqcrtyadfr", ts::UString(u"aqcrtyadfr").toSubstituted(u'a', u'a'));
}

TSUNIT_DEFINE_TEST(Split)
{
    ts::UStringVector v1;
    ts::UString(u"az, ,  fr,  ze ,t").split(v1);
    TSUNIT_EQUAL(5, v1.size());
    TSUNIT_EQUAL(u"az", v1[0]);
    TSUNIT_EQUAL(u"", v1[1]);
    TSUNIT_EQUAL(u"fr", v1[2]);
    TSUNIT_EQUAL(u"ze", v1[3]);
    TSUNIT_EQUAL(u"t", v1[4]);

    ts::UStringVector v2;
    const ts::UString s2(u"az, ,  fr,  ze ,t");
    s2.split(v2);
    TSUNIT_EQUAL(5, v2.size());
    TSUNIT_EQUAL(u"az", v2[0]);
    TSUNIT_EQUAL(u"", v2[1]);
    TSUNIT_EQUAL(u"fr", v2[2]);
    TSUNIT_EQUAL(u"ze", v2[3]);
    TSUNIT_EQUAL(u"t", v2[4]);

    ts::UStringVector v3;
    ts::UString(u"az, ,  fr,  ze ,t").split(v3, ts::COMMA, false);
    TSUNIT_EQUAL(5, v3.size());
    TSUNIT_EQUAL(u"az", v3[0]);
    TSUNIT_EQUAL(u" ", v3[1]);
    TSUNIT_EQUAL(u"  fr", v3[2]);
    TSUNIT_EQUAL(u"  ze ", v3[3]);
    TSUNIT_EQUAL(u"t", v3[4]);

    ts::UStringVector v4;
    ts::UString(u"az, ,  fr,  ze ,t").split(v4, ts::UChar('z'), false);
    TSUNIT_EQUAL(3, v4.size());
    TSUNIT_EQUAL(u"a", v4[0]);
    TSUNIT_EQUAL(u", ,  fr,  ", v4[1]);
    TSUNIT_EQUAL(u"e ,t", v4[2]);

    // Make sure that the null character is a valid separator.
    ts::UStringVector v5;
    ts::UString s5(u"abcd");
    s5.append(ts::CHAR_NULL);
    s5.append(u"ef");
    s5.append(ts::CHAR_NULL);
    s5.append(u"ghi");
    s5.split(v5, ts::CHAR_NULL);
    TSUNIT_EQUAL(3, v5.size());
    TSUNIT_EQUAL(u"abcd", v5[0]);
    TSUNIT_EQUAL(u"ef", v5[1]);
    TSUNIT_EQUAL(u"ghi", v5[2]);
}

TSUNIT_DEFINE_TEST(SplitShellStyle)
{
    ts::UStringVector v;
    ts::UString(u" qfdjh qf f'az ef ' df\"nn'\\\"ju\" ").splitShellStyle(v);
    TSUNIT_EQUAL(4, v.size());
    TSUNIT_EQUAL(u"qfdjh", v[0]);
    TSUNIT_EQUAL(u"qf", v[1]);
    TSUNIT_EQUAL(u"faz ef ", v[2]);
    TSUNIT_EQUAL(u"dfnn'\"ju", v[3]);
}

TSUNIT_DEFINE_TEST(Join)
{
    ts::UStringVector v;
    v.push_back(u"az");
    v.push_back(u"sd");
    v.push_back(u"tg");
    TSUNIT_EQUAL(u"az, sd, tg", ts::UString::Join(v));
    TSUNIT_EQUAL(u"sd, tg", ts::UString::Join(++v.begin(), v.end()));
}

TSUNIT_DEFINE_TEST(BreakLines)
{
    ts::UStringVector v;
    ts::UString(u"aze arf erf r+oih zf").splitLines(v, 8);
    TSUNIT_EQUAL(3, v.size());
    TSUNIT_EQUAL(u"aze arf", v[0]);
    TSUNIT_EQUAL(u"erf", v[1]);
    TSUNIT_EQUAL(u"r+oih zf", v[2]);

    v.clear();
    ts::UString(u"aze arf erf r+oih zf").splitLines(v, 8, u"+");
    TSUNIT_EQUAL(3, v.size());
    TSUNIT_EQUAL(u"aze arf", v[0]);
    TSUNIT_EQUAL(u"erf r+", v[1]);
    TSUNIT_EQUAL(u"oih zf", v[2]);

    v.clear();
    ts::UString(u"aze arf erf r+oih zf").splitLines(v, 8, u"", u"==");
    TSUNIT_EQUAL(4, v.size());
    TSUNIT_EQUAL(u"aze arf", v[0]);
    TSUNIT_EQUAL(u"==erf", v[1]);
    TSUNIT_EQUAL(u"==r+oih", v[2]);
    TSUNIT_EQUAL(u"==zf", v[3]);

    v.clear();
    ts::UString(u"aze arf dkvyfngofnb ff").splitLines(v, 8);
    TSUNIT_EQUAL(3, v.size());
    TSUNIT_EQUAL(u"aze arf", v[0]);
    TSUNIT_EQUAL(u"dkvyfngofnb", v[1]);
    TSUNIT_EQUAL(u"ff", v[2]);

    v.clear();
    ts::UString(u"aze arf dkvyfngofnb ff").splitLines(v, 8, u"", u"", true);
    TSUNIT_EQUAL(3, v.size());
    TSUNIT_EQUAL(u"aze arf", v[0]);
    TSUNIT_EQUAL(u"dkvyfngo", v[1]);
    TSUNIT_EQUAL(u"fnb ff", v[2]);

    v.clear();
    ts::UString(u"abc def ghi\nfoo bar tom").splitLines(v, 8);
    TSUNIT_EQUAL(4, v.size());
    TSUNIT_EQUAL(u"abc def", v[0]);
    TSUNIT_EQUAL(u"ghi", v[1]);
    TSUNIT_EQUAL(u"foo bar", v[2]);
    TSUNIT_EQUAL(u"tom", v[3]);

    v.clear();
    ts::UString(u"abc def ghi\n\n\nfoo bar tom").splitLines(v, 8);
    TSUNIT_EQUAL(6, v.size());
    TSUNIT_EQUAL(u"abc def", v[0]);
    TSUNIT_EQUAL(u"ghi", v[1]);
    TSUNIT_EQUAL(u"", v[2]);
    TSUNIT_EQUAL(u"", v[3]);
    TSUNIT_EQUAL(u"foo bar", v[4]);
    TSUNIT_EQUAL(u"tom", v[5]);

    v.clear();
    ts::UString(u"abc def ghi\nfoo bar tom").splitLines(v, 10, ts::UString(), u"  ");
    TSUNIT_EQUAL(4, v.size());
    TSUNIT_EQUAL(u"abc def", v[0]);
    TSUNIT_EQUAL(u"  ghi", v[1]);
    TSUNIT_EQUAL(u"  foo bar", v[2]);
    TSUNIT_EQUAL(u"  tom", v[3]);

    v.clear();
    ts::UString(u"abc def ghi\nfoo bar tom").splitLines(v, 80, ts::UString(), u"  ");
    TSUNIT_EQUAL(2, v.size());
    TSUNIT_EQUAL(u"abc def ghi", v[0]);
    TSUNIT_EQUAL(u"  foo bar tom", v[1]);

    v.clear();
    ts::UString(u"  --debug").splitLines(v, 80, ts::UString(), u"  ");
    TSUNIT_EQUAL(1, v.size());
    TSUNIT_EQUAL(u"  --debug", v[0]);
}

TSUNIT_DEFINE_TEST(RemovePrefix)
{
    ts::UString s;

    s = u"abcdef";
    s.removePrefix(u"ab");
    TSUNIT_EQUAL(u"cdef", s);

    s = u"abcdef";
    s.removePrefix(u"xy");
    TSUNIT_EQUAL(u"abcdef", s);

    s = u"abcdef";
    s.removePrefix(u"");
    TSUNIT_EQUAL(u"abcdef", s);

    s = u"";
    s.removePrefix(u"ab");
    TSUNIT_EQUAL(u"", s);

    TSUNIT_EQUAL(u"cdef", ts::UString(u"abcdef").toRemovedPrefix(u"ab"));
    TSUNIT_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedPrefix(u"xy"));
    TSUNIT_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedPrefix(u""));
    TSUNIT_EQUAL(u"", ts::UString(u"").toRemovedPrefix(u"ab"));

    TSUNIT_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedPrefix(u"AB"));
    TSUNIT_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedPrefix(u"AB", ts::CASE_SENSITIVE));
    TSUNIT_EQUAL(u"cdef", ts::UString(u"abcdef").toRemovedPrefix(u"AB", ts::CASE_INSENSITIVE));
}

TSUNIT_DEFINE_TEST(RemoveSuffix)
{
    ts::UString s;

    s = u"abcdef";
    s.removeSuffix(u"ef");
    TSUNIT_EQUAL(u"abcd", s);

    s = u"abcdef";
    s.removeSuffix(u"xy");
    TSUNIT_EQUAL(u"abcdef", s);

    s = u"abcdef";
    s.removeSuffix(u"");
    TSUNIT_EQUAL(u"abcdef", s);

    s = u"";
    s.removeSuffix(u"ef");
    TSUNIT_EQUAL(u"", s);

    TSUNIT_EQUAL(u"abcd", ts::UString(u"abcdef").toRemovedSuffix(u"ef"));
    TSUNIT_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedSuffix(u"xy"));
    TSUNIT_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedSuffix(u""));
    TSUNIT_EQUAL(u"", ts::UString(u"").toRemovedSuffix(u"ef"));

    TSUNIT_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedSuffix(u"EF"));
    TSUNIT_EQUAL(u"abcdef", ts::UString(u"abcdef").toRemovedSuffix(u"EF", ts::CASE_SENSITIVE));
    TSUNIT_EQUAL(u"abcd", ts::UString(u"abcdef").toRemovedSuffix(u"EF", ts::CASE_INSENSITIVE));
}

TSUNIT_DEFINE_TEST(Start)
{
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(u"azer"));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").starts_with(u"aZer"));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").starts_with(u"azeR"));

    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(u"azer", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(u"aZer", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(u"azeR", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").starts_with(u"azerq", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(u"rtyu", ts::CASE_INSENSITIVE, false, 3));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").starts_with(u"azer", ts::CASE_INSENSITIVE, false, 3));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").starts_with(u"azer", ts::CASE_INSENSITIVE, false, 2500));

    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(u""));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").starts_with(u"azertyuiopqsdf"));

    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(u"", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").starts_with(u"azertyuiopqsdf", ts::CASE_INSENSITIVE));

    TSUNIT_ASSERT(ts::UString(u"").starts_with(u""));
    TSUNIT_ASSERT(!ts::UString(u"").starts_with(u"abcd"));

    TSUNIT_ASSERT(ts::UString(u"").starts_with(u"", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(!ts::UString(u"").starts_with(u"abcd", ts::CASE_INSENSITIVE));

    TSUNIT_ASSERT(!ts::UString(u"  azertyuiop").starts_with(u"az", ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"  azertyuiop").starts_with(u"az", ts::CASE_SENSITIVE, true));

    const ts::UChar a = u'a';
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(a));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(a, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"AZERTYIUOP").starts_with(a, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"AZERTYIUOP").starts_with(a, ts::CASE_INSENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"  azertyuiop").starts_with(a, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"  azertyuiop").starts_with(a, ts::CASE_SENSITIVE, true));

    const ts::UChar q = u'q';
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").starts_with(q));
    TSUNIT_ASSERT(!ts::UString(u"AZERTYIUOP").starts_with(q, ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(!ts::UString(u"AZERTYIUOP").starts_with(a, ts::CASE_SENSITIVE));

    const ts::UChar azer[] = u"azer";
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(azer));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(azer, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"AZERTYIUOP").starts_with(azer, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"AZERTYIUOP").starts_with(azer, ts::CASE_INSENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"  azertyuiop").starts_with(azer, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"  azertyuiop").starts_with(azer, ts::CASE_SENSITIVE, true));
    TSUNIT_ASSERT(ts::UString(u"  AZERTYIUOP").starts_with(azer, ts::CASE_INSENSITIVE, true));

    std::u16string_view azert = u"azert";
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(azert));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").starts_with(azert, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"AZERTYIUOP").starts_with(azert, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"AZERTYIUOP").starts_with(azert, ts::CASE_INSENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"  azertyuiop").starts_with(azert, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"  azertyuiop").starts_with(azert, ts::CASE_SENSITIVE, true));
    TSUNIT_ASSERT(ts::UString(u"  AZERTYIUOP").starts_with(azert, ts::CASE_INSENSITIVE, true));
}

TSUNIT_DEFINE_TEST(End)
{
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(u"uiop"));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").ends_with(u"uiOp"));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").ends_with(u"Uiop"));

    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(u"uiop", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(u"uiOp", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(u"Uiop", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").ends_with(u"wuiop", ts::CASE_INSENSITIVE));

    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(u""));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").ends_with(u"qsazertyuiop"));

    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(u"", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").ends_with(u"qsazertyuiop", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(u"erty", ts::CASE_INSENSITIVE, false, 6));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").ends_with(u"uiop", ts::CASE_INSENSITIVE, false, 6));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(u"uiop", ts::CASE_INSENSITIVE, false, 2500));

    TSUNIT_ASSERT(ts::UString(u"").ends_with(u""));
    TSUNIT_ASSERT(!ts::UString(u"").ends_with(u"abcd"));

    TSUNIT_ASSERT(ts::UString(u"").ends_with(u"", ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(!ts::UString(u"").ends_with(u"abcd", ts::CASE_INSENSITIVE));

    TSUNIT_ASSERT(!ts::UString(u"azertyuiop  ").ends_with(u"uiop", ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop  ").ends_with(u"uiop", ts::CASE_SENSITIVE, true));

    ts::UChar p = u'p';
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(p));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(p, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"AZERTYIUOP").ends_with(p, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"AZERTYIUOP").ends_with(p, ts::CASE_INSENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop  ").ends_with(p, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"  azertyuiop  ").ends_with(p, ts::CASE_SENSITIVE, true));

    const ts::UChar q = u'q';
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop").ends_with(q));
    TSUNIT_ASSERT(!ts::UString(u"AZERTYIUOP").ends_with(q, ts::CASE_INSENSITIVE));
    TSUNIT_ASSERT(!ts::UString(u"AZERTYIUOP").ends_with(p, ts::CASE_SENSITIVE));

    const ts::UChar uiop[] = u"uiop";
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(uiop));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(uiop, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"AZERTYUIOP").ends_with(uiop, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"AZERTYUIOP").ends_with(uiop, ts::CASE_INSENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop   ").ends_with(uiop, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop   ").ends_with(uiop, ts::CASE_SENSITIVE, true));
    TSUNIT_ASSERT(ts::UString(u"AZERTYUIOP   ").ends_with(uiop, ts::CASE_INSENSITIVE, true));

    std::u16string_view iop = u"iop";
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(iop));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop").ends_with(iop, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"AZERTYUIOP").ends_with(iop, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"AZERTUIOP").ends_with(iop, ts::CASE_INSENSITIVE, false));
    TSUNIT_ASSERT(!ts::UString(u"azertyuiop      ").ends_with(iop, ts::CASE_SENSITIVE, false));
    TSUNIT_ASSERT(ts::UString(u"azertyuiop    ").ends_with(iop, ts::CASE_SENSITIVE, true));
    TSUNIT_ASSERT(ts::UString(u"AZERTYUIOP    ").ends_with(iop, ts::CASE_INSENSITIVE, true));
}

TSUNIT_DEFINE_TEST(JustifyLeft)
{
    TSUNIT_EQUAL(u"abc     ", ts::UString(u"abc").toJustifiedLeft(8));
    TSUNIT_EQUAL(u"abc.....", ts::UString(u"abc").toJustifiedLeft(8, '.'));
    TSUNIT_EQUAL(u"abcdefghij", ts::UString(u"abcdefghij").toJustifiedLeft(8));
    TSUNIT_EQUAL(u"abcdefgh", ts::UString(u"abcdefghij").toJustifiedLeft(8, ' ', true));
}

TSUNIT_DEFINE_TEST(JustifyRight)
{
    TSUNIT_EQUAL(u"     abc", ts::UString(u"abc").toJustifiedRight(8));
    TSUNIT_EQUAL(u".....abc", ts::UString(u"abc").toJustifiedRight(8, '.'));
    TSUNIT_EQUAL(u"abcdefghij", ts::UString(u"abcdefghij").toJustifiedRight(8));
    TSUNIT_EQUAL(u"cdefghij", ts::UString(u"abcdefghij").toJustifiedRight(8, ' ', true));
}

TSUNIT_DEFINE_TEST(JustifyCentered)
{
    TSUNIT_EQUAL(u"  abc   ", ts::UString(u"abc").toJustifiedCentered(8));
    TSUNIT_EQUAL(u"..abc...", ts::UString(u"abc").toJustifiedCentered(8, '.'));
    TSUNIT_EQUAL(u"abcdefghij", ts::UString(u"abcdefghij").toJustifiedCentered(8));
    TSUNIT_EQUAL(u"abcdefgh", ts::UString(u"abcdefghij").toJustifiedCentered(8, ' ', true));
}

TSUNIT_DEFINE_TEST(Justify)
{
    TSUNIT_EQUAL(u"abc  def", ts::UString(u"abc").toJustified(u"def", 8));
    TSUNIT_EQUAL(u"abc..def", ts::UString(u"abc").toJustified(u"def", 8, '.'));
    TSUNIT_EQUAL(u"abcdefgh", ts::UString(u"abcd").toJustified(u"efgh", 8));
    TSUNIT_EQUAL(u"abcdefghij", ts::UString(u"abcde").toJustified(u"fghij", 8));
}

TSUNIT_DEFINE_TEST(YesNo)
{
    TSUNIT_EQUAL(u"yes", ts::UString::YesNo(true));
    TSUNIT_EQUAL(u"no", ts::UString::YesNo(false));
}

TSUNIT_DEFINE_TEST(TrueFalse)
{
    TSUNIT_EQUAL(u"true", ts::UString::TrueFalse(true));
    TSUNIT_EQUAL(u"false", ts::UString::TrueFalse(false));
}

TSUNIT_DEFINE_TEST(OnOff)
{
    TSUNIT_EQUAL(u"on", ts::UString::OnOff(true));
    TSUNIT_EQUAL(u"off", ts::UString::OnOff(false));
}

TSUNIT_DEFINE_TEST(SimilarStrings)
{
    TSUNIT_ASSERT(ts::UString(u"").similar(u""));
    TSUNIT_ASSERT(ts::UString(u"aZer tY").similar(u"  AZE R T Y    "));
    TSUNIT_ASSERT(ts::UString(u"  AZE R T Y    ").similar(u"aZer tY"));
    TSUNIT_ASSERT(!ts::UString(u"").similar(u"az"));
    TSUNIT_ASSERT(!ts::UString(u"az").similar(u""));
}

TSUNIT_DEFINE_TEST(LoadSave)
{
    ts::UStringList ref;

    ts::UChar c = ts::LATIN_CAPITAL_LETTER_A_WITH_MACRON;
    for (int i = 1; i <= 20; ++i) {
        ref.push_back(ts::UString::Format(u"%s, line %d", ts::UString(2, c++), i));
    }
    TSUNIT_ASSERT(ref.size() == 20);

    const ts::UString file1(newTemporaryFileName());
    TSUNIT_ASSERT(ts::UString::Save(ref, file1));

    ts::UStringList load1;
    TSUNIT_ASSERT(ts::UString::Load(load1, file1));
    TSUNIT_EQUAL(20, load1.size());
    TSUNIT_ASSERT(load1 == ref);

    const auto refFirst = ++(ref.begin());
    const auto refLast = --(ref.end());

    const ts::UString file2(newTemporaryFileName());
    TSUNIT_ASSERT(ts::UString::Save(refFirst, refLast, file2));

    ts::UStringList ref2(refFirst, refLast);
    TSUNIT_EQUAL(18, ref2.size());

    ts::UStringList load2;
    TSUNIT_ASSERT(ts::UString::Load(load2, file2));
    TSUNIT_EQUAL(18, load2.size());
    TSUNIT_ASSERT(load2 == ref2);

    ts::UStringList ref3;
    ref3.push_back(u"abcdef");
    ref3.insert(ref3.end(), refFirst, refLast);
    TSUNIT_EQUAL(19, ref3.size());

    ts::UStringList load3;
    load3.push_back(u"abcdef");
    TSUNIT_ASSERT(ts::UString::LoadAppend(load3, file2));
    TSUNIT_EQUAL(19, load3.size());
    TSUNIT_ASSERT(load3 == ref3);
}

TSUNIT_DEFINE_TEST(ToDigit)
{
    TSUNIT_EQUAL(0,  ts::ToDigit(ts::UChar('0')));
    TSUNIT_EQUAL(9,  ts::ToDigit(ts::UChar('9')));
    TSUNIT_EQUAL(-1, ts::ToDigit(ts::UChar('a')));
    TSUNIT_EQUAL(-1, ts::ToDigit(ts::UChar('f')));
    TSUNIT_EQUAL(-1, ts::ToDigit(ts::UChar('z')));
    TSUNIT_EQUAL(10, ts::ToDigit(ts::UChar('a'), 16));
    TSUNIT_EQUAL(15, ts::ToDigit(ts::UChar('f'), 16));
    TSUNIT_EQUAL(-1, ts::ToDigit(ts::UChar('z'), 16));
    TSUNIT_EQUAL(10, ts::ToDigit(ts::UChar('a'), 36));
    TSUNIT_EQUAL(15, ts::ToDigit(ts::UChar('f'), 36));
    TSUNIT_EQUAL(35, ts::ToDigit(ts::UChar('z'), 36));
    TSUNIT_EQUAL(10, ts::ToDigit(ts::UChar('A'), 16));
    TSUNIT_EQUAL(15, ts::ToDigit(ts::UChar('F'), 16));
    TSUNIT_EQUAL(-1, ts::ToDigit(ts::UChar('Z'), 16));
    TSUNIT_EQUAL(10, ts::ToDigit(ts::UChar('A'), 36));
    TSUNIT_EQUAL(15, ts::ToDigit(ts::UChar('F'), 36));
    TSUNIT_EQUAL(35, ts::ToDigit(ts::UChar('Z'), 36));
    TSUNIT_EQUAL(-1, ts::ToDigit(ts::UChar('?')));
    TSUNIT_EQUAL(-2, ts::ToDigit(ts::UChar('?'), 10, -2));
}

TSUNIT_DEFINE_TEST(ToInteger)
{
    int      i;
    uint32_t ui32;
    uint64_t ui64;
    int64_t  i64;
    int16_t  i16;

    TSUNIT_ASSERT(ts::UString(u"1").toInteger(i));
    TSUNIT_EQUAL(1, i);

    TSUNIT_ASSERT(ts::UString(u"-001").toInteger(i));
    TSUNIT_EQUAL(-1, i);

    TSUNIT_ASSERT(ts::UString(u"   -0xA0  ").toInteger(i));
    TSUNIT_EQUAL(-160, i);

    TSUNIT_ASSERT(!ts::UString(u"").toInteger(i));
    TSUNIT_EQUAL(0, i);

    TSUNIT_ASSERT(ts::UString(u"123").toInteger(ui32));
    TSUNIT_EQUAL(123, ui32);

    TSUNIT_ASSERT(!ts::UString(u"-123").toInteger(ui32));
    TSUNIT_EQUAL(0, ui32);

    TSUNIT_ASSERT(ts::UString(u"0").toInteger(ui64));
    TSUNIT_EQUAL(0, ui64);

    TSUNIT_ASSERT(ts::UString(u"0xffffffffFFFFFFFF").toInteger(ui64));
    TSUNIT_EQUAL(0XFFFFFFFFFFFFFFFF, ui64);

    TSUNIT_ASSERT(ts::UString(u"0x7fffffffFFFFFFFF").toInteger(ui64));
    TSUNIT_EQUAL(0X7FFFFFFFFFFFFFFF, ui64);

    TSUNIT_ASSERT(ts::UString(u"0").toInteger(i64));
    TSUNIT_EQUAL(0, i64);

    TSUNIT_ASSERT(ts::UString(u"0x7fffffffFFFFFFFF").toInteger(i64));
    TSUNIT_EQUAL(0X7FFFFFFFFFFFFFFF, i64);

    TSUNIT_ASSERT(ts::UString(u" 12,345").toInteger(i, u",."));
    TSUNIT_EQUAL(12345, i);

    TSUNIT_ASSERT(ts::UString(u" -12#345").toInteger(i, u",#"));
    TSUNIT_EQUAL(-12345, i);

    TSUNIT_ASSERT(!ts::UString(u" -12;345").toInteger(i, u",#"));
    TSUNIT_EQUAL(-12, i);

    TSUNIT_ASSERT(ts::UString(u" -12.345").toInteger(i, u",", 3));
    TSUNIT_EQUAL(-12345, i);

    TSUNIT_ASSERT(ts::UString(u" 1  ").toInteger(ui32, u",", 3));
    TSUNIT_EQUAL(1000, ui32);

    TSUNIT_ASSERT(ts::UString(u"1.").toInteger(ui32, u",", 2));
    TSUNIT_EQUAL(100, ui32);

    TSUNIT_ASSERT(ts::UString(u" +1.23").toInteger(ui32, u",", 6));
    TSUNIT_EQUAL(1230000, ui32);

    TSUNIT_ASSERT(ts::UString(u"2.345678").toInteger(i, u",", 3));
    TSUNIT_EQUAL(2345, i);

    TSUNIT_ASSERT(!ts::UString(u"2.345678").toInteger(i));
    TSUNIT_EQUAL(2, i);

    TSUNIT_ASSERT(ts::UString(u" -2.345678").toInteger(i, u",", 4));
    TSUNIT_EQUAL(-23456, i);

    TSUNIT_ASSERT(!ts::UString(u" -2.345678").toInteger(i));
    TSUNIT_EQUAL(-2, i);

    TSUNIT_ASSERT(ts::UString(u"-32768").toInteger(i));
    TSUNIT_EQUAL(-32768, i);

    TSUNIT_ASSERT(ts::UString(u"-32768").toInteger(i16));
    TSUNIT_EQUAL(-32768, i16);

    std::list<int32_t> i32List;
    std::list<int32_t> i32Ref;

    i32Ref.clear();
    i32Ref.push_back(-12345);
    i32Ref.push_back(256);
    i32Ref.push_back(0);
    i32Ref.push_back(7);

    TSUNIT_ASSERT(ts::UString(u"-12345 0x100 0 7").toIntegers(i32List));
    TSUNIT_ASSERT(i32Ref == i32List);

    TSUNIT_ASSERT(ts::UString(u" , -12345    0x100 ,  0,  7  ").toIntegers(i32List));
    TSUNIT_ASSERT(i32Ref == i32List);

    TSUNIT_ASSERT(!ts::UString(u" , -12345    0x100 ,  0,  7  xxx 45").toIntegers(i32List));
    TSUNIT_ASSERT(i32Ref == i32List);
}

TSUNIT_DEFINE_TEST(ToTristate)
{
    ts::Tristate t;

    t = ts::Tristate::Maybe;
    TSUNIT_ASSERT(ts::UString(u"yes").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::True, t);

    t = ts::Tristate::Maybe;
    TSUNIT_ASSERT(ts::UString(u"True").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::True, t);

    t = ts::Tristate::Maybe;
    TSUNIT_ASSERT(ts::UString(u"ON").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::True, t);

    t = ts::Tristate::Maybe;
    TSUNIT_ASSERT(ts::UString(u"NO").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::False, t);

    t = ts::Tristate::Maybe;
    TSUNIT_ASSERT(ts::UString(u"FaLsE").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::False, t);

    t = ts::Tristate::Maybe;
    TSUNIT_ASSERT(ts::UString(u"off").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::False, t);

    t = ts::Tristate::True;
    TSUNIT_ASSERT(ts::UString(u"MayBe").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::Maybe, t);

    t = ts::Tristate::True;
    TSUNIT_ASSERT(ts::UString(u"Unknown").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::Maybe, t);

    t = ts::Tristate::True;
    TSUNIT_ASSERT(ts::UString(u"0x0000").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::False, t);

    t = ts::Tristate::Maybe;
    TSUNIT_ASSERT(ts::UString(u"1").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::True, t);

    t = ts::Tristate::Maybe;
    TSUNIT_ASSERT(ts::UString(u"56469").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::True, t);

    t = ts::Tristate::True;
    TSUNIT_ASSERT(ts::UString(u"-1").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::Maybe, t);

    t = ts::Tristate::True;
    TSUNIT_ASSERT(ts::UString(u"-56").toTristate(t));
    TSUNIT_EQUAL(ts::Tristate::Maybe, t);

    TSUNIT_ASSERT(!ts::UString(u"abcd").toTristate(t));
    TSUNIT_ASSERT(!ts::UString(u"0df").toTristate(t));
    TSUNIT_ASSERT(!ts::UString(u"").toTristate(t));
    TSUNIT_ASSERT(!ts::UString(u" ").toTristate(t));
}

TSUNIT_DEFINE_TEST(HexaDecode)
{
    ts::ByteBlock bytes;

    TSUNIT_ASSERT(ts::UString(u"0123456789ABCDEF").hexaDecode(bytes));
    TSUNIT_ASSERT(bytes == ts::ByteBlock({0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}));

    TSUNIT_ASSERT(ts::UString(u" 0 1234 56 789 ABC DEF ").hexaDecode(bytes));
    TSUNIT_ASSERT(bytes == ts::ByteBlock({0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}));

    TSUNIT_ASSERT(!ts::UString(u" 0 1234 56 - 789 ABC DEF ").hexaDecode(bytes));
    TSUNIT_ASSERT(bytes == ts::ByteBlock({0x01, 0x23, 0x45}));

    TSUNIT_ASSERT(!ts::UString(u"X 0 1234 56 - 789 ABC DEF ").hexaDecode(bytes));
    TSUNIT_ASSERT(bytes.empty());

    TSUNIT_ASSERT(!ts::UString(u"01 23 {0x45, 0x67};").hexaDecode(bytes));
    TSUNIT_ASSERT(bytes == ts::ByteBlock({0x01, 0x23}));

    TSUNIT_ASSERT(ts::UString(u"01 23 {0x45, 0x67};").hexaDecode(bytes, true));
    TSUNIT_ASSERT(bytes == ts::ByteBlock({0x01, 0x23, 0x45, 0x67}));
}

TSUNIT_DEFINE_TEST(AppendContainer)
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

    TSUNIT_ASSERT(ts::UString::Append(var, 4, arr1) == ref);

    const char* arr2[] = {"ab", "cde", "", "fghi"};

    var.clear();
    var.push_back(u"begin");
    TSUNIT_ASSERT(ts::UString::Append(var, 4, arr2) == ref);
}

TSUNIT_DEFINE_TEST(AssignContainer)
{
    const char* arr1[] = {"ab", "cde", "", "fghi"};
    ts::UStringList var;
    ts::UStringList ref;

    var.push_back(u"previous");

    ref.push_back(u"ab");
    ref.push_back(u"cde");
    ref.push_back(u"");
    ref.push_back(u"fghi");

    TSUNIT_ASSERT(ts::UString::Assign(var, 4, arr1) == ref);

    const char* arr2[] = {"ab", "cde", "", "fghi"};

    var.clear();
    var.push_back(u"other");
    TSUNIT_ASSERT(ts::UString::Assign(var, 4, arr2) == ref);
}

TSUNIT_DEFINE_TEST(Decimal)
{
    TSUNIT_EQUAL(u"0", ts::UString::Decimal(0));
    TSUNIT_EQUAL(u"0", ts::UString::Decimal(0L));
    TSUNIT_EQUAL(u"0", ts::UString::Decimal(-0));
    TSUNIT_EQUAL(u"0", ts::UString::Decimal(0));
    TSUNIT_EQUAL(u"1,234", ts::UString::Decimal(1234));
    TSUNIT_EQUAL(u"     1,234", ts::UString::Decimal(1234, 10));
    TSUNIT_EQUAL(u"     1,234", ts::UString::Decimal(1234, 10, true));
    TSUNIT_EQUAL(u"1,234     ", ts::UString::Decimal(1234, 10, false));
    TSUNIT_EQUAL(u"      1234", ts::UString::Decimal(1234, 10, true, u""));
    TSUNIT_EQUAL(u"  1()234()567()890", ts::UString::Decimal(1234567890, 18, true, u"()"));
    TSUNIT_EQUAL(u"    +1,234", ts::UString::Decimal(1234, 10, true, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, true));
    TSUNIT_EQUAL(u"    -1,234", ts::UString::Decimal(-1234, 10, true, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, true));
    TSUNIT_EQUAL(u"    -1,234", ts::UString::Decimal(-1234, 10, true, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, false));
    TSUNIT_EQUAL(u"-1,234,567,890,123,456", ts::UString::Decimal(-1234567890123456));
    TSUNIT_EQUAL(u"-32,768", ts::UString::Decimal(-32768));
    TSUNIT_EQUAL(u"-32,768", ts::UString::Decimal(int16_t(-32768)));
    TSUNIT_EQUAL(u"-9223372036854775808", ts::UString::Decimal(std::numeric_limits<int64_t>::min(), 0, true, u""));
    TSUNIT_EQUAL(u"-9,223,372,036,854,775,808", ts::UString::Decimal(std::numeric_limits<int64_t>::min()));
    TSUNIT_EQUAL(u"9,223,372,036,854,775,807", ts::UString::Decimal(std::numeric_limits<int64_t>::max()));
}

TSUNIT_DEFINE_TEST(DecimalList)
{
    TSUNIT_EQUAL(u"", ts::UString::Decimal(std::vector<int>{}));
    TSUNIT_EQUAL(u"56", ts::UString::Decimal(std::vector<int>{56}));
    TSUNIT_EQUAL(u"1, -2, 3, -1234", ts::UString::Decimal(std::vector<int>{1, -2, 3, -1234}));
    TSUNIT_EQUAL(u"1/-2/3/-1234", ts::UString::Decimal(std::vector<int>{1, -2, 3, -1234}, u"/"));
    TSUNIT_EQUAL(u"+1/-2/+3/-1234", ts::UString::Decimal(std::vector<int>{1, -2, 3, -1234}, u"/", true));

    TSUNIT_EQUAL(u"", ts::UString::Decimal(std::list<int>{}));
    TSUNIT_EQUAL(u"56", ts::UString::Decimal(std::list<int>{56}));
    TSUNIT_EQUAL(u"1, -2, 3, -1234", ts::UString::Decimal(std::list<int>{1, -2, 3, -1234}));
}

TSUNIT_DEFINE_TEST(Hexa)
{
    TSUNIT_EQUAL(u"0x00", ts::UString::Hexa<uint8_t>(0));
    TSUNIT_EQUAL(u"0x00000123", ts::UString::Hexa<uint32_t>(0x123));
    TSUNIT_EQUAL(u"0x0000000000000123", ts::UString::Hexa(0x123LL));
    TSUNIT_EQUAL(u"0xFFFFFFFFFFFFFFFD", ts::UString::Hexa(-3LL));
    TSUNIT_EQUAL(u"0xfffffffffffffffd", ts::UString::Hexa(-3LL, 0, ts::UString(), true, false));
    TSUNIT_EQUAL(u"0x002", ts::UString::Hexa<uint16_t>(0x02, 3));
    TSUNIT_EQUAL(u"0x000002", ts::UString::Hexa<uint16_t>(0x02, 6));
    TSUNIT_EQUAL(u"0x0000<>0123", ts::UString::Hexa<uint32_t>(0x123, 0, u"<>"));
    TSUNIT_EQUAL(u"0000,0123", ts::UString::Hexa<uint32_t>(0x123, 0, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, false));

    TSUNIT_EQUAL(u"0x00", ts::UString::HexaMin<uint8_t>(0, 0));
    TSUNIT_EQUAL(u"0x0", ts::UString::HexaMin<uint8_t>(0, 1));
    TSUNIT_EQUAL(u"0x0", ts::UString::HexaMin<uint8_t>(0, 2));
    TSUNIT_EQUAL(u"0x0", ts::UString::HexaMin<uint8_t>(0, 3));
    TSUNIT_EQUAL(u"0x00", ts::UString::HexaMin<uint8_t>(0, 4));
    TSUNIT_EQUAL(u"0x000", ts::UString::HexaMin<uint8_t>(0, 5));

    TSUNIT_EQUAL(u"0000,0123", ts::UString::HexaMin<uint32_t>(0x123, 0, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, false));
    TSUNIT_EQUAL(u"123", ts::UString::HexaMin<uint32_t>(0x123, 1, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, false));
    TSUNIT_EQUAL(u"000,0123", ts::UString::HexaMin<uint32_t>(0x123, 8, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, false));
    TSUNIT_EQUAL(u"0x0,0123", ts::UString::HexaMin<uint32_t>(0x123, 8, ts::UString::DEFAULT_THOUSANDS_SEPARATOR, true));
}

TSUNIT_DEFINE_TEST(HexaDump)
{
    // Reference byte array: 256 bytes, index == value
    static const uint8_t refBytes[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
        0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
        0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
        0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
        0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    };

    const ts::UString hex1(ts::UString::Dump(refBytes, 40));
    const ts::UChar* ref1 =
        u"00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19\n"
        u"1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27\n";
    TSUNIT_EQUAL(ref1, hex1);

    const ts::UString hex2(ts::UString::Dump(refBytes, 40, ts::UString::HEXA | ts::UString::ASCII));
    const ts::UChar* ref2 =
        u"00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11  ..................\n"
        u"12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23  .............. !\"#\n"
        u"24 25 26 27                                            $%&'\n";
    TSUNIT_EQUAL(ref2, hex2);

    const ts::UString hex3(ts::UString::Dump(refBytes + 32,
                                             40,
                                             ts::UString::HEXA | ts::UString::ASCII | ts::UString::OFFSET,
                                             4,     // indent
                                             50,    // line_width
                                             32));  // init_offset
    const ts::UChar* ref3 =
        u"    0020:  20 21 22 23 24 25 26 27   !\"#$%&'\n"
        u"    0028:  28 29 2A 2B 2C 2D 2E 2F  ()*+,-./\n"
        u"    0030:  30 31 32 33 34 35 36 37  01234567\n"
        u"    0038:  38 39 3A 3B 3C 3D 3E 3F  89:;<=>?\n"
        u"    0040:  40 41 42 43 44 45 46 47  @ABCDEFG\n";
    TSUNIT_EQUAL(ref3, hex3);

    const ts::UString hex4(ts::UString::Dump(refBytes + 32,
                                             22,
                                             ts::UString::HEXA | ts::UString::ASCII | ts::UString::OFFSET | ts::UString::BPL,
                                             4,     // indent
                                             10,    // lineWidth (in bytes)
                                             32));  // initOffset
    const ts::UChar* ref4 =
        u"    0020:  20 21 22 23 24 25 26 27 28 29   !\"#$%&'()\n"
        u"    002A:  2A 2B 2C 2D 2E 2F 30 31 32 33  *+,-./0123\n"
        u"    0034:  34 35                          45\n";
    TSUNIT_EQUAL(ref4, hex4);

    const ts::UString hex5(ts::UString::Dump(refBytes + 32, 12, ts::UString::SINGLE_LINE));
    const ts::UChar* ref5 = u"20 21 22 23 24 25 26 27 28 29 2A 2B";
    TSUNIT_EQUAL(ref5, hex5);

    const ts::UString hex6(ts::UString::Dump(refBytes + 32, 20, ts::UString::HEXA | ts::UString::C_STYLE));
    const ts::UChar* ref6 =
        u"0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,\n"
        u"0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33,\n";
    TSUNIT_EQUAL(ref6, hex6);

    const ts::UString hex7(ts::UString::Dump(refBytes + 32, 10, ts::UString::BINARY | ts::UString::ASCII));
    const ts::UChar* ref7 =
        u"00100000 00100001 00100010 00100011 00100100 00100101   !\"#$%\n"
        u"00100110 00100111 00101000 00101001                    &'()\n";
    TSUNIT_EQUAL(ref7, hex7);

    const ts::UString hex8(ts::UString::Dump(refBytes + 32, 10, ts::UString::BIN_NIBBLE | ts::UString::ASCII));
    const ts::UChar* ref8 =
        u"0010.0000 0010.0001 0010.0010 0010.0011 0010.0100 0010.0101   !\"#$%\n"
        u"0010.0110 0010.0111 0010.1000 0010.1001                      &'()\n";
    TSUNIT_EQUAL(ref8, hex8);
}

TSUNIT_DEFINE_TEST(ArgMixIn)
{
    testArgMixInCalled1({});

    const std::string ok("ok");
    const ts::UString us(u"an UString");
    const uint8_t u8 = 23;
    const int16_t i16 = -432;
    const size_t sz = 8;

    enum : uint16_t {EA = 7, EB = 48};
    enum : int8_t {EC = 4, ED = 8};
    const ts::IPSocketAddress sock(10, 20, 30, 40, 12345);

    testArgMixInCalled2({12, u8, i16, -99ll, "foo", ok, u"bar", us, ok + " 2", us + u" 2", sz, EB, EC, sock, fs::path(ts::UString(u"foo.bar")), cn::milliseconds(-27), cn::days(12)});
}

void UStringTest::testArgMixInCalled1(std::initializer_list<ts::ArgMixIn> list)
{
    TSUNIT_EQUAL(0, list.size());
}

void UStringTest::testArgMixInCalled2(std::initializer_list<ts::ArgMixIn> list)
{
    TSUNIT_EQUAL(17, list.size());

    auto it = list.begin();

    // 12
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(it->isInteger());
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(!it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(4, it->size());
    TSUNIT_EQUAL(12, it->toInt32());
    TSUNIT_EQUAL(12, it->toUInt32());
    TSUNIT_EQUAL(12, it->toInt64());
    TSUNIT_EQUAL(12, it->toUInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // u8 = 23
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(it->isUnsigned());
    TSUNIT_ASSERT(!it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(1, it->size());
    TSUNIT_EQUAL(23, it->toInt32());
    TSUNIT_EQUAL(23, it->toUInt32());
    TSUNIT_EQUAL(23, it->toInt64());
    TSUNIT_EQUAL(23, it->toUInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // i16 = -432
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(it->isInteger());
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(!it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(2, it->size());
    TSUNIT_EQUAL(-432, it->toInt32());
    TSUNIT_EQUAL(-432, it->toInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // -99ll
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(it->isInteger());
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(!it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(8, it->size());
    TSUNIT_EQUAL(-99, it->toInt32());
    TSUNIT_EQUAL(-99, it->toInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // "foo"
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(!it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(it->isAnyString());
    TSUNIT_ASSERT(it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(0, it->size());
    TSUNIT_EQUAL(0, it->toInt32());
    TSUNIT_EQUAL(0, it->toUInt32());
    TSUNIT_EQUAL(0, it->toInt64());
    TSUNIT_EQUAL(0, it->toUInt64());
    TSUNIT_EQUAL("foo", it->toCharPtr());
    TSUNIT_EQUAL(u"foo", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"foo", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // ok = "ok"
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(!it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(it->isAnyString());
    TSUNIT_ASSERT(it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(0, it->size());
    TSUNIT_EQUAL(0, it->toInt32());
    TSUNIT_EQUAL(0, it->toUInt32());
    TSUNIT_EQUAL(0, it->toInt64());
    TSUNIT_EQUAL(0, it->toUInt64());
    TSUNIT_EQUAL("ok", it->toCharPtr());
    TSUNIT_EQUAL(u"ok", it->toUCharPtr());
    TSUNIT_EQUAL("ok", it->toString());
    TSUNIT_EQUAL(u"ok", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // u"bar"
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(!it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(0, it->size());
    TSUNIT_EQUAL(0, it->toInt32());
    TSUNIT_EQUAL(0, it->toUInt32());
    TSUNIT_EQUAL(0, it->toInt64());
    TSUNIT_EQUAL(0, it->toUInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"bar", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"bar", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // us = u"an UString"
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(!it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(0, it->size());
    TSUNIT_EQUAL(0, it->toInt32());
    TSUNIT_EQUAL(0, it->toUInt32());
    TSUNIT_EQUAL(0, it->toInt64());
    TSUNIT_EQUAL(0, it->toUInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"an UString", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"an UString", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // ok + " 2"
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(!it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(it->isAnyString());
    TSUNIT_ASSERT(it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(0, it->size());
    TSUNIT_EQUAL(0, it->toInt32());
    TSUNIT_EQUAL(0, it->toUInt32());
    TSUNIT_EQUAL(0, it->toInt64());
    TSUNIT_EQUAL(0, it->toUInt64());
    TSUNIT_EQUAL("ok 2", it->toCharPtr());
    TSUNIT_EQUAL(u"ok 2", it->toUCharPtr());
    TSUNIT_EQUAL("ok 2", it->toString());
    TSUNIT_EQUAL(u"ok 2", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // us + u" 2"
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(!it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(0, it->size());
    TSUNIT_EQUAL(0, it->toInt32());
    TSUNIT_EQUAL(0, it->toUInt32());
    TSUNIT_EQUAL(0, it->toInt64());
    TSUNIT_EQUAL(0, it->toUInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"an UString 2", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"an UString 2", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // sz = 8 (size_t)
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(it->isUnsigned());
    TSUNIT_ASSERT(!it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(sizeof(size_t), it->size());
    TSUNIT_EQUAL(8, it->toInt32());
    TSUNIT_EQUAL(8, it->toUInt32());
    TSUNIT_EQUAL(8, it->toInt64());
    TSUNIT_EQUAL(8, it->toUInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // EB (48, uint16_t)
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(it->isUnsigned());
    TSUNIT_ASSERT(!it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(2, it->size());
    TSUNIT_EQUAL(48, it->toInt32());
    TSUNIT_EQUAL(48, it->toUInt32());
    TSUNIT_EQUAL(48, it->toInt64());
    TSUNIT_EQUAL(48, it->toUInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // EC (4, int8_t)
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(it->isInteger());
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(!it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(1, it->size());
    TSUNIT_EQUAL(4, it->toInt32());
    TSUNIT_EQUAL(4, it->toUInt32());
    TSUNIT_EQUAL(4, it->toInt64());
    TSUNIT_EQUAL(4, it->toUInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // IPSocketAddress(10, 20, 30, 40, 12345);
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(!it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL(0, it->size());
    TSUNIT_EQUAL(0, it->toInt32());
    TSUNIT_EQUAL(0, it->toUInt32());
    TSUNIT_EQUAL(0, it->toInt64());
    TSUNIT_EQUAL(0, it->toUInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"10.20.30.40:12345", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"10.20.30.40:12345", it->toUString());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // fs::path(u"foo.bar")
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(!it->isInteger());
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(it->isAnyString());
#if defined(TS_WINDOWS)
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(it->isAnyString16());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(it->isUString());
    TSUNIT_EQUAL("", it->toCharPtr());
#else
    TSUNIT_ASSERT(it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(it->isString());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(!it->isChrono());
    TSUNIT_EQUAL("foo.bar", it->toCharPtr());
#endif
    TSUNIT_EQUAL(u"foo.bar", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"foo.bar", it->toUString());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_EQUAL(0, it->size());
    TSUNIT_EQUAL(0, it->toInt32());
    TSUNIT_EQUAL(0, it->toUInt32());
    TSUNIT_EQUAL(0, it->toInt64());
    TSUNIT_EQUAL(0, it->toUInt64());
    TSUNIT_EQUAL(0, it->num());
    TSUNIT_EQUAL(0, it->den());
    ++it;

    // cn::milliseconds(-27)
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(it->isInteger());
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(!it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(it->isChrono());
    TSUNIT_EQUAL(8, it->size());
    TSUNIT_EQUAL(-27, it->toInt32());
    TSUNIT_EQUAL(-27, it->toInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"", it->toUString());
    TSUNIT_EQUAL(cn::milliseconds::period::num, it->num());
    TSUNIT_EQUAL(cn::milliseconds::period::den, it->den());
    ++it;

    // cn::days(12)
    TSUNIT_ASSERT(!it->isOutputInteger());
    TSUNIT_ASSERT(it->isInteger());
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_ASSERT(!it->isAnyString());
    TSUNIT_ASSERT(!it->isAnyString8());
    TSUNIT_ASSERT(!it->isAnyString16());
    TSUNIT_ASSERT(!it->isCharPtr());
    TSUNIT_ASSERT(!it->isString());
    TSUNIT_ASSERT(!it->isUCharPtr());
    TSUNIT_ASSERT(!it->isUString());
    TSUNIT_ASSERT(it->isChrono());
    TSUNIT_EQUAL(8, it->size());
    TSUNIT_EQUAL(12, it->toInt32());
    TSUNIT_EQUAL(12, it->toInt64());
    TSUNIT_EQUAL("", it->toCharPtr());
    TSUNIT_EQUAL(u"", it->toUCharPtr());
    TSUNIT_EQUAL("", it->toString());
    TSUNIT_EQUAL(u"", it->toUString());
    TSUNIT_EQUAL(cn::days::period::num, it->num());
    TSUNIT_EQUAL(cn::days::period::den, it->den());
    ++it;

    TSUNIT_ASSERT(it == list.end());
}

TSUNIT_DEFINE_TEST(Format)
{
    TSUNIT_EQUAL(u"", ts::UString::Format(u""));
    TSUNIT_EQUAL(u"abc", ts::UString::Format(u"abc"));

    TSUNIT_EQUAL(u"abc1", ts::UString::Format(u"abc%d", 1));
    TSUNIT_EQUAL(u"abc1de%f", ts::UString::Format(u"abc%dde%%f", 1));

    // Invalid formats / arguments. Define environment variable TSDUCK_FORMAT_DEBUG to get error messages.
    TSUNIT_EQUAL(u"a) 1 2",     ts::UString::Format(u"a) %d %d", 1, 2, 3, 4));
    TSUNIT_EQUAL(u"b) 1 ",      ts::UString::Format(u"b) %d %d", 1));
    TSUNIT_EQUAL(u"c) 1 abc",   ts::UString::Format(u"c) %d %d", 1, u"abc"));
    TSUNIT_EQUAL(u"d) 1 2",     ts::UString::Format(u"d) %d %s", 1, 2));
    TSUNIT_EQUAL(u"e) abXcdef", ts::UString::Format(u"e) ab%scd%sef", u"X"));
    TSUNIT_EQUAL(u"f) 1 ",      ts::UString::Format(u"f) %d %01", 1, 2, 3));

    int i = -1234;
    uint16_t u16 = 128;
    const ts::UString us(u"abc");
    const std::string s("def");
    TSUNIT_EQUAL(u"i = -1,234, u16 = 0x0080, 27 abc def ghi jkl", ts::UString::Format(u"i = %'d, u16 = 0x%X, %d %s %s %s %s", i, u16, 27, us, s, u"ghi", "jkl"));

    // Character.
    const ts::UString ref1({u'A', ts::GREEK_CAPITAL_LETTER_ALPHA_WITH_TONOS, u'B'});
    TSUNIT_EQUAL(ref1, ts::UString::Format(u"A%cB", int(ts::GREEK_CAPITAL_LETTER_ALPHA_WITH_TONOS)));
    TSUNIT_EQUAL(ref1, ts::UString::Format(u"A%cB", ts::GREEK_CAPITAL_LETTER_ALPHA_WITH_TONOS));
    TSUNIT_EQUAL(u"AxyB", ts::UString::Format(u"A%c%cB", 'x', u'y'));

    // Decimal integer.
    TSUNIT_EQUAL(u"1234567", ts::UString::Format(u"%d", 1234567));
    TSUNIT_EQUAL(u"+1234567", ts::UString::Format(u"%+d", 1234567));
    TSUNIT_EQUAL(u"1,234,567", ts::UString::Format(u"%'d", 1234567));
    TSUNIT_EQUAL(u"+1,234,567", ts::UString::Format(u"%+'d", 1234567));
    TSUNIT_EQUAL(u"1,234,567", ts::UString::Format(u"%-'d", 1234567));
    TSUNIT_EQUAL(u"1234567", ts::UString::Format(u"%0d", 1234567));
    TSUNIT_EQUAL(u"1234567", ts::UString::Format(u"%05d", 1234567));
    TSUNIT_EQUAL(u"0001234567", ts::UString::Format(u"%010d", 1234567));
    TSUNIT_EQUAL(u"   1234567", ts::UString::Format(u"%10d", 1234567));
    TSUNIT_EQUAL(u"1234567   ", ts::UString::Format(u"%-10d", 1234567));
    TSUNIT_EQUAL(u"     1234567", ts::UString::Format(u"%*d", 12, 1234567));
    TSUNIT_EQUAL(u"1234567     ", ts::UString::Format(u"%-*d", 12, 1234567));
    TSUNIT_EQUAL(u"1,234,567   ", ts::UString::Format(u"%-*'d", 12, 1234567));

    // Hexadecimal integer.
    TSUNIT_EQUAL(u"AB", ts::UString::Format(u"%X", uint8_t(171)));
    TSUNIT_EQUAL(u"00AB", ts::UString::Format(u"%X", int16_t(171)));
    TSUNIT_EQUAL(u"000000AB", ts::UString::Format(u"%X", uint32_t(171)));
    TSUNIT_EQUAL(u"00000000000000AB", ts::UString::Format(u"%X", 171LL));
    TSUNIT_EQUAL(u"000000000000000000AB", ts::UString::Format(u"%20X", 171LL));
    TSUNIT_EQUAL(u"00AB", ts::UString::Format(u"%*X", 4, 171LL));
    TSUNIT_EQUAL(u"AB", ts::UString::Format(u"%*X", 1, 171LL));
    TSUNIT_EQUAL(u"0123,4567", ts::UString::Format(u"%'X", uint32_t(0x1234567)));

    // Enumerations
    enum E1 : uint8_t {E10 = 10, E11 = 11};
    enum E2 : int16_t {E20 = 20, E21 = 21};
    E1 e1 = E11;
    TSUNIT_EQUAL(u"11 10", ts::UString::Format(u"%d %d", e1, E10));
    TSUNIT_EQUAL(u"0B", ts::UString::Format(u"%X", e1));
    TSUNIT_EQUAL(u"0B", ts::UString::Format(u"%X", E11));
    TSUNIT_EQUAL(u"0014", ts::UString::Format(u"%X", E20));

    // String.
    TSUNIT_EQUAL(u"||", ts::UString::Format(u"|%s|"));
    TSUNIT_EQUAL(u"|abc|", ts::UString::Format(u"|%s|", "abc"));
    TSUNIT_EQUAL(u"|abc|", ts::UString::Format(u"|%s|", u"abc"));
    TSUNIT_EQUAL(u"|abc|", ts::UString::Format(u"|%s|", std::string("abc")));
    TSUNIT_EQUAL(u"|abc|", ts::UString::Format(u"|%s|", ts::UString(u"abc")));
    TSUNIT_EQUAL(u"|abc|", ts::UString::Format(u"|%2s|", u"abc"));
    TSUNIT_EQUAL(u"| abc|", ts::UString::Format(u"|%4s|", u"abc"));
    TSUNIT_EQUAL(u"|abc |", ts::UString::Format(u"|%-4s|", u"abc"));
    TSUNIT_EQUAL(u"|000abc|", ts::UString::Format(u"|%06s|", u"abc"));
    TSUNIT_EQUAL(u"|abc000|", ts::UString::Format(u"|%-06s|", u"abc"));
    TSUNIT_EQUAL(u"|abc     |", ts::UString::Format(u"|%-*s|", 8, u"abc"));
    TSUNIT_EQUAL(u"|abc     |", ts::UString::Format(u"|%-*.*s|", 8, 12, u"abc"));
    TSUNIT_EQUAL(u"|abcdefgh|", ts::UString::Format(u"|%-*.*s|", 8, 12, u"abcdefgh"));
    TSUNIT_EQUAL(u"|abcdefghijkl|", ts::UString::Format(u"|%-*.*s|", 8, 12, u"abcdefghijklmnop"));
    TSUNIT_EQUAL(u"|abcdefghijklmnop|", ts::UString::Format(u"|%-*s|", 8, u"abcdefghijklmnop"));

    // Stringifiable.
    TSUNIT_EQUAL(u"|1.2.3.4|", ts::UString::Format(u"|%s|", ts::IPAddress(1, 2, 3, 4)));
    TSUNIT_EQUAL(u"|11.22.33.44:678|", ts::UString::Format(u"|%s|", ts::IPSocketAddress(11, 22, 33, 44, 678)));

    // Boolean.
    bool b = false;
    TSUNIT_EQUAL(u"|false|", ts::UString::Format(u"|%s|", b));
    TSUNIT_EQUAL(u"|true|", ts::UString::Format(u"|%s|", b = true));
    TSUNIT_EQUAL(u"|true|", ts::UString::Format(u"|%s|", true));
    TSUNIT_EQUAL(u"|false|", ts::UString::Format(u"|%s|", false));
    TSUNIT_EQUAL(u"|    true|", ts::UString::Format(u"|%8s|", true));
    TSUNIT_EQUAL(u"|false   |", ts::UString::Format(u"|%-8s|", false));

    // Floats.
    TSUNIT_EQUAL(u"0.666667", ts::UString::Format(u"%f", 2.0 / 3.0));
    TSUNIT_EQUAL(u"-0.666667", ts::UString::Format(u"%f", 2.0 / -3.0));
    TSUNIT_EQUAL(u"+0.666667", ts::UString::Format(u"%+f", 2.0 / 3.0));
    TSUNIT_EQUAL(u"-0.666667", ts::UString::Format(u"%+f", 2.0 / -3.0));
    TSUNIT_EQUAL(u"2.000000", ts::UString::Format(u"%f", 2.0));
    TSUNIT_EQUAL(u"2.000000", ts::UString::Format(u"%f", 2));
    TSUNIT_EQUAL(u"-2.000000", ts::UString::Format(u"%f", -2));
    TSUNIT_EQUAL(u" 0.667", ts::UString::Format(u"%6.3f", 2.0 / 3.0));

    // Repeat previous argument.
    TSUNIT_EQUAL(u"1 1 2", ts::UString::Format(u"%d %<d %d", 1, 2));
    TSUNIT_EQUAL(u" 1 2", ts::UString::Format(u"%<d %d %d", 1, 2));
    TSUNIT_EQUAL(u"   1   1 2", ts::UString::Format(u"%*d %<*d %d", 4, 1, 3, 2));

    // std::chrono::duration values.
    TSUNIT_EQUAL(u"-1234 milliseconds", ts::UString::Format(u"%s", cn::milliseconds(-1234)));
    TSUNIT_EQUAL(u"1,234 ms", ts::UString::Format(u"%'!s", cn::milliseconds(1234)));
    TSUNIT_EQUAL(u"-1 millisecond", ts::UString::Format(u"%s", cn::milliseconds(-1)));
    TSUNIT_EQUAL(u"|    1,234 ms|", ts::UString::Format(u"|%12'!s|", cn::milliseconds(1234)));
    TSUNIT_EQUAL(u"|1,234 ms|", ts::UString::Format(u"|%6'!s|", cn::milliseconds(1234)));
    TSUNIT_EQUAL(u"1 PCR", ts::UString::Format(u"%s", ts::PCR(1)));
    TSUNIT_EQUAL(u"12345 PCR", ts::UString::Format(u"%!s", ts::PCR(12345)));
    TSUNIT_EQUAL(u"+12,345 microseconds", ts::UString::Format(u"%+'s", cn::microseconds(12345)));

    // Normalized hexadecimal + decimal.
    TSUNIT_EQUAL(u"0xAB (171)", ts::UString::Format(u"%n", uint8_t(171)));
    TSUNIT_EQUAL(u"0x00AB (171)", ts::UString::Format(u"%n", int16_t(171)));
    TSUNIT_EQUAL(u"0x00AB (+171)", ts::UString::Format(u"%+n", int16_t(171)));
    TSUNIT_EQUAL(u"0xFF55 (-171)", ts::UString::Format(u"%n", int16_t(-171)));
    TSUNIT_EQUAL(u"0x000000AB (171)", ts::UString::Format(u"%n", uint32_t(171)));
    TSUNIT_EQUAL(u"0x00000000000000AB (171)", ts::UString::Format(u"%n", 171LL));
    TSUNIT_EQUAL(u"0x1234,5678 (305,419,896)", ts::UString::Format(u"%'n", 305419896));
    TSUNIT_EQUAL(u"0x000000AB (171)    ", ts::UString::Format(u"%-20n", uint32_t(171)));
    TSUNIT_EQUAL(u"    0x000000AB (171)", ts::UString::Format(u"%20n", uint32_t(171)));
    TSUNIT_EQUAL(u"00AB (171)", ts::UString::Format(u"%.10n", uint32_t(171)));
    TSUNIT_EQUAL(u"0x000000AB", ts::UString::Format(u"%-.10n", uint32_t(171)));
    TSUNIT_EQUAL(u"  0x000000AB (171)", ts::UString::Format(u"%*n", 18, uint32_t(171)));
}

TSUNIT_DEFINE_TEST(ArgMixOut)
{
    enum E1 : uint16_t {E10 = 5, E11, E12, E13};
    enum E2 : int8_t   {E20 = -10, E21, E22, E23};

    int8_t   i8  = -2;
    uint8_t  u8  = 23;
    int16_t  i16 = -432;
    uint16_t u16 = 439;
    int32_t  i32 = -123456;
    uint32_t u32 = 987654;
    int64_t  i64 = -1234567890123;
    uint64_t u64 = 9876543210657;
    size_t   sz  = 8;
    E1       e1  = E11;
    E2       e2  = E20;

    testArgMixOutCalled({&i8, &u8, &i16, &u16, &i32, &u32, &i64, &u64, &sz, &e1, &e2});

    TSUNIT_EQUAL(-1, i8);
    TSUNIT_EQUAL(24, u8);
    TSUNIT_EQUAL(-431, i16);
    TSUNIT_EQUAL(440, u16);
    TSUNIT_EQUAL(-123455, i32);
    TSUNIT_EQUAL(987655, u32);
    TSUNIT_EQUAL(-1234567890122, i64);
    TSUNIT_EQUAL(9876543210658, u64);
    TSUNIT_EQUAL(9, sz);
    TSUNIT_EQUAL(E12, e1);
    TSUNIT_EQUAL(E21, e2);
}

TSUNIT_DEFINE_TEST(ArgMixOutFloat)
{
    double d = 0.0;
    ts::ArgMixOut a(&d);

    a.storeFloat(4.5);
    TSUNIT_EQUAL(4.5, d);

    a.storeFloat(-3.25);
    TSUNIT_EQUAL(-3.25, d);
}

void UStringTest::testArgMixOutCalled(std::initializer_list<ts::ArgMixOut> list)
{
    TSUNIT_EQUAL(11, list.size());

    int32_t  i32 = 0;
    uint32_t u32 = 0;
    int64_t  i64 = 0;
    uint64_t u64 = 0;

    // Test all invariants.
    for (const auto& elem : list) {
        TSUNIT_ASSERT(elem.isOutputInteger());
        TSUNIT_ASSERT(elem.isInteger());
        TSUNIT_ASSERT(!elem.isAnyString());
        TSUNIT_ASSERT(!elem.isAnyString8());
        TSUNIT_ASSERT(!elem.isAnyString16());
        TSUNIT_ASSERT(!elem.isCharPtr());
        TSUNIT_ASSERT(!elem.isString());
        TSUNIT_ASSERT(!elem.isUCharPtr());
        TSUNIT_ASSERT(!elem.isUString());
    }

    // Now test specific values.
    auto it = list.begin();

    // int8_t   i8  = -2;
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_EQUAL(1, it->size());
    TSUNIT_EQUAL(-2, i32 = it->toInt32());
    TSUNIT_ASSERT(it->storeInteger(++i32));
    ++it;

    // uint8_t  u8  = 23;
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(it->isUnsigned());
    TSUNIT_EQUAL(1, it->size());
    TSUNIT_EQUAL(23, u32 = it->toUInt32());
    TSUNIT_ASSERT(it->storeInteger(++u32));
    ++it;

    // int16_t  i16 = -432;
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_EQUAL(2, it->size());
    TSUNIT_EQUAL(-432, i32 = it->toInt32());
    TSUNIT_ASSERT(it->storeInteger(++i32));
    ++it;

    // uint16_t u16 = 439;
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(it->isUnsigned());
    TSUNIT_EQUAL(2, it->size());
    TSUNIT_EQUAL(439, u32 = it->toUInt32());
    TSUNIT_ASSERT(it->storeInteger(++u32));
    ++it;

    // int32_t  i32 = -123456;
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_EQUAL(4, it->size());
    TSUNIT_EQUAL(-123456, i32 = it->toInt32());
    TSUNIT_ASSERT(it->storeInteger(++i32));
    ++it;

    // uint32_t u32 = 987654;
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(it->isUnsigned());
    TSUNIT_EQUAL(4, it->size());
    TSUNIT_EQUAL(987654, u32 = it->toUInt32());
    TSUNIT_ASSERT(it->storeInteger(++u32));
    ++it;

    // int64_t  i64 = -1234567890123;
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_EQUAL(8, it->size());
    TSUNIT_EQUAL(-1234567890123, i64 = it->toInt64());
    TSUNIT_ASSERT(it->storeInteger(++i64));
    ++it;

    // uint64_t u64 = 9876543210657;
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(it->isUnsigned());
    TSUNIT_EQUAL(8, it->size());
    TSUNIT_EQUAL(9876543210657, u64 = it->toUInt64());
    TSUNIT_ASSERT(it->storeInteger(++u64));
    ++it;

    // size_t   sz  = 8;
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(it->isUnsigned());
    TSUNIT_EQUAL(sizeof(size_t), it->size());
    TSUNIT_EQUAL(8, u64 = it->toUInt64());
    TSUNIT_ASSERT(it->storeInteger(++u64));
    ++it;

    // E1       e1  = E11;
    TSUNIT_ASSERT(!it->isSigned());
    TSUNIT_ASSERT(it->isUnsigned());
    TSUNIT_EQUAL(2, it->size());
    TSUNIT_EQUAL(6, u32 = it->toUInt32());
    TSUNIT_ASSERT(it->storeInteger(++u32));
    ++it;

    // E2       e2  = E20;
    TSUNIT_ASSERT(it->isSigned());
    TSUNIT_ASSERT(!it->isUnsigned());
    TSUNIT_EQUAL(1, it->size());
    TSUNIT_EQUAL(-10, i32 = it->toInt32());
    TSUNIT_ASSERT(it->storeInteger(++i32));
    ++it;

    TSUNIT_ASSERT(it == list.end());
}

TSUNIT_DEFINE_TEST(Scan)
{
    size_t count = 0;
    size_t index = 0;
    int i = 0;
    uint8_t u8 = 0;
    int16_t i16 = 0;
    uint32_t u32 = 0;
    int64_t i64 = 0;
    double d = 0.0;
    ts::UChar uc = ts::CHAR_NULL;

    TSUNIT_ASSERT(ts::UString(u"").scan(count, index, u""));
    TSUNIT_EQUAL(0, count);
    TSUNIT_EQUAL(0, index);

    TSUNIT_ASSERT(ts::UString(u"  ").scan(count, index, u" "));
    TSUNIT_EQUAL(0, count);
    TSUNIT_EQUAL(2, index);

    TSUNIT_ASSERT(ts::UString(u" ").scan(count, index, u"   "));
    TSUNIT_EQUAL(0, count);
    TSUNIT_EQUAL(1, index);

    TSUNIT_ASSERT(ts::UString(u"-133").scan(count, index, u"%d", &i));
    TSUNIT_EQUAL(1, count);
    TSUNIT_EQUAL(4, index);
    TSUNIT_EQUAL(-133, i);

    TSUNIT_ASSERT(ts::UString(u"  6893  ").scan(count, index, u"%d", &i));
    TSUNIT_EQUAL(1, count);
    TSUNIT_EQUAL(8, index);
    TSUNIT_EQUAL(6893, i);

    TSUNIT_ASSERT(ts::UString(u" -654 / 0x54/0x0123456789ABCDEF x 54:5  ").scan(count, index, u" %d/%d/%d%c%d:%d", &i, &u8, &i64, &uc, &i16, &u32));
    TSUNIT_EQUAL(6, count);
    TSUNIT_EQUAL(40, index);
    TSUNIT_EQUAL(-654, i);
    TSUNIT_EQUAL(0x54, u8);
    TSUNIT_EQUAL(0x0123456789ABCDEF, i64);
    TSUNIT_EQUAL(u'x', uc);
    TSUNIT_EQUAL(54, i16);
    TSUNIT_EQUAL(5, u32);

    u32 = 27;
    TSUNIT_ASSERT(!ts::UString(u" 45 / 79").scan(count, index, u" %d/%d/%d", &u8, &i16, &u32));
    TSUNIT_EQUAL(2, count);
    TSUNIT_EQUAL(8, index);
    TSUNIT_EQUAL(45, u8);
    TSUNIT_EQUAL(79, i16);
    TSUNIT_EQUAL(27, u32);

    i = 87;
    TSUNIT_ASSERT(!ts::UString(u" 67 / 657 / 46 / 78").scan(count, index, u" %d/%d/%d", &u8, &i16, &u32, &i));
    TSUNIT_EQUAL(3, count);
    TSUNIT_EQUAL(15, index);
    TSUNIT_EQUAL(67, u8);
    TSUNIT_EQUAL(657, i16);
    TSUNIT_EQUAL(46, u32);
    TSUNIT_EQUAL(87, i);

    TSUNIT_ASSERT(!ts::UString(u" 98 / -7889 / 89 / 2 ").scan(count, index, u" %d/%d/%d", &u8, &i16));
    TSUNIT_EQUAL(2, count);
    TSUNIT_EQUAL(14, index);
    TSUNIT_EQUAL(98, u8);
    TSUNIT_EQUAL(-7889, i16);

    TSUNIT_ASSERT(ts::UString(u"8/9/").scan(count, index, u" %i/%i/", &u8, &i16));
    TSUNIT_EQUAL(2, count);
    TSUNIT_EQUAL(4, index);
    TSUNIT_EQUAL(8, u8);
    TSUNIT_EQUAL(9, i16);

    TSUNIT_ASSERT(ts::UString(u"73/-3457").scan(u" %i/%i", &u8, &i16));
    TSUNIT_EQUAL(73, u8);
    TSUNIT_EQUAL(-3457, i16);

    TSUNIT_ASSERT(ts::UString(u"12345").scan(u"%d", &i));
    TSUNIT_EQUAL(12345, i);

    TSUNIT_ASSERT(!ts::UString(u"62,852").scan(u"%d", &i));

    TSUNIT_ASSERT(ts::UString(u"67,654").scan(u"%'d", &i));
    TSUNIT_EQUAL(67654, i);

    TSUNIT_ASSERT(ts::UString(u"1.5").scan(u"%f", &d));
    TSUNIT_EQUAL(1.5, d);

    TSUNIT_ASSERT(ts::UString(u"  1,234.5").scan(u"%'f", &d));
    TSUNIT_EQUAL(1234.5, d);

    u32 = 97;
    TSUNIT_ASSERT(!ts::UString(u" 1,234.5  ").scan(count, index, u" %f", &d, &u32));
    TSUNIT_EQUAL(1, count);
    TSUNIT_EQUAL(2, index);
    TSUNIT_EQUAL(1.0, d);

    TSUNIT_ASSERT(ts::UString(u"-3.5e+4").scan(u"%f", &d));
    TSUNIT_EQUAL(-35000.0, d);

    TSUNIT_ASSERT(ts::UString(u"3.5e-3").scan(u"%f", &d));
    TSUNIT_EQUAL(0.0035, d);
}

TSUNIT_DEFINE_TEST(CommonPrefix)
{
    TSUNIT_EQUAL(0, ts::UString(u"").commonPrefixSize(u""));
    TSUNIT_EQUAL(0, ts::UString(u"abc").commonPrefixSize(u"def"));
    TSUNIT_EQUAL(1, ts::UString(u"abc").commonPrefixSize(u"a"));
    TSUNIT_EQUAL(1, ts::UString(u"abc").commonPrefixSize(u"axyz"));
    TSUNIT_EQUAL(2, ts::UString(u"abcd").commonPrefixSize(u"abCXYZ"));
    TSUNIT_EQUAL(3, ts::UString(u"abcd").commonPrefixSize(u"abCXYZ", ts::CASE_INSENSITIVE));
}

TSUNIT_DEFINE_TEST(CommonSuffix)
{
    TSUNIT_EQUAL(0, ts::UString(u"").commonSuffixSize(u""));
    TSUNIT_EQUAL(0, ts::UString(u"abc").commonSuffixSize(u"def"));
    TSUNIT_EQUAL(1, ts::UString(u"abc").commonSuffixSize(u"c"));
    TSUNIT_EQUAL(1, ts::UString(u"abc").commonSuffixSize(u"xyc"));
    TSUNIT_EQUAL(2, ts::UString(u"abcd").commonSuffixSize(u"QSZBcd"));
    TSUNIT_EQUAL(3, ts::UString(u"abcd").commonSuffixSize(u"QSZBcd", ts::CASE_INSENSITIVE));
}

TSUNIT_DEFINE_TEST(Precombined)
{
    TSUNIT_EQUAL(ts::LATIN_SMALL_LETTER_I_WITH_GRAVE, ts::Precombined(ts::LATIN_SMALL_LETTER_I, ts::COMBINING_GRAVE_ACCENT));
    TSUNIT_EQUAL(ts::LATIN_CAPITAL_LETTER_K_WITH_CEDILLA, ts::Precombined(ts::LATIN_CAPITAL_LETTER_K, ts::COMBINING_CEDILLA));
    TSUNIT_EQUAL(ts::CHAR_NULL, ts::Precombined(ts::LATIN_CAPITAL_LETTER_K, ts::COMBINING_GREEK_DIALYTIKA_TONOS));

    ts::UChar letter = ts::CHAR_NULL;
    ts::UChar mark = ts::CHAR_NULL;

    TSUNIT_ASSERT(!ts::DecomposePrecombined(ts::LATIN_CAPITAL_LETTER_A, letter, mark));

    TSUNIT_ASSERT(ts::DecomposePrecombined(ts::GREEK_CAPITAL_LETTER_IOTA_WITH_TONOS, letter, mark));
    TSUNIT_EQUAL(ts::GREEK_CAPITAL_LETTER_IOTA, letter);
    TSUNIT_EQUAL(ts::COMBINING_GREEK_DIALYTIKA_TONOS, mark);

    TSUNIT_ASSERT(ts::DecomposePrecombined(ts::LATIN_CAPITAL_LETTER_D_WITH_DOT_ABOVE, letter, mark));
    TSUNIT_EQUAL(ts::LATIN_CAPITAL_LETTER_D, letter);
    TSUNIT_EQUAL(ts::COMBINING_DOT_ABOVE, mark);

    TSUNIT_EQUAL(u"", ts::UString().toCombinedDiacritical());
    TSUNIT_EQUAL(u"abcdef", ts::UString(u"abcdef").toCombinedDiacritical());

    const ts::UString ref1{u'a', ts::LATIN_SMALL_LETTER_S_WITH_CIRCUMFLEX, ts::GREEK_CAPITAL_LETTER_IOTA_WITH_TONOS, u'z'};
    const ts::UString str1{u'a', ts::LATIN_SMALL_LETTER_S, ts::COMBINING_CIRCUMFLEX_ACCENT, ts::GREEK_CAPITAL_LETTER_IOTA, ts::COMBINING_GREEK_DIALYTIKA_TONOS, u'z'};
    TSUNIT_EQUAL(ref1, str1.toCombinedDiacritical());

    const ts::UString ref2{ts::LATIN_SMALL_LETTER_S_WITH_CIRCUMFLEX, u'a', ts::GREEK_CAPITAL_LETTER_IOTA_WITH_TONOS};
    const ts::UString str2{ts::LATIN_SMALL_LETTER_S, ts::COMBINING_CIRCUMFLEX_ACCENT, u'a', ts::GREEK_CAPITAL_LETTER_IOTA, ts::COMBINING_GREEK_DIALYTIKA_TONOS};
    TSUNIT_EQUAL(ref2, str2.toCombinedDiacritical());

    TSUNIT_EQUAL(u"", ts::UString().toDecomposedDiacritical());
    TSUNIT_EQUAL(u"abcdef", ts::UString(u"abcdef").toDecomposedDiacritical());
    TSUNIT_EQUAL(str1, ref1.toDecomposedDiacritical());
    TSUNIT_EQUAL(str2, ref2.toDecomposedDiacritical());
}

TSUNIT_DEFINE_TEST(Quoted)
{
    TSUNIT_EQUAL(u"''", ts::UString().toQuoted());
    TSUNIT_EQUAL(u"||", ts::UString().toQuoted(u'|'));
    TSUNIT_EQUAL(u"a", ts::UString(u"a").toQuoted());
    TSUNIT_EQUAL(u"'a b'", ts::UString(u"a b").toQuoted());
    TSUNIT_EQUAL(u"a.b", ts::UString(u"a.b").toQuoted());
    TSUNIT_EQUAL(u"'a?b'", ts::UString(u"a?b").toQuoted());
    TSUNIT_EQUAL(u"'a\\nb'", ts::UString(u"a\nb").toQuoted());
    TSUNIT_EQUAL(u"'a\\\\b'", ts::UString(u"a\\b").toQuoted());
}

TSUNIT_DEFINE_TEST(Unquoted)
{
    TSUNIT_EQUAL(u"", ts::UString().toUnquoted());
    TSUNIT_EQUAL(u"abcd", ts::UString("abcd").toUnquoted());
    TSUNIT_EQUAL(u"abcd", ts::UString("'abcd'").toUnquoted());
    TSUNIT_EQUAL(u"abcd", ts::UString("''\"abcd\"''").toUnquoted());
}

TSUNIT_DEFINE_TEST(ToQuotedLine)
{
    TSUNIT_EQUAL(u"", ts::UString::ToQuotedLine(ts::UStringVector()));
    TSUNIT_EQUAL(u"''", ts::UString::ToQuotedLine(ts::UStringVector({u""})));
    TSUNIT_EQUAL(u"''", ts::UString::ToQuotedLine(ts::UStringVector({u""})));
    TSUNIT_EQUAL(u"|| ||", ts::UString::ToQuotedLine(ts::UStringVector({u"", u""}), u'|'));
    TSUNIT_EQUAL(u"ab cde fghi", ts::UString::ToQuotedLine(ts::UStringVector({u"ab", u"cde", u"fghi"})));
    TSUNIT_EQUAL(u"ab 'er ty' 'sdf?hh' ' \"vf\\ndf\" '", ts::UString::ToQuotedLine(ts::UStringVector({u"ab", u"er ty", u"sdf?hh", u" \"vf\ndf\" "})));
    TSUNIT_EQUAL(u"ab 'er ty' 'sdf?hh' ' \\'vf\\ndf\\' '", ts::UString::ToQuotedLine(ts::UStringVector({u"ab", u"er ty", u"sdf?hh", u" 'vf\ndf' "})));
}

TSUNIT_DEFINE_TEST(FromQuotedLine)
{
    ts::UStringVector s;

    ts::UString(u"ab cd ef").fromQuotedLine(s);
    TSUNIT_EQUAL(3, s.size());
    TSUNIT_EQUAL(u"ab", s[0]);
    TSUNIT_EQUAL(u"cd", s[1]);
    TSUNIT_EQUAL(u"ef", s[2]);

    ts::UString().fromQuotedLine(s);
    TSUNIT_EQUAL(0, s.size());

    ts::UString(u" a'b c'd ef    ").fromQuotedLine(s);
    TSUNIT_EQUAL(2, s.size());
    TSUNIT_EQUAL(u"ab cd", s[0]);
    TSUNIT_EQUAL(u"ef", s[1]);

    ts::UString(u" a\\ \\nb c[d\\ e]f    ").fromQuotedLine(s);
    TSUNIT_EQUAL(2, s.size());
    TSUNIT_EQUAL(u"a \nb", s[0]);
    TSUNIT_EQUAL(u"c[d e]f", s[1]);
}

TSUNIT_DEFINE_TEST(Indent)
{
    TSUNIT_EQUAL(u"", ts::UString().toIndented(0));
    TSUNIT_EQUAL(u"", ts::UString().toIndented(4));
    TSUNIT_EQUAL(u"  ", ts::UString(u"  ").toIndented(4));
    TSUNIT_EQUAL(u"      a", ts::UString(u"  a").toIndented(4));
    TSUNIT_EQUAL(u"    a\n\n  b\n  c d", ts::UString(u"  a\n\nb\nc d").toIndented(2));
}

TSUNIT_DEFINE_TEST(ToFloat)
{
    double val = 0.0;
    TSUNIT_ASSERT(!ts::UString().toFloat(val));

    TSUNIT_ASSERT(ts::UString(u"1.23").toFloat(val));
    TSUNIT_EQUAL(1.23, val);

    TSUNIT_ASSERT(!ts::UString(u"1c.23").toFloat(val));
    TSUNIT_EQUAL(1.0, val);

    TSUNIT_ASSERT(!ts::UString(u"c.23").toFloat(val));
    TSUNIT_EQUAL(0.0, val);

    TSUNIT_ASSERT(ts::UString(u"-34.56e-4").toFloat(val));
    TSUNIT_EQUAL(-34.56e-4, val);

    TSUNIT_ASSERT(ts::UString(u"78.31e12").toFloat(val));
    TSUNIT_EQUAL(78.31e12, val);

    TSUNIT_ASSERT(!ts::UString(u"1.2").toFloat(val, -1.0, 1.0));
    TSUNIT_EQUAL(1.2, val);

    TSUNIT_ASSERT(ts::UString(u"11").toFloat(val));
    TSUNIT_EQUAL(11.0, val);
}

TSUNIT_DEFINE_TEST(SuperCompare)
{
    TSUNIT_EQUAL(0, ts::UString::SuperCompare(nullptr, nullptr));
    TSUNIT_EQUAL(1, ts::UString::SuperCompare(u"", nullptr));
    TSUNIT_EQUAL(1, ts::UString::SuperCompare(u"abc", nullptr));
    TSUNIT_EQUAL(-1, ts::UString::SuperCompare(nullptr, u""));
    TSUNIT_EQUAL(-1, ts::UString::SuperCompare(nullptr, u"xyz"));
    TSUNIT_EQUAL(-1, ts::UString::SuperCompare(u"a bc", u"abc"));
    TSUNIT_EQUAL(0, ts::UString::SuperCompare(u"a bc", u"abc", ts::SCOMP_IGNORE_BLANKS));
    TSUNIT_EQUAL(0, ts::UString::SuperCompare(u"  \t a   \n b \r c  ", u"a bc", ts::SCOMP_IGNORE_BLANKS));
    TSUNIT_EQUAL(0, ts::UString::SuperCompare(u"a bC", u" aBc", ts::SCOMP_IGNORE_BLANKS | ts::SCOMP_CASE_INSENSITIVE));
    TSUNIT_EQUAL(-1, ts::UString::SuperCompare(u"version 8.12.3", u"VERSION 8.4.21", ts::SCOMP_IGNORE_BLANKS | ts::SCOMP_CASE_INSENSITIVE));
    TSUNIT_EQUAL(1, ts::UString::SuperCompare(u"version 8.12.3", u"VERSION 8.4.21", ts::SCOMP_IGNORE_BLANKS | ts::SCOMP_NUMERIC | ts::SCOMP_CASE_INSENSITIVE));
}

TSUNIT_DEFINE_TEST(ChronoUnit)
{
    TSUNIT_EQUAL(u"nanosecond", ts::UString::ChronoUnit<cn::nanoseconds>());
    TSUNIT_EQUAL(u"second", ts::UString::ChronoUnit<cn::seconds>());
    TSUNIT_EQUAL(u"hour", ts::UString::ChronoUnit<cn::hours>());

    TSUNIT_EQUAL(u"ns", ts::UString::ChronoUnit<cn::nanoseconds>(true));
    TSUNIT_EQUAL(u"s", ts::UString::ChronoUnit<cn::seconds>(true));
    TSUNIT_EQUAL(u"h", ts::UString::ChronoUnit<cn::hours>(true));

    TSUNIT_EQUAL(u"nanoseconds", ts::UString::ChronoUnit<cn::nanoseconds>(false, true));
    TSUNIT_EQUAL(u"seconds", ts::UString::ChronoUnit<cn::seconds>(false, true));
    TSUNIT_EQUAL(u"hours", ts::UString::ChronoUnit<cn::hours>(false, true));

    using centiseconds = cn::duration<int, std::ratio<2, 200>>;
    TSUNIT_EQUAL(u"1/100-second", ts::UString::ChronoUnit<centiseconds>());
    TSUNIT_EQUAL(u"1/100-sec", ts::UString::ChronoUnit<centiseconds>(true));

    using decaseconds = cn::duration<int, std::ratio<10>>;
    TSUNIT_EQUAL(u"10-second", ts::UString::ChronoUnit<decaseconds>());
    TSUNIT_EQUAL(u"10-sec", ts::UString::ChronoUnit<decaseconds>(true));
}

TSUNIT_DEFINE_TEST(Chrono)
{
    TSUNIT_EQUAL(u"12,345 nanoseconds", ts::UString::Chrono(cn::nanoseconds(12345)));
    TSUNIT_EQUAL(u"1 second", ts::UString::Chrono(cn::seconds(1)));
    TSUNIT_EQUAL(u"250 ms", ts::UString::Chrono(cn::milliseconds(250), true));
    TSUNIT_EQUAL(u"8,512 PCR", ts::UString::Chrono(ts::PCR(8512), true));
    TSUNIT_EQUAL(u"25 PTS/DTS", ts::UString::Chrono(ts::PTS(25)));
    TSUNIT_EQUAL(u"25 PTS/DTS", ts::UString::Chrono(ts::DTS(25)));
}

TSUNIT_DEFINE_TEST(Duration)
{
    TSUNIT_EQUAL(u"00:00:00.000", ts::UString::Duration(cn::hours::zero()));
    TSUNIT_EQUAL(u"00:00:00.000", ts::UString::Duration(cn::hours::zero(), true));
    TSUNIT_EQUAL(u"51:00:00.000", ts::UString::Duration(cn::hours(51)));
    TSUNIT_EQUAL(u"2d 03:00:00.000", ts::UString::Duration(cn::hours(51), true));
    TSUNIT_EQUAL(u"373:24:57.123", ts::UString::Duration(cn::milliseconds(1'344'297'123)));
    TSUNIT_EQUAL(u"15d 13:24:57.123", ts::UString::Duration(cn::milliseconds(1'344'297'123), true));
    TSUNIT_EQUAL(u"-373:24:57.123", ts::UString::Duration(cn::milliseconds(-1'344'297'123)));
    TSUNIT_EQUAL(u"-15d 13:24:57.123", ts::UString::Duration(cn::milliseconds(-1'344'297'123), true));
}

TSUNIT_DEFINE_TEST(Percentage)
{
    TSUNIT_EQUAL(u"?", ts::UString::Percentage(2, -1));
    TSUNIT_EQUAL(u"0.00%", ts::UString::Percentage(2, 0));
    TSUNIT_EQUAL(u"0.00%", ts::UString::Percentage(0, 34));
    TSUNIT_EQUAL(u"50.00%", ts::UString::Percentage(200, 400));
    TSUNIT_EQUAL(u"50.00%", ts::UString::Percentage(cn::milliseconds(500), cn::seconds(1)));
}
