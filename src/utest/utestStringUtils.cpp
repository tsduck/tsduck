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
//  CppUnit test suite for tsStringUtils.h and some other string utilies
//
//----------------------------------------------------------------------------

#include "tsStringUtils.h"
#include "tsAlgorithm.h"
#include "tsHexa.h"
#include "tsDecimal.h"
#include "tsFormat.h"
#include "tsToInteger.h"
#include "tsSysUtils.h"
#include "tsByteBlock.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;

#if defined(__unix)
#include <sys/types.h>
#include <unistd.h>
#endif


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class StringUtilsTest: public CppUnit::TestFixture
{
public:
    StringUtilsTest();
    void setUp();
    void tearDown();
    void testIsSpace();
    void testIsPrint();
    void testToPrintableCopy();
    void testToLowerString();
    void testToUpperString();
    void testToLowerCopy();
    void testToUpperCopy();
    void testTrim();
    void testTrimCopy();
    void testRemove();
    void testRemoveCopy();
    void testRemovePrefix();
    void testRemovePrefixCopy();
    void testRemoveSuffix();
    void testRemoveSuffixCopy();
    void testAppendUnique();
    void testAppendContainer();
    void testAssignContainer();
    void testSplit();
    void testJoin();
    void testBreakLines();
    void testJustifyLeft();
    void testJustifyRight();
    void testJustifyCentered();
    void testJustify();
    void testYesNo();
    void testTrueFalse();
    void testOnOff();
    void testSimilarStrings();
    void testFormat();
    void testHexaDecode();
    void testHexa();
    void testDecimal();
    void testLoadSave();
    void testToInteger();
    void testStart();
    void testEnd();
    void testSubstitute();

    CPPUNIT_TEST_SUITE(StringUtilsTest);
    CPPUNIT_TEST(testIsSpace);
    CPPUNIT_TEST(testIsPrint);
    CPPUNIT_TEST(testToPrintableCopy);
    CPPUNIT_TEST(testToLowerString);
    CPPUNIT_TEST(testToUpperString);
    CPPUNIT_TEST(testToLowerCopy);
    CPPUNIT_TEST(testToUpperCopy);
    CPPUNIT_TEST(testTrim);
    CPPUNIT_TEST(testTrimCopy);
    CPPUNIT_TEST(testRemove);
    CPPUNIT_TEST(testRemoveCopy);
    CPPUNIT_TEST(testRemovePrefix);
    CPPUNIT_TEST(testRemovePrefixCopy);
    CPPUNIT_TEST(testRemoveSuffix);
    CPPUNIT_TEST(testRemoveSuffixCopy);
    CPPUNIT_TEST(testAppendUnique);
    CPPUNIT_TEST(testAppendContainer);
    CPPUNIT_TEST(testAssignContainer);
    CPPUNIT_TEST(testSplit);
    CPPUNIT_TEST(testJoin);
    CPPUNIT_TEST(testBreakLines);
    CPPUNIT_TEST(testJustifyLeft);
    CPPUNIT_TEST(testJustifyRight);
    CPPUNIT_TEST(testJustifyCentered);
    CPPUNIT_TEST(testJustify);
    CPPUNIT_TEST(testYesNo);
    CPPUNIT_TEST(testTrueFalse);
    CPPUNIT_TEST(testOnOff);
    CPPUNIT_TEST(testSimilarStrings);
    CPPUNIT_TEST(testFormat);
    CPPUNIT_TEST(testHexaDecode);
    CPPUNIT_TEST(testHexa);
    CPPUNIT_TEST(testDecimal);
    CPPUNIT_TEST(testLoadSave);
    CPPUNIT_TEST(testToInteger);
    CPPUNIT_TEST(testStart);
    CPPUNIT_TEST(testEnd);
    CPPUNIT_TEST(testSubstitute);
    CPPUNIT_TEST_SUITE_END();

private:
    std::string _tempFilePrefix;
    int _nextFileIndex;
    std::string temporaryFileName(int) const;
    std::string newTemporaryFileName();
};

CPPUNIT_TEST_SUITE_REGISTRATION(StringUtilsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
StringUtilsTest::StringUtilsTest() :
    _tempFilePrefix(),
    _nextFileIndex(0)
{
}

// Test suite initialization method.
void StringUtilsTest::setUp()
{
    // Select the directory name and prefix for temporary files
    _tempFilePrefix = ts::TempFile(".");

    // Next file will use suffix "000"
    _nextFileIndex = 0;
}

// Test suite cleanup method.
void StringUtilsTest::tearDown()
{
    // Delete all temporary files
    std::vector<std::string> tempFiles;
    ts::ExpandWildcard(tempFiles, _tempFilePrefix + "*");
    for (std::vector<std::string>::const_iterator i = tempFiles.begin(); i != tempFiles.end(); ++i) {
        utest::Out() << "StringUtilsTest: deleting temporary file \"" << *i << "\"" << std::endl;
        ts::DeleteFile (*i);
    }
    _nextFileIndex = 0;
}

// Get the name of a temporary file from an index
std::string StringUtilsTest::temporaryFileName (int index) const
{
    const std::string name (_tempFilePrefix + ts::Format ("%03d", index));
    return name;
}

// Get the name of the next temporary file
std::string StringUtilsTest::newTemporaryFileName()
{
    return temporaryFileName (_nextFileIndex++);
}

// Reference byte array: 256 bytes, index == value
namespace {
    const uint8_t _bytes[] = {
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
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void StringUtilsTest::testIsSpace()
{
    CPPUNIT_ASSERT(ts::IsSpace (' '));
    CPPUNIT_ASSERT(ts::IsSpace ('\n'));
    CPPUNIT_ASSERT(ts::IsSpace ('\r'));
    CPPUNIT_ASSERT(ts::IsSpace ('\t'));
    CPPUNIT_ASSERT(ts::IsSpace ('\v'));
    CPPUNIT_ASSERT(ts::IsSpace ('\f'));
    CPPUNIT_ASSERT(!ts::IsSpace ('a'));
    CPPUNIT_ASSERT(!ts::IsSpace ('.'));
    CPPUNIT_ASSERT(!ts::IsSpace ('\0'));
}

void StringUtilsTest::testIsPrint()
{
    CPPUNIT_ASSERT(ts::IsPrintable('a'));
    CPPUNIT_ASSERT(ts::IsPrintable('.'));
    CPPUNIT_ASSERT(ts::IsPrintable('0'));
    CPPUNIT_ASSERT(ts::IsPrintable(' '));
    CPPUNIT_ASSERT(!ts::IsPrintable('\t'));
    CPPUNIT_ASSERT(!ts::IsPrintable('\n'));
    CPPUNIT_ASSERT(!ts::IsPrintable('\0'));
    CPPUNIT_ASSERT(!ts::IsPrintable('\177'));
}

void StringUtilsTest::testToPrintableCopy()
{
    CPPUNIT_ASSERT(ts::Printable("a.\t0\n\177 ") == "a..0.. ");
}

void StringUtilsTest::testToLowerString()
{
    std::string s("aAZzeR65=eR");
    ts::ToLowerCase(s);
    CPPUNIT_ASSERT(s == "aazzer65=er");
}

void StringUtilsTest::testToUpperString()
{
    std::string s("aAZzeR65=eR");
    ts::ToUpperCase(s);
    CPPUNIT_ASSERT(s == "AAZZER65=ER");
}

void StringUtilsTest::testToLowerCopy()
{
    CPPUNIT_ASSERT(ts::LowerCaseValue("aAZzeR65=eR") == "aazzer65=er");
}

void StringUtilsTest::testToUpperCopy()
{
    CPPUNIT_ASSERT(ts::UpperCaseValue("aAZzeR65=eR") == "AAZZER65=ER");
}

void StringUtilsTest::testTrim()
{
    std::string s;

    s = "  abc  ";
    ts::Trim (s);
    CPPUNIT_ASSERT(s == "abc");

    s = "  abc  ";
    ts::Trim (s, true, false);
    CPPUNIT_ASSERT(s == "abc  ");

    s = "  abc  ";
    ts::Trim (s, false, true);
    CPPUNIT_ASSERT(s == "  abc");

    s = "  abc  ";
    ts::Trim (s, false, false);
    CPPUNIT_ASSERT(s == "  abc  ");

    s = "abc";
    ts::Trim (s);
    CPPUNIT_ASSERT(s == "abc");

    s = "abc";
    ts::Trim (s, true, false);
    CPPUNIT_ASSERT(s == "abc");

    s = "abc";
    ts::Trim (s, false, true);
    CPPUNIT_ASSERT(s == "abc");

    s = "abc";
    ts::Trim (s, false, false);
    CPPUNIT_ASSERT(s == "abc");
}

void StringUtilsTest::testTrimCopy()
{
    CPPUNIT_ASSERT(ts::ReturnTrim("  abc  ") == "abc");
    CPPUNIT_ASSERT(ts::ReturnTrim("  abc  ", true, false) == "abc  ");
    CPPUNIT_ASSERT(ts::ReturnTrim("  abc  ", false, true) == "  abc");
    CPPUNIT_ASSERT(ts::ReturnTrim("  abc  ", false, false) == "  abc  ");
    CPPUNIT_ASSERT(ts::ReturnTrim("abc") == "abc");
    CPPUNIT_ASSERT(ts::ReturnTrim("abc", true, false) == "abc");
    CPPUNIT_ASSERT(ts::ReturnTrim("abc", false, true) == "abc");
    CPPUNIT_ASSERT(ts::ReturnTrim("abc", false, false) == "abc");
}

void StringUtilsTest::testRemove()
{
    std::string s;

    s = "az zef cer ";
    ts::RemoveSubstring(s, " ");
    CPPUNIT_ASSERT(s == "azzefcer");

    s = "fooAZfoo==fooBARfoo";
    ts::RemoveSubstring(s, "foo");
    CPPUNIT_ASSERT(s == "AZ==BAR");

    s = "fooAZfoo==fooBARfoo";
    const std::string foo ("foo");
    ts::RemoveSubstring(s, foo);
    CPPUNIT_ASSERT(s == "AZ==BAR");

    s = "fooAZfoo==fooBARfoo";
    ts::RemoveSubstring(s, "NOTTHERE");
    CPPUNIT_ASSERT(s == "fooAZfoo==fooBARfoo");

    s = "";
    ts::RemoveSubstring(s, "foo");
    CPPUNIT_ASSERT(s == "");

    s = "fooAZfoo==fooBARfoo";
    ts::RemoveSubstring(s, "");
    CPPUNIT_ASSERT(s == "fooAZfoo==fooBARfoo");

    s = "fooAZfoo==fooBARfoo";
    ts::RemoveSubstring(s, "o");
    CPPUNIT_ASSERT(s == "fAZf==fBARf");

    s = "fooAZfoo==fooBARfoo";
    ts::RemoveSubstring(s, "z");
    CPPUNIT_ASSERT(s == "fooAZfoo==fooBARfoo");
}

void StringUtilsTest::testRemoveCopy()
{
    std::string s;

    s = "az zef cer ";
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring(s, " ") == "azzefcer");

    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring("fooAZfoo==fooBARfoo", "foo") == "AZ==BAR");

    s = "fooAZfoo==fooBARfoo";
    const std::string foo ("foo");
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring(s, foo) == "AZ==BAR");
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring(s, "NOTTHERE") == "fooAZfoo==fooBARfoo");

    s = "";
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring(s, "foo") == "");

    s = "fooAZfoo==fooBARfoo";
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring(s, "")  == "fooAZfoo==fooBARfoo");
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring(s, "o") == "fAZf==fBARf");
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring(s, "z") == "fooAZfoo==fooBARfoo");
}

void StringUtilsTest::testRemovePrefix()
{
    std::string s;

    s = "abcdef";
    ts::RemovePrefix (s, "ab");
    CPPUNIT_ASSERT(s == "cdef");

    s = "abcdef";
    ts::RemovePrefix (s, "xy");
    CPPUNIT_ASSERT(s == "abcdef");

    s = "abcdef";
    ts::RemovePrefix (s, "");
    CPPUNIT_ASSERT(s == "abcdef");

    s = "";
    ts::RemovePrefix (s, "ab");
    CPPUNIT_ASSERT(s == "");
}

void StringUtilsTest::testRemovePrefixCopy()
{
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring("abcdef", "ab") == "cdef");
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring("abcdef", "xy") == "abcdef");
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring("abcdef", "") == "abcdef");
    CPPUNIT_ASSERT(ts::ReturnRemoveSubstring("", "ab") == "");
}

void StringUtilsTest::testRemoveSuffix()
{
    std::string s;

    s = "abcdef";
    ts::RemoveSuffix (s, "ef");
    CPPUNIT_ASSERT(s == "abcd");

    s = "abcdef";
    ts::RemoveSuffix (s, "xy");
    CPPUNIT_ASSERT(s == "abcdef");

    s = "abcdef";
    ts::RemoveSuffix (s, "");
    CPPUNIT_ASSERT(s == "abcdef");

    s = "";
    ts::RemoveSuffix (s, "ef");
    CPPUNIT_ASSERT(s == "");
}

void StringUtilsTest::testRemoveSuffixCopy()
{
    CPPUNIT_ASSERT(ts::ReturnRemoveSuffix("abcdef", "ef") == "abcd");
    CPPUNIT_ASSERT(ts::ReturnRemoveSuffix("abcdef", "xy") == "abcdef");
    CPPUNIT_ASSERT(ts::ReturnRemoveSuffix("abcdef", "") == "abcdef");
    CPPUNIT_ASSERT(ts::ReturnRemoveSuffix("", "ef") == "");
}

void StringUtilsTest::testAppendUnique()
{
    std::list<std::string> var;
    std::list<std::string> ref;
    CPPUNIT_ASSERT(var == ref);

    ts::AppendUnique (var, "abcd");
    ref.push_back ("abcd");
    CPPUNIT_ASSERT(var == ref);

    ts::AppendUnique (var, "xyz");
    ref.push_back ("xyz");
    CPPUNIT_ASSERT(var == ref);

    ts::AppendUnique (var, "abcd");
    CPPUNIT_ASSERT(var == ref);

    ts::AppendUnique (var, "xyz");
    CPPUNIT_ASSERT(var == ref);

    ts::AppendUnique (var, "end");
    ref.push_back ("end");
    CPPUNIT_ASSERT(var == ref);
}

void StringUtilsTest::testAppendContainer()
{
    const char* arr1[] = {"ab", "cde", "", "fghi"};
    std::list <std::string> var;
    std::list <std::string> ref;

    var.push_back ("begin");

    ref.push_back ("begin");
    ref.push_back ("ab");
    ref.push_back ("cde");
    ref.push_back ("");
    ref.push_back ("fghi");

    CPPUNIT_ASSERT(ts::AppendContainer (var, 4, arr1) == ref);

    char* arr2[] = {(char*)"ab", (char*)"cde", (char*)"", (char*)"fghi"};

    var.clear();
    var.push_back ("begin");
    CPPUNIT_ASSERT(ts::AppendContainer (var, 4, arr2) == ref);
}

void StringUtilsTest::testAssignContainer()
{
    const char* arr1[] = {"ab", "cde", "", "fghi"};
    std::vector <std::string> var;
    std::vector <std::string> ref;

    var.push_back ("previous");

    ref.push_back("ab");
    ref.push_back("cde");
    ref.push_back("");
    ref.push_back("fghi");

    CPPUNIT_ASSERT(ts::AssignContainer (var, 4, arr1) == ref);

    char* arr2[] = {(char*)"ab", (char*)"cde", (char*)"", (char*)"fghi"};

    var.clear();
    var.push_back("other");
    CPPUNIT_ASSERT(ts::AssignContainer (var, 4, arr2) == ref);
}

void StringUtilsTest::testSplit()
{
    std::vector <std::string> v1;
    ts::SplitString(v1, "az, ,  fr,  ze ,t");
    CPPUNIT_ASSERT(v1.size() == 5);
    CPPUNIT_ASSERT(v1[0] == "az");
    CPPUNIT_ASSERT(v1[1] == "");
    CPPUNIT_ASSERT(v1[2] == "fr");
    CPPUNIT_ASSERT(v1[3] == "ze");
    CPPUNIT_ASSERT(v1[4] == "t");

    std::vector <std::string> v2;
    const std::string s2 ("az, ,  fr,  ze ,t");
    ts::SplitString(v2, s2);
    CPPUNIT_ASSERT(v2.size() == 5);
    CPPUNIT_ASSERT(v2[0] == "az");
    CPPUNIT_ASSERT(v2[1] == "");
    CPPUNIT_ASSERT(v2[2] == "fr");
    CPPUNIT_ASSERT(v2[3] == "ze");
    CPPUNIT_ASSERT(v2[4] == "t");

    std::vector <std::string> v3;
    ts::SplitString(v3, "az, ,  fr,  ze ,t", ',', false);
    CPPUNIT_ASSERT(v3.size() == 5);
    CPPUNIT_ASSERT(v3[0] == "az");
    CPPUNIT_ASSERT(v3[1] == " ");
    CPPUNIT_ASSERT(v3[2] == "  fr");
    CPPUNIT_ASSERT(v3[3] == "  ze ");
    CPPUNIT_ASSERT(v3[4] == "t");

    std::vector <std::string> v4;
    ts::SplitString(v4, "az, ,  fr,  ze ,t", 'z', false);
    CPPUNIT_ASSERT(v4.size() == 3);
    CPPUNIT_ASSERT(v4[0] == "a");
    CPPUNIT_ASSERT(v4[1] == ", ,  fr,  ");
    CPPUNIT_ASSERT(v4[2] == "e ,t");
}

void StringUtilsTest::testJoin()
{
    std::vector <std::string> v;
    v.push_back("az");
    v.push_back("sd");
    v.push_back("tg");
    CPPUNIT_ASSERT(ts::JoinStrings(v) == "az, sd, tg");
    CPPUNIT_ASSERT(ts::JoinStrings(++v.begin(), v.end()) == "sd, tg");
}

void StringUtilsTest::testBreakLines()
{
    std::vector <std::string> v1;
    ts::SplitLines(v1, "aze arf erf r+oih zf", 8);
    CPPUNIT_ASSERT(v1.size() == 3);
    CPPUNIT_ASSERT(v1[0] == "aze arf");
    CPPUNIT_ASSERT(v1[1] == "erf");
    CPPUNIT_ASSERT(v1[2] == "r+oih zf");

    std::vector <std::string> v2;
    ts::SplitLines(v2, "aze arf erf r+oih zf", 8, "+");
    CPPUNIT_ASSERT(v2.size() == 3);
    CPPUNIT_ASSERT(v2[0] == "aze arf");
    CPPUNIT_ASSERT(v2[1] == "erf r+");
    CPPUNIT_ASSERT(v2[2] == "oih zf");

    std::vector <std::string> v3;
    ts::SplitLines(v3, "aze arf erf r+oih zf", 8, "", "==");
    CPPUNIT_ASSERT(v3.size() == 4);
    CPPUNIT_ASSERT(v3[0] == "aze arf");
    CPPUNIT_ASSERT(v3[1] == "==erf");
    CPPUNIT_ASSERT(v3[2] == "==r+oih");
    CPPUNIT_ASSERT(v3[3] == "==zf");

    std::vector <std::string> v4;
    ts::SplitLines(v4, "aze arf dkvyfngofnb ff", 8);
    CPPUNIT_ASSERT(v4.size() == 3);
    CPPUNIT_ASSERT(v4[0] == "aze arf");
    CPPUNIT_ASSERT(v4[1] == "dkvyfngofnb");
    CPPUNIT_ASSERT(v4[2] == "ff");

    std::vector <std::string> v5;
    ts::SplitLines(v5, "aze arf dkvyfngofnb ff", 8, "", "", true);
    CPPUNIT_ASSERT(v5.size() == 3);
    CPPUNIT_ASSERT(v5[0] == "aze arf");
    CPPUNIT_ASSERT(v5[1] == "dkvyfngo");
    CPPUNIT_ASSERT(v5[2] == "fnb ff");
}

void StringUtilsTest::testJustifyLeft()
{
    CPPUNIT_ASSERT(ts::JustifyLeft ("abc", 8) == "abc     ");
    CPPUNIT_ASSERT(ts::JustifyLeft ("abc", 8, '.') == "abc.....");
    CPPUNIT_ASSERT(ts::JustifyLeft ("abcdefghij", 8) == "abcdefghij");
    CPPUNIT_ASSERT(ts::JustifyLeft ("abcdefghij", 8, ' ', true) == "abcdefgh");
}

void StringUtilsTest::testJustifyRight()
{
    CPPUNIT_ASSERT(ts::JustifyRight ("abc", 8) == "     abc");
    CPPUNIT_ASSERT(ts::JustifyRight ("abc", 8, '.') == ".....abc");
    CPPUNIT_ASSERT(ts::JustifyRight ("abcdefghij", 8) == "abcdefghij");
    CPPUNIT_ASSERT(ts::JustifyRight ("abcdefghij", 8, ' ', true) == "cdefghij");
}

void StringUtilsTest::testJustifyCentered()
{
    CPPUNIT_ASSERT(ts::JustifyCentered ("abc", 8) == "  abc   ");
    CPPUNIT_ASSERT(ts::JustifyCentered ("abc", 8, '.') == "..abc...");
    CPPUNIT_ASSERT(ts::JustifyCentered ("abcdefghij", 8) == "abcdefghij");
    CPPUNIT_ASSERT(ts::JustifyCentered ("abcdefghij", 8, ' ', true) == "abcdefgh");
}

void StringUtilsTest::testJustify()
{
    CPPUNIT_ASSERT(ts::Justify ("abc", "def", 8) == "abc  def");
    CPPUNIT_ASSERT(ts::Justify ("abc", "def", 8, '.') == "abc..def");
    CPPUNIT_ASSERT(ts::Justify ("abcd", "efgh", 8) == "abcdefgh");
    CPPUNIT_ASSERT(ts::Justify ("abcde", "fghij", 8) == "abcdefghij");
}

void StringUtilsTest::testYesNo()
{
    CPPUNIT_ASSERT(std::string (ts::YesNo (true)) == "yes");
    CPPUNIT_ASSERT(std::string (ts::YesNo (false)) == "no");
}

void StringUtilsTest::testTrueFalse()
{
    CPPUNIT_ASSERT(std::string (ts::TrueFalse (true)) == "true");
    CPPUNIT_ASSERT(std::string (ts::TrueFalse (false)) == "false");
}

void StringUtilsTest::testOnOff()
{
    CPPUNIT_ASSERT(std::string (ts::OnOff (true)) == "on");
    CPPUNIT_ASSERT(std::string (ts::OnOff (false)) == "off");
}

void StringUtilsTest::testSimilarStrings()
{
    CPPUNIT_ASSERT(ts::SimilarStrings ("", ""));
    CPPUNIT_ASSERT(ts::SimilarStrings ("aZer tY", "  AZE R T Y    "));
    CPPUNIT_ASSERT(ts::SimilarStrings ("  AZE R T Y    ", "aZer tY"));
    CPPUNIT_ASSERT(!ts::SimilarStrings ("", "az"));
    CPPUNIT_ASSERT(!ts::SimilarStrings ("az", ""));
}

void StringUtilsTest::testFormat()
{
    CPPUNIT_ASSERT(ts::Format("test") == "test");
    CPPUNIT_ASSERT(ts::Format("a = %d", 1) == "a = 1");
    CPPUNIT_ASSERT(ts::Format("a = %" FMT_SIZE_T "d", size_t(1234)) == "a = 1234");
    CPPUNIT_ASSERT(ts::Format("a = %016" FMT_INT64 "X", uint64_t(TS_CONST64(0x0123456789ABCDEF))) == "a = 0123456789ABCDEF");
}

// Build a vector of UInt8 from a variable number of int arguments.
// Stop at the first negative number.
namespace {
    ts::ByteBlock UInt8Vector(int byte, ...)
    {
        ts::ByteBlock res;
        if (byte >= 0) {
            res.push_back(byte & 0xFF);
            va_list ap;
            va_start(ap, byte);
            for (;;) {
                const int b = va_arg(ap, int);
                if (b < 0) {
                    break;
                }
                else {
                    res.push_back(b & 0xFF);
                }
            }
            va_end(ap);
        }
        return res;
    }
}

void StringUtilsTest::testHexaDecode()
{
    ts::ByteBlock bytes;

    CPPUNIT_ASSERT(ts::HexaDecode(bytes, "0123456789ABCDEF"));
    CPPUNIT_ASSERT(bytes == UInt8Vector(0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, -1));

    CPPUNIT_ASSERT(ts::HexaDecode(bytes, " 0 1234 56 789 ABC DEF "));
    CPPUNIT_ASSERT(bytes == UInt8Vector(0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, -1));

    CPPUNIT_ASSERT(!ts::HexaDecode (bytes, " 0 1234 56 - 789 ABC DEF "));
    CPPUNIT_ASSERT(bytes == UInt8Vector(0x01, 0x23, 0x45, -1));

    CPPUNIT_ASSERT(!ts::HexaDecode(bytes, "X 0 1234 56 - 789 ABC DEF "));
    CPPUNIT_ASSERT(bytes.empty());
}

void StringUtilsTest::testHexa()
{
    const std::string hex1 (ts::Hexa (_bytes, 40));
    const char* ref1 =
        "00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19\n"
        "1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27\n";
    CPPUNIT_ASSERT(hex1 == ref1);

    const std::string hex2 (ts::Hexa (_bytes, 40, ts::hexa::HEXA | ts::hexa::ASCII));
    const char* ref2 =
        "00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11  ..................\n"
        "12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23  .............. !\"#\n"
        "24 25 26 27                                            $%&'\n";
    CPPUNIT_ASSERT(hex2 == ref2);

    const std::string hex3 (ts::Hexa (_bytes + 32,
                                          40,
                                          ts::hexa::HEXA | ts::hexa::ASCII | ts::hexa::OFFSET,
                                          4,     // indent
                                          50,    // lineWidth
                                          32));  // initOffset
    const char* ref3 =
        "    0020:  20 21 22 23 24 25 26 27   !\"#$%&'\n"
        "    0028:  28 29 2A 2B 2C 2D 2E 2F  ()*+,-./\n"
        "    0030:  30 31 32 33 34 35 36 37  01234567\n"
        "    0038:  38 39 3A 3B 3C 3D 3E 3F  89:;<=>?\n"
        "    0040:  40 41 42 43 44 45 46 47  @ABCDEFG\n";
    CPPUNIT_ASSERT(hex3 == ref3);

    const std::string hex4 (ts::Hexa (_bytes + 32,
                                          22,
                                          ts::hexa::HEXA | ts::hexa::ASCII | ts::hexa::OFFSET | ts::hexa::BPL,
                                          4,     // indent
                                          10,    // lineWidth (in bytes)
                                          32));  // initOffset
    const char* ref4 =
        "    0020:  20 21 22 23 24 25 26 27 28 29   !\"#$%&'()\n"
        "    002A:  2A 2B 2C 2D 2E 2F 30 31 32 33  *+,-./0123\n"
        "    0034:  34 35                          45\n";
    CPPUNIT_ASSERT(hex4 == ref4);

    const std::string hex5 (ts::Hexa (_bytes + 32, 12, ts::hexa::SINGLE_LINE));
    const char* ref5 = "20 21 22 23 24 25 26 27 28 29 2A 2B";
    CPPUNIT_ASSERT(hex5 == ref5);

    const std::string hex6 (ts::Hexa (_bytes + 32, 20, ts::hexa::HEXA | ts::hexa::C_STYLE));
    const char* ref6 =
        "0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,\n"
        "0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33,\n";
    CPPUNIT_ASSERT(hex6 == ref6);

    const std::string hex7 (ts::Hexa (_bytes + 32, 10, ts::hexa::BINARY | ts::hexa::ASCII));
    const char* ref7 =
        "00100000 00100001 00100010 00100011 00100100 00100101   !\"#$%\n"
        "00100110 00100111 00101000 00101001                    &'()\n";
    CPPUNIT_ASSERT(hex7 == ref7);

    const std::string hex8 (ts::Hexa (_bytes + 32, 10, ts::hexa::BIN_NIBBLE | ts::hexa::ASCII));
    const char* ref8 =
        "0010.0000 0010.0001 0010.0010 0010.0011 0010.0100 0010.0101   !\"#$%\n"
        "0010.0110 0010.0111 0010.1000 0010.1001                      &'()\n";
    CPPUNIT_ASSERT(hex8 == ref8);
}

void StringUtilsTest::testDecimal()
{
    CPPUNIT_ASSERT_EQUAL(std::string("0"), ts::Decimal(0));
    CPPUNIT_ASSERT_EQUAL(std::string("0"), ts::Decimal(0L));
    CPPUNIT_ASSERT_EQUAL(std::string("0"), ts::Decimal(-0));
    CPPUNIT_ASSERT_EQUAL(std::string("0"), ts::Decimal(TS_CONST64(0)));
    CPPUNIT_ASSERT_EQUAL(std::string("1,234"), ts::Decimal(1234));
    CPPUNIT_ASSERT_EQUAL(std::string("     1,234"), ts::Decimal(1234, 10));
    CPPUNIT_ASSERT_EQUAL(std::string("     1,234"), ts::Decimal(1234, 10, true));
    CPPUNIT_ASSERT_EQUAL(std::string("1,234     "), ts::Decimal(1234, 10, false));
    CPPUNIT_ASSERT_EQUAL(std::string("      1234"), ts::Decimal(1234, 10, true, ""));
    CPPUNIT_ASSERT_EQUAL(std::string("  1()234()567()890"), ts::Decimal(1234567890, 18, true, "()"));
    CPPUNIT_ASSERT_EQUAL(std::string("    +1,234"), ts::Decimal(1234, 10, true, ",", true));
    CPPUNIT_ASSERT_EQUAL(std::string("    -1,234"), ts::Decimal(-1234, 10, true, ",", true));
    CPPUNIT_ASSERT_EQUAL(std::string("    -1,234"), ts::Decimal(-1234, 10, true, ",", false));
    CPPUNIT_ASSERT_EQUAL(std::string("-1,234,567,890,123,456"), ts::Decimal(TS_CONST64(-1234567890123456)));
}

void StringUtilsTest::testLoadSave()
{
    std::list<std::string> ref;
    for (int i = 1; i <= 20; ++i) {
        ref.push_back (ts::Format ("line %d", i));
    }
    CPPUNIT_ASSERT(ref.size() == 20);

    const std::string file1(newTemporaryFileName());
    CPPUNIT_ASSERT(ts::SaveStrings(ref, file1));

    std::list<std::string> load1;
    CPPUNIT_ASSERT(ts::LoadStrings(load1, file1));
    CPPUNIT_ASSERT(load1.size() == 20);
    CPPUNIT_ASSERT(load1 == ref);

    const std::list<std::string>::const_iterator refFirst = ++(ref.begin());
    const std::list<std::string>::const_iterator refLast = --(ref.end());

    const std::string file2(newTemporaryFileName());
    CPPUNIT_ASSERT(ts::SaveStrings(refFirst, refLast, file2));

    std::list<std::string> ref2 (refFirst, refLast);
    CPPUNIT_ASSERT(ref2.size() == 18);

    std::list<std::string> load2;
    CPPUNIT_ASSERT(ts::LoadStrings(load2, file2));
    CPPUNIT_ASSERT(load2.size() == 18);
    CPPUNIT_ASSERT(load2 == ref2);

    std::list<std::string> ref3;
    ref3.push_back("abcdef");
    ref3.insert (ref3.end(), refFirst, refLast);
    CPPUNIT_ASSERT(ref3.size() == 19);

    std::list<std::string> load3;
    load3.push_back("abcdef");
    CPPUNIT_ASSERT(ts::LoadAppendStrings(load3, file2));
    CPPUNIT_ASSERT(load3.size() == 19);
    CPPUNIT_ASSERT(load3 == ref3);
}

void StringUtilsTest::testToInteger()
{
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('0') == 0);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('9') == 9);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('a') == -1);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('f') == -1);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('z') == -1);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('a', 16) == 10);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('f', 16) == 15);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('z', 16) == -1);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('a', 36) == 10);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('f', 36) == 15);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('z', 36) == 35);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('A', 16) == 10);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('F', 16) == 15);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('Z', 16) == -1);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('A', 36) == 10);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('F', 36) == 15);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('Z', 36) == 35);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('?') == -1);
    CPPUNIT_ASSERT(ts::ToIntegerDigit ('?', 10, -2) == -2);

    int    i;
    uint32_t ui32;
    uint64_t ui64;
    int64_t  i64;

    CPPUNIT_ASSERT(ts::ToInteger(i, "1"));
    CPPUNIT_ASSERT(i == 1);

    CPPUNIT_ASSERT(ts::ToInteger(i, "-001"));
    CPPUNIT_ASSERT(i == -1);

    CPPUNIT_ASSERT(ts::ToInteger(i, "   -0xA0  "));
    CPPUNIT_ASSERT(i == -160);

    CPPUNIT_ASSERT(!ts::ToInteger(i, ""));
    CPPUNIT_ASSERT(i == 0);

    CPPUNIT_ASSERT(ts::ToInteger(ui32, "123"));
    CPPUNIT_ASSERT(ui32 == 123);

    CPPUNIT_ASSERT(!ts::ToInteger(ui32, "-123"));
    CPPUNIT_ASSERT(ui32 == 0);

    CPPUNIT_ASSERT(ts::ToInteger(ui64, "0"));
    CPPUNIT_ASSERT(ui64 == TS_UCONST64 (0));

    CPPUNIT_ASSERT(ts::ToInteger(ui64, "0xffffffffFFFFFFFF"));
    CPPUNIT_ASSERT(ui64 == TS_UCONST64 (0XFFFFFFFFFFFFFFFF));

    CPPUNIT_ASSERT(ts::ToInteger(ui64, "0x7fffffffFFFFFFFF"));
    CPPUNIT_ASSERT(ui64 == TS_UCONST64 (0X7FFFFFFFFFFFFFFF));

    CPPUNIT_ASSERT(ts::ToInteger(i64, "0"));
    CPPUNIT_ASSERT(i64 == TS_CONST64 (0));

    CPPUNIT_ASSERT(ts::ToInteger(i64, "0x7fffffffFFFFFFFF"));
    CPPUNIT_ASSERT(i64 == TS_CONST64 (0X7FFFFFFFFFFFFFFF));

    CPPUNIT_ASSERT(ts::ToInteger(i, " 12,345", ",."));
    CPPUNIT_ASSERT(i == 12345);

    CPPUNIT_ASSERT(ts::ToInteger(i, " -12.345", ",."));
    CPPUNIT_ASSERT(i == -12345);

    CPPUNIT_ASSERT(!ts::ToInteger(i, " -12;345", ",."));
    CPPUNIT_ASSERT(i == -12);

    std::list<int32_t> i32List;
    std::list<int32_t> i32Ref;

    i32Ref.clear();
    i32Ref.push_back (-12345);
    i32Ref.push_back (256);
    i32Ref.push_back (0);
    i32Ref.push_back (7);

    CPPUNIT_ASSERT(ts::ToIntegers(i32List, "-12345 0x100 0 7"));
    CPPUNIT_ASSERT(i32List == i32Ref);

    CPPUNIT_ASSERT(ts::ToIntegers(i32List, " , -12345    0x100 ,  0,  7  "));
    CPPUNIT_ASSERT(i32List == i32Ref);

    CPPUNIT_ASSERT(!ts::ToIntegers(i32List, " , -12345    0x100 ,  0,  7  xxx 45"));
    CPPUNIT_ASSERT(i32List == i32Ref);
}

void StringUtilsTest::testStart()
{
    CPPUNIT_ASSERT(ts::StartWith ("azertyuiop", "azer"));
    CPPUNIT_ASSERT(!ts::StartWith ("azertyuiop", "aZer"));
    CPPUNIT_ASSERT(!ts::StartWith ("azertyuiop", "azeR"));

    CPPUNIT_ASSERT(ts::StartWithInsensitive ("azertyuiop", "azer"));
    CPPUNIT_ASSERT(ts::StartWithInsensitive ("azertyuiop", "aZer"));
    CPPUNIT_ASSERT(ts::StartWithInsensitive ("azertyuiop", "azeR"));
    CPPUNIT_ASSERT(!ts::StartWithInsensitive ("azertyuiop", "azerq"));

    CPPUNIT_ASSERT(!ts::StartWith ("azertyuiop", 0));
    CPPUNIT_ASSERT(ts::StartWith ("azertyuiop", ""));
    CPPUNIT_ASSERT(!ts::StartWith ("azertyuiop", "azertyuiopqsdf"));

    CPPUNIT_ASSERT(!ts::StartWithInsensitive ("azertyuiop", 0));
    CPPUNIT_ASSERT(ts::StartWithInsensitive ("azertyuiop", ""));
    CPPUNIT_ASSERT(!ts::StartWithInsensitive ("azertyuiop", "azertyuiopqsdf"));

    CPPUNIT_ASSERT(!ts::StartWith ("", 0));
    CPPUNIT_ASSERT(ts::StartWith ("", ""));
    CPPUNIT_ASSERT(!ts::StartWith ("", "abcd"));

    CPPUNIT_ASSERT(!ts::StartWithInsensitive ("", 0));
    CPPUNIT_ASSERT(ts::StartWithInsensitive ("", ""));
    CPPUNIT_ASSERT(!ts::StartWithInsensitive ("", "abcd"));
}

void StringUtilsTest::testEnd()
{
    CPPUNIT_ASSERT(ts::EndWith ("azertyuiop", "uiop"));
    CPPUNIT_ASSERT(!ts::EndWith ("azertyuiop", "uiOp"));
    CPPUNIT_ASSERT(!ts::EndWith ("azertyuiop", "Uiop"));

    CPPUNIT_ASSERT(ts::EndWithInsensitive ("azertyuiop", "uiop"));
    CPPUNIT_ASSERT(ts::EndWithInsensitive ("azertyuiop", "uiOp"));
    CPPUNIT_ASSERT(ts::EndWithInsensitive ("azertyuiop", "Uiop"));
    CPPUNIT_ASSERT(!ts::EndWithInsensitive ("azertyuiop", "wuiop"));

    CPPUNIT_ASSERT(!ts::EndWith ("azertyuiop", 0));
    CPPUNIT_ASSERT(ts::EndWith ("azertyuiop", ""));
    CPPUNIT_ASSERT(!ts::EndWith ("azertyuiop", "qsazertyuiop"));

    CPPUNIT_ASSERT(!ts::EndWithInsensitive ("azertyuiop", 0));
    CPPUNIT_ASSERT(ts::EndWithInsensitive ("azertyuiop", ""));
    CPPUNIT_ASSERT(!ts::EndWithInsensitive ("azertyuiop", "qsazertyuiop"));

    CPPUNIT_ASSERT(!ts::EndWith ("", 0));
    CPPUNIT_ASSERT(ts::EndWith ("", ""));
    CPPUNIT_ASSERT(!ts::EndWith ("", "abcd"));

    CPPUNIT_ASSERT(!ts::EndWithInsensitive ("", 0));
    CPPUNIT_ASSERT(ts::EndWithInsensitive ("", ""));
    CPPUNIT_ASSERT(!ts::EndWithInsensitive ("", "abcd"));
}

void StringUtilsTest::testSubstitute()
{
    CPPUNIT_ASSERT(ts::ReturnSubstituteAll("", "", "") == "");
    CPPUNIT_ASSERT(ts::ReturnSubstituteAll("abcdefabcdef", "ab", "xyz") == "xyzcdefxyzcdef");
    CPPUNIT_ASSERT(ts::ReturnSubstituteAll("abcdefabcdef", "ef", "xyz") == "abcdxyzabcdxyz");
    CPPUNIT_ASSERT(ts::ReturnSubstituteAll("abcdba", "b", "bb") == "abbcdbba");
    CPPUNIT_ASSERT(ts::ReturnSubstituteAll("abcdefabcdef", "ef", "") == "abcdabcd");
}
