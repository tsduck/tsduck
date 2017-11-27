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
//  CppUnit test suite for class ts::Args
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsReportBuffer.h"
#include "tsVersion.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ArgsTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testAccessors();
    void testHelp();
    void testCopyOptions();
    void testValidCommandVariableArgs();
    void testValidCommandArgcArgv();
    void testValidCommandContainer();
    void testOptionalValue();
    void testThousandsSeparator();
    void testMissingParameter();
    void testTooManyParameters();
    void testAmbiguousOption();
    void testInvalidIntegerOption();
    void testIntegerTooLow();
    void testIntegerTooHigh();
    void testInvalidEnum();
    void testValidEnum();
    void testBitMask();
    void testGatherParameters();

    CPPUNIT_TEST_SUITE(ArgsTest);
    CPPUNIT_TEST(testAccessors);
    CPPUNIT_TEST(testHelp);
    CPPUNIT_TEST(testCopyOptions);
    CPPUNIT_TEST(testValidCommandVariableArgs);
    CPPUNIT_TEST(testValidCommandArgcArgv);
    CPPUNIT_TEST(testValidCommandContainer);
    CPPUNIT_TEST(testOptionalValue);
    CPPUNIT_TEST(testThousandsSeparator);
    CPPUNIT_TEST(testMissingParameter);
    CPPUNIT_TEST(testTooManyParameters);
    CPPUNIT_TEST(testAmbiguousOption);
    CPPUNIT_TEST(testInvalidIntegerOption);
    CPPUNIT_TEST(testIntegerTooLow);
    CPPUNIT_TEST(testIntegerTooHigh);
    CPPUNIT_TEST(testInvalidEnum);
    CPPUNIT_TEST(testValidEnum);
    CPPUNIT_TEST(testBitMask);
    CPPUNIT_TEST(testGatherParameters);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ArgsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ArgsTest::setUp()
{
}

// Test suite cleanup method.
void ArgsTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test case: basic accessors
void ArgsTest::testAccessors()
{
    ts::Args args(u"description", u"syntax", u"help", ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS);

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"description", args.getDescription());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"syntax", args.getSyntax());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"help", args.getHelp());
    CPPUNIT_ASSERT(args.getFlags() == (ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS));

    args.setDescription(u"description-1");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"description-1", args.getDescription());

    args.setSyntax(u"syntax-1");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"syntax-1", args.getSyntax());

    args.setHelp(u"help-1");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"help-1", args.getHelp());

    args.setShell(u"shell-1");
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"shell-1", args.getShell());

    args.setFlags(ts::Args::NO_EXIT_ON_ERROR);
    CPPUNIT_ASSERT(args.getFlags() == ts::Args::NO_EXIT_ON_ERROR);
}

// Test case: help text
void ArgsTest::testHelp()
{
    ts::ReportBuffer<> log;
    ts::Args args(u"{description}", u"{syntax}", u"{help}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION);
    args.redirectReport(&log);

    CPPUNIT_ASSERT(!args.analyze(u"test", {u"--help"}));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"\n"
                                  u"{description}\n"
                                  u"\n"
                                  u"Usage: test {syntax}\n"
                                  u"\n"
                                  u"{help}",
                                  log.getMessages());

    args.setShell(u"{shell}");
    log.resetMessages();
    CPPUNIT_ASSERT(!args.analyze(u"test", {u"--help"}));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"\n"
                                  u"{description}\n"
                                  u"\n"
                                  u"Usage: {shell} test {syntax}\n"
                                  u"\n"
                                  u"{help}",
                                  log.getMessages());

    log.resetMessages();
    CPPUNIT_ASSERT(!args.analyze(u"test", {u"--version=short"}));
    const ts::UString version(log.getMessages());
    const size_t dash = version.find(u'-');
    CPPUNIT_ASSERT(dash != ts::UString::NPOS);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(TS_STRINGIFY(TS_VERSION_MAJOR) "." TS_STRINGIFY(TS_VERSION_MINOR), version.substr(0, dash));
}

// Test case: copy options
void ArgsTest::testCopyOptions()
{
    ts::ReportBuffer<> log;
    ts::Args args1(u"{description1}", u"{syntax1}", u"{help1}", ts::Args::NO_EXIT_ON_ERROR);
    ts::Args args2(u"{description2}", u"{syntax2}", u"{help2}", ts::Args::NO_EXIT_ON_ERROR);

    args1.redirectReport(&log);
    args2.redirectReport(&log);

    args1.option(u"opt1");
    args1.option(u"opt2", u'o', ts::Args::UNSIGNED);

    CPPUNIT_ASSERT(args1.analyze(u"test", {u"--opt1", u"--opt2", u"1"}));
    CPPUNIT_ASSERT(!args2.analyze(u"test", {u"--opt1", u"--opt2", u"1"}));

    args2.copyOptions(args1, false);
    CPPUNIT_ASSERT(args2.analyze(u"test", {u"--opt1", u"--opt2", u"1"}));
}

// An Args class with a defined syntax
namespace {
    class TestArgs: public ts::Args
    {
    public:
        explicit TestArgs(ts::Report* log) :
            ts::Args("{description}", u"{syntax}", u"{help}", ts::Args::NO_EXIT_ON_ERROR)
        {
            redirectReport(log);
            option(u"",      0,  ts::Args::STRING, 1, 2);
            option(u"opt1",  0,  ts::Args::NONE);
            option(u"opt2", 'a', ts::Args::STRING);
            option(u"opt3",  0,  ts::Args::INTEGER, 0, ts::Args::UNLIMITED_COUNT, -4, 7);
            option(u"opt4",  0,  ts::Args::UNSIGNED, 0, 2);
            option(u"opt5", '5', ts::Args::POSITIVE, 0, ts::Args::UNLIMITED_COUNT);
            option(u"opt6", 'b', ts::Args::UINT8);
            option(u"opt7",  0,  ts::Args::UINT16);
            option(u"opt8",  0,  ts::Args::UINT32, 0, 0, 0, 0, true);
            option(u"opt9", 'c', ts::Enumeration({{"val1", 11}, {u"val2", 12}, {u"val3", 13}}));
            option(u"mask",  0,  ts::Enumeration({{"bit1", 0x01}, {u"bit2", 0x02}, {u"bit3", 0x04}}), 0, ts::Args::UNLIMITED_COUNT);
        }
    };
}

// Test case: analyze valid command, get option values, use analyze() with variable length argument list
void ArgsTest::testValidCommandVariableArgs()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(args.analyze(u"test", {u"--opt3", u"6", u"a", u"--opt1", u"b", u"--opt9", u"val2", u"--opt3", u"0", u"--opt3", u"6"}));

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"test", args.appName());
    CPPUNIT_ASSERT(args.count(u"") == 2);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"a", args.value(u"", u"", 0));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"b", args.value(u"", u"", 1));
    CPPUNIT_ASSERT(args.count(u"opt1") == 1);
    CPPUNIT_ASSERT(args.present(u"opt1"));
    CPPUNIT_ASSERT(args.count(u"opt2") == 0);
    CPPUNIT_ASSERT(!args.present(u"opt2"));
    CPPUNIT_ASSERT(args.count(u"opt3") == 3);
    CPPUNIT_ASSERT(args.present(u"opt3"));
    CPPUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 0) == 6);
    CPPUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 1) == 0);
    CPPUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 2) == 6);
    CPPUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 3) == -1);
    CPPUNIT_ASSERT(args.count(u"opt4") == 0);
    CPPUNIT_ASSERT(args.count(u"opt5") == 0);
    CPPUNIT_ASSERT(args.count(u"opt6") == 0);
    CPPUNIT_ASSERT(args.count(u"opt7") == 0);
    CPPUNIT_ASSERT(args.count(u"opt8") == 0);
    CPPUNIT_ASSERT(args.count(u"opt9") == 1);
    CPPUNIT_ASSERT(args.present(u"opt9"));
    CPPUNIT_ASSERT(args.intValue<int>(u"opt9") == 12);

    ts::UString s;
    args.getValue(s, u"", u"x", 0);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"a", s);
    args.getValue(s, u"", u"x", 1);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"b", s);
    args.getValue(s, u"", u"x", 2);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"x", s);

    ts::UStringVector vs;
    args.getValues(vs, u"");
    ts::UStringVector ref;
    ref.push_back("a");
    ref.push_back("b");
    CPPUNIT_ASSERT(vs == ref);

    int i;
    args.getIntValue(i, u"opt3", -1, 0);
    CPPUNIT_ASSERT(i == 6);
    args.getIntValue(i, u"opt3", -1, 1);
    CPPUNIT_ASSERT(i == 0);
    args.getIntValue(i, u"opt3", -1, 2);
    CPPUNIT_ASSERT(i == 6);
    args.getIntValue(i, u"opt3", -1, 3);
    CPPUNIT_ASSERT(i == -1);

    std::vector<int> vi;
    args.getIntValues(vi, u"opt3");
    std::vector<int> iref;
    iref.push_back(6);
    iref.push_back(0);
    iref.push_back(6);
    CPPUNIT_ASSERT(vi == iref);

    std::set<int> vis;
    args.getIntValues(vis, u"opt3");
    std::set<int> isref;
    isref.insert(0);
    isref.insert(6);
    CPPUNIT_ASSERT(isref == vis);
}

// Test case: analyze valid command, get option values, use analyze() with argc, argv parameters.
void ArgsTest::testValidCommandArgcArgv()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    char* argv[] = {
        const_cast<char*>("test"),
        const_cast<char*>("--opt3"),
        const_cast<char*>("6"),
        const_cast<char*>("a"),
        const_cast<char*>("--opt1"),
        const_cast<char*>("b"),
        const_cast<char*>("--opt9"),
        const_cast<char*>("val2"),
        const_cast<char*>("--opt3"),
        const_cast<char*>("0")};
    const int argc = 10;

    CPPUNIT_ASSERT(args.analyze(argc, argv));

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"test", args.appName());
    CPPUNIT_ASSERT(args.count(u"") == 2);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"a", args.value(u"", u"", 0));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"b", args.value(u"", u"", 1));
    CPPUNIT_ASSERT(args.count(u"opt1") == 1);
    CPPUNIT_ASSERT(args.present(u"opt1"));
    CPPUNIT_ASSERT(args.count(u"opt2") == 0);
    CPPUNIT_ASSERT(!args.present(u"opt2"));
    CPPUNIT_ASSERT(args.count(u"opt3") == 2);
    CPPUNIT_ASSERT(args.present(u"opt3"));
    CPPUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 0) == 6);
    CPPUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 1) == 0);
    CPPUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 2) == -1);
    CPPUNIT_ASSERT(args.count(u"opt4") == 0);
    CPPUNIT_ASSERT(args.count(u"opt5") == 0);
    CPPUNIT_ASSERT(args.count(u"opt6") == 0);
    CPPUNIT_ASSERT(args.count(u"opt7") == 0);
    CPPUNIT_ASSERT(args.count(u"opt8") == 0);
    CPPUNIT_ASSERT(args.count(u"opt9") == 1);
    CPPUNIT_ASSERT(args.present(u"opt9"));
    CPPUNIT_ASSERT(args.intValue<int>(u"opt9") == 12);

    ts::UString s;
    args.getValue(s, u"", u"x", 0);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"a", s);
    args.getValue(s, u"", u"x", 1);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"b", s);
    args.getValue(s, u"", u"x", 2);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"x", s);

    ts::UStringVector vs;
    args.getValues(vs, u"");
    ts::UStringVector ref;
    ref.push_back("a");
    ref.push_back("b");
    CPPUNIT_ASSERT(vs == ref);

    int i;
    args.getIntValue(i, u"opt3", -1, 0);
    CPPUNIT_ASSERT(i == 6);
    args.getIntValue(i, u"opt3", -1, 1);
    CPPUNIT_ASSERT(i == 0);
    args.getIntValue(i, u"opt3", -1, 2);
    CPPUNIT_ASSERT(i == -1);

    std::vector<int> vi;
    args.getIntValues(vi, u"opt3");
    std::vector<int> iref;
    iref.push_back(6);
    iref.push_back(0);
    CPPUNIT_ASSERT(vi == iref);
}

// Test case: analyze valid command, get option values, use analyze() with container of arguments
void ArgsTest::testValidCommandContainer()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    ts::UStringVector arguments;
    arguments.push_back(u"--opt3");
    arguments.push_back(u"6");
    arguments.push_back(u"a");
    arguments.push_back(u"--opt1");
    arguments.push_back(u"b");
    arguments.push_back(u"--opt9");
    arguments.push_back(u"val2");
    arguments.push_back(u"--opt3");
    arguments.push_back(u"0");

    CPPUNIT_ASSERT(args.analyze(u"test", {arguments}));

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"test", args.appName());
    CPPUNIT_ASSERT(args.count(u"") == 2);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"a", args.value(u"", u"", 0));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"b", args.value(u"", u"", 1));
    CPPUNIT_ASSERT(args.count(u"opt1") == 1);
    CPPUNIT_ASSERT(args.present(u"opt1"));
    CPPUNIT_ASSERT(args.count(u"opt2") == 0);
    CPPUNIT_ASSERT(!args.present(u"opt2"));
    CPPUNIT_ASSERT(args.count(u"opt3") == 2);
    CPPUNIT_ASSERT(args.present(u"opt3"));
    CPPUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 0) == 6);
    CPPUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 1) == 0);
    CPPUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 2) == -1);
    CPPUNIT_ASSERT(args.count(u"opt4") == 0);
    CPPUNIT_ASSERT(args.count(u"opt5") == 0);
    CPPUNIT_ASSERT(args.count(u"opt6") == 0);
    CPPUNIT_ASSERT(args.count(u"opt7") == 0);
    CPPUNIT_ASSERT(args.count(u"opt8") == 0);
    CPPUNIT_ASSERT(args.count(u"opt9") == 1);
    CPPUNIT_ASSERT(args.present(u"opt9"));
    CPPUNIT_ASSERT(args.intValue<int>(u"opt9") == 12);

    ts::UString s;
    args.getValue(s, u"", u"x", 0);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"a", s);
    args.getValue(s, u"", u"x", 1);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"b", s);
    args.getValue(s, u"", u"x", 2);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"x", s);

    ts::UStringVector vs;
    args.getValues(vs, u"");
    ts::UStringVector ref;
    ref.push_back("a");
    ref.push_back("b");
    CPPUNIT_ASSERT(vs == ref);

    int i;
    args.getIntValue(i, u"opt3", -1, 0);
    CPPUNIT_ASSERT(i == 6);
    args.getIntValue(i, u"opt3", -1, 1);
    CPPUNIT_ASSERT(i == 0);
    args.getIntValue(i, u"opt3", -1, 2);
    CPPUNIT_ASSERT(i == -1);

    std::vector<int> vi;
    args.getIntValues(vi, u"opt3");
    std::vector<int> iref;
    iref.push_back(6);
    iref.push_back(0);
    CPPUNIT_ASSERT(vi == iref);
}

// Test case: presence of thousands separator
void ArgsTest::testThousandsSeparator()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(args.analyze(u"test", {u"a", u"-5", u"2000", u"--opt5=3,000", u"-50x4.000", u"-5", u"80 000", u"-5", u"2.000 000"}));
    CPPUNIT_ASSERT_EQUAL(size_t(5), args.count(u"opt5"));
    CPPUNIT_ASSERT_EQUAL(2000, args.intValue<int>(u"opt5", 0, 0));
    CPPUNIT_ASSERT_EQUAL(3000, args.intValue<int>(u"opt5", 0, 1));
    CPPUNIT_ASSERT_EQUAL(0x4000, args.intValue<int>(u"opt5", 0, 2));
    CPPUNIT_ASSERT_EQUAL(80000, args.intValue<int>(u"opt5", 0, 3));
    CPPUNIT_ASSERT_EQUAL(2000000, args.intValue<int>(u"opt5", 0, 4));
}


// Test case: syntax of optional values
void ArgsTest::testOptionalValue()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(args.analyze(u"test", {u"a", u"--opt8", u"2"}));
    CPPUNIT_ASSERT(args.count(u"") == 2);
    CPPUNIT_ASSERT(args.present(u"opt8"));
    CPPUNIT_ASSERT(args.intValue<uint32_t>(u"opt8") == 0);

    CPPUNIT_ASSERT(args.analyze(u"test", {u"a", u"--opt8=2"}));
    CPPUNIT_ASSERT(args.count(u"") == 1);
    CPPUNIT_ASSERT(args.present(u"opt8"));
    CPPUNIT_ASSERT(args.intValue<uint32_t>(u"opt8") == 2);
}

// Test case:
void ArgsTest::testMissingParameter()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(!args.analyze(u"test", {u"--opt1"}));
    utest::Out() << "ArgsTest: testMissingParameter: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Error: missing parameter", log.getMessages());
}

// Test case:
void ArgsTest::testTooManyParameters()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(!args.analyze(u"test", {u"a", u"b", u"c"}));
    utest::Out() << "ArgsTest: testTooManyParameters: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Error: too many parameter, 2 maximum", log.getMessages());
}

// Test case:
void ArgsTest::testAmbiguousOption()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(!args.analyze(u"test", {u"--opt", u"a", u"b"}));
    utest::Out() << "ArgsTest: testAmbiguousOption: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Error: ambiguous option --opt (--opt1, --opt2)", log.getMessages());
}

// Test case:
void ArgsTest::testInvalidIntegerOption()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(!args.analyze(u"test", {u"--opt3", u"x", u"a", u"b"}));
    utest::Out() << "ArgsTest: testInvalidIntegerOption: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Error: invalid integer value x for option --opt3", log.getMessages());
}

// Test case:
void ArgsTest::testIntegerTooLow()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(!args.analyze(u"test", {u"--opt3", u"-10", u"a", u"b"}));
    utest::Out() << "ArgsTest: testIntegerTooLow: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Error: value for option --opt3 must be >= -4", log.getMessages());
}

// Test case:
void ArgsTest::testIntegerTooHigh()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(!args.analyze(u"test", {u"--opt3", u"10", u"a", u"b"}));
    utest::Out() << "ArgsTest: testIntegerTooHigh: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Error: value for option --opt3 must be <= 7", log.getMessages());
}

// Test case:
void ArgsTest::testInvalidEnum()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(!args.analyze(u"test", {u"--opt9", u"x", u"a", u"b"}));
    utest::Out() << "ArgsTest: testInvalidEnum: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT_EQUAL(ts::UString("Error: invalid value x for option --opt9 (-c), use one of val1, val2, val3"), log.getMessages());
}

// Test case:
void ArgsTest::testValidEnum()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(args.analyze(u"test", {u"--opt9", u"0x20", u"a", u"b"}));
    CPPUNIT_ASSERT_EQUAL(32, args.intValue<int>(u"opt9"));
}

// Test case:
void ArgsTest::testBitMask()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(args.analyze(u"test", {u"a"}));
    CPPUNIT_ASSERT(args.bitMaskValue<int>(u"mask", 0x10) == 0x10);

    CPPUNIT_ASSERT(args.analyze(u"test", {u"--mask", u"bit1", u"a"}));
    CPPUNIT_ASSERT(args.bitMaskValue<int>(u"mask", 0x10) == 0x01);

    CPPUNIT_ASSERT(args.analyze(u"test", {u"--mask", u"bit2", u"--mask", u"bit3", u"a"}));
    CPPUNIT_ASSERT(args.bitMaskValue<int>(u"mask", 0x10) == 0x06);
}

// Test case: "gather parameters" option
void ArgsTest::testGatherParameters()
{
    ts::ReportBuffer<> log;
    ts::Args args("description", u"syntax", u"help", ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS);
    args.redirectReport(&log);

    args.option(u"");
    args.option(u"opt1");
    args.option(u"opt2", 'o', ts::Args::UNSIGNED);

    CPPUNIT_ASSERT(args.analyze(u"test", {u"--opt1", u"--opt2", u"11", u"12", u"--opt2", u"13"}));
    CPPUNIT_ASSERT(args.valid());
    CPPUNIT_ASSERT(args.count(u"opt1") == 1);
    CPPUNIT_ASSERT(args.count(u"opt2") == 1);
    CPPUNIT_ASSERT(args.count(u"") == 3);
    CPPUNIT_ASSERT(args.intValue<int>(u"opt2") == 11);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"12", args.value(u"", u"", 0));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"--opt2", args.value(u"", u"", 1));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"13", args.value(u"", u"", 2));

    CPPUNIT_ASSERT(args.valid());
    args.invalidate();
    CPPUNIT_ASSERT(!args.valid());
}
