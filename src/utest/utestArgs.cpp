//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  TSUnit test suite for class ts::Args
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsReportBuffer.h"
#include "tsNullReport.h"
#include "tsFileUtils.h"
#include "tsFixedPoint.h"
#include "tsFraction.h"
#include "tsFloatingPoint.h"
#include "tsVersion.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ArgsTest: public tsunit::Test
{
public:
    ArgsTest();

    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testAccessors();
    void testHelpDefault();
    void testCopyOptions();
    void testHelpCustom();
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
    void testRedirection();
    void testTristate();
    void testRanges();
    void testDecimals();
    void testFixedPoint();
    void testFraction();
    void testDouble();
    void testInvalidFraction();
    void testInvalidDouble();

    TSUNIT_TEST_BEGIN(ArgsTest);
    TSUNIT_TEST(testAccessors);
    TSUNIT_TEST(testHelpDefault);
    TSUNIT_TEST(testCopyOptions);
    TSUNIT_TEST(testHelpCustom);
    TSUNIT_TEST(testValidCommandVariableArgs);
    TSUNIT_TEST(testValidCommandArgcArgv);
    TSUNIT_TEST(testValidCommandContainer);
    TSUNIT_TEST(testOptionalValue);
    TSUNIT_TEST(testThousandsSeparator);
    TSUNIT_TEST(testMissingParameter);
    TSUNIT_TEST(testTooManyParameters);
    TSUNIT_TEST(testAmbiguousOption);
    TSUNIT_TEST(testInvalidIntegerOption);
    TSUNIT_TEST(testIntegerTooLow);
    TSUNIT_TEST(testIntegerTooHigh);
    TSUNIT_TEST(testInvalidEnum);
    TSUNIT_TEST(testValidEnum);
    TSUNIT_TEST(testBitMask);
    TSUNIT_TEST(testGatherParameters);
    TSUNIT_TEST(testRedirection);
    TSUNIT_TEST(testTristate);
    TSUNIT_TEST(testRanges);
    TSUNIT_TEST(testDecimals);
    TSUNIT_TEST(testFixedPoint);
    TSUNIT_TEST(testFraction);
    TSUNIT_TEST(testDouble);
    TSUNIT_TEST(testInvalidFraction);
    TSUNIT_TEST(testInvalidDouble);
    TSUNIT_TEST_END();

private:
    typedef ts::FloatingPoint<double> Double;
    typedef ts::UStringVector USV;
    ts::UString _tempFile1;
    ts::UString _tempFile2;
};

TSUNIT_REGISTER(ArgsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
ArgsTest::ArgsTest() :
    _tempFile1(),
    _tempFile2()
{
}

// Test suite initialization method.
void ArgsTest::beforeTest()
{
    _tempFile1 = ts::TempFile();
    _tempFile2 = ts::TempFile();
}

// Test suite cleanup method.
void ArgsTest::afterTest()
{
    ts::DeleteFile(_tempFile1, NULLREP);
    ts::DeleteFile(_tempFile2, NULLREP);
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test case: basic accessors
void ArgsTest::testAccessors()
{
    ts::Args args(u"description", u"syntax", ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS);

    TSUNIT_EQUAL(u"description", args.getDescription());
    TSUNIT_EQUAL(u"syntax", args.getSyntax());
    TSUNIT_EQUAL(ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS, args.getFlags());

    args.setDescription(u"description-1");
    TSUNIT_EQUAL(u"description-1", args.getDescription());

    args.setSyntax(u"syntax-1");
    TSUNIT_EQUAL(u"syntax-1", args.getSyntax());

    args.setShell(u"shell-1");
    TSUNIT_EQUAL(u"shell-1", args.getShell());

    args.setFlags(ts::Args::NO_EXIT_ON_ERROR);
    TSUNIT_EQUAL(int(ts::Args::NO_EXIT_ON_ERROR), args.getFlags());
}

// Test case: help text with default options
void ArgsTest::testHelpDefault()
{
    ts::ReportBuffer<> log;
    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);
    args.redirectReport(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", USV({u"--help"})));
    TSUNIT_EQUAL(u"\n"
                 u"{description}\n"
                 u"\n"
                 u"Usage: test {syntax}\n"
                 u"\n"
                 u"Options:\n"
                 u"\n"
                 u"  -d[level]\n"
                 u"  --debug[=level]\n"
                 u"      Produce debug traces. The default level is 1. Higher levels produce more\n"
                 u"      messages.\n"
                 u"\n"
                 u"  --help\n"
                 u"      Display this help text.\n"
                 u"\n"
                 u"  -v\n"
                 u"  --verbose\n"
                 u"      Produce verbose output.\n"
                 u"\n"
                 u"  --version\n"
                 u"      Display the TSDuck version number.\n",
                 log.getMessages());

    args.setShell(u"{shell}");
    log.resetMessages();
    TSUNIT_ASSERT(!args.analyze(u"test", USV({u"--help"})));
    TSUNIT_EQUAL(u"\n"
                 u"{description}\n"
                 u"\n"
                 u"Usage: {shell} test {syntax}\n"
                 u"\n"
                 u"Options:\n"
                 u"\n"
                 u"  -d[level]\n"
                 u"  --debug[=level]\n"
                 u"      Produce debug traces. The default level is 1. Higher levels produce more\n"
                 u"      messages.\n"
                 u"\n"
                 u"  --help\n"
                 u"      Display this help text.\n"
                 u"\n"
                 u"  -v\n"
                 u"  --verbose\n"
                 u"      Produce verbose output.\n"
                 u"\n"
                 u"  --version\n"
                 u"      Display the TSDuck version number.\n",
                 log.getMessages());

    log.resetMessages();
    TSUNIT_ASSERT(!args.analyze(u"test", USV({u"--version=short"})));
    const ts::UString version(log.getMessages());
    debug() << "ArgsTest::testHelpDefault: version = \"" << version << "\"" << std::endl;
    const size_t dash = version.find(u'-');
    TSUNIT_ASSERT(dash != ts::NPOS);
    TSUNIT_EQUAL(TS_USTRINGIFY(TS_VERSION_MAJOR) u"." TS_USTRINGIFY(TS_VERSION_MINOR), version.substr(0, dash));
}

// Test case: copy options
void ArgsTest::testCopyOptions()
{
    ts::ReportBuffer<> log;
    ts::Args args1(u"{description1}", u"{syntax1}", ts::Args::NO_EXIT_ON_ERROR);
    ts::Args args2(u"{description2}", u"{syntax2}", ts::Args::NO_EXIT_ON_ERROR);

    args1.redirectReport(&log);
    args2.redirectReport(&log);

    args1.option(u"opt1");
    args1.option(u"opt2", u'o', ts::Args::UNSIGNED);

    TSUNIT_ASSERT(args1.analyze(u"test", {u"--opt1", u"--opt2", u"1"}));
    TSUNIT_ASSERT(!args2.analyze(u"test", {u"--opt1", u"--opt2", u"1"}));

    args2.copyOptions(args1, false);
    TSUNIT_ASSERT(args2.analyze(u"test", {u"--opt1", u"--opt2", u"1"}));
}

// An Args class with a defined syntax
namespace {
    class TestArgs: public ts::Args
    {
    public:
        explicit TestArgs(ts::Report* log) :
            ts::Args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS)
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
            option(u"opt9", 'c', ts::Enumeration({{u"val1", 11}, {u"val2", 12}, {u"val3", 13}}));
            option(u"mask",  0,  ts::Enumeration({{u"bit1", 0x01}, {u"bit2", 0x02}, {u"bit3", 0x04}}), 0, ts::Args::UNLIMITED_COUNT);
            option(u"opt10", 0,  ts::Args::UNSIGNED, 0, ts::Args::UNLIMITED_COUNT, 0, 0, false, 3);

            help(u"", u"The parameters");
            help(u"opt1", u"No value.");
            help(u"opt2", u"String value.");
            help(u"opt3", u"Integer from -4 to 7, unlimited count.");
            help(u"opt4", u"Integer from 0 to 2.");
            help(u"opt5", u"Positive integer, unlimited count.");
            help(u"opt6", u"Unsigned int, 8 bits.");
            help(u"opt7", u"Unsigned int, 16 bits.");
            help(u"opt8", u"Unsigned int, 32 bits, optional value.");
            help(u"opt9", u"Enumeration.");
            help(u"mask", u"Enumeration, unlimited count.");
            help(u"opt10", u"Unsigned int 3 decimal digits.");
        }
    };
}

// Test case: help text of a custom commmand.
void ArgsTest::testHelpCustom()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", USV({u"--help"})));
    TSUNIT_EQUAL(u"\n"
        u"{description}\n"
        u"\n"
        u"Usage: test {syntax}\n"
        u"\n"
        u"Parameters:\n"
        u"\n"
        u"  The parameters\n"
        u"\n"
        u"Options:\n"
        u"\n"
        u"  -d[level]\n"
        u"  --debug[=level]\n"
        u"      Produce debug traces. The default level is 1. Higher levels produce more\n"
        u"      messages.\n"
        u"\n"
        u"  --help\n"
        u"      Display this help text.\n"
        u"\n"
        u"  --mask value\n"
        u"      Enumeration, unlimited count.\n"
        u"      Must be one of \"bit1\", \"bit2\", \"bit3\".\n"
        u"\n"
        u"  --opt1\n"
        u"      No value.\n"
        u"\n"
        u"  --opt10 value\n"
        u"      Unsigned int 3 decimal digits.\n"
        u"      The value may include up to 3 meaningful decimal digits.\n"
        u"\n"
        u"  -a value\n"
        u"  --opt2 value\n"
        u"      String value.\n"
        u"\n"
        u"  --opt3 value\n"
        u"      Integer from -4 to 7, unlimited count.\n"
        u"\n"
        u"  --opt4 value\n"
        u"      Integer from 0 to 2.\n"
        u"\n"
        u"  -5 value\n"
        u"  --opt5 value\n"
        u"      Positive integer, unlimited count.\n"
        u"\n"
        u"  -b value\n"
        u"  --opt6 value\n"
        u"      Unsigned int, 8 bits.\n"
        u"\n"
        u"  --opt7 value\n"
        u"      Unsigned int, 16 bits.\n"
        u"\n"
        u"  --opt8[=value]\n"
        u"      Unsigned int, 32 bits, optional value.\n"
        u"\n"
        u"  -c value\n"
        u"  --opt9 value\n"
        u"      Enumeration.\n"
        u"      Must be one of \"val1\", \"val2\", \"val3\".\n"
        u"\n"
        u"  -v\n"
        u"  --verbose\n"
        u"      Produce verbose output.\n"
        u"\n"
        u"  --version\n"
        u"      Display the TSDuck version number.\n",
        log.getMessages());
}

// Test case: analyze valid command, get option values, use analyze() with variable length argument list
void ArgsTest::testValidCommandVariableArgs()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(args.analyze(u"test", {u"--opt3", u"6", u"a", u"--opt1", u"b", u"--opt9", u"val2", u"--opt3", u"0", u"--opt3", u"6"}));

    TSUNIT_EQUAL(u"test", args.appName());
    TSUNIT_EQUAL(2, args.count(u""));
    TSUNIT_EQUAL(u"a", args.value(u"", u"", 0));
    TSUNIT_EQUAL(u"b", args.value(u"", u"", 1));
    TSUNIT_EQUAL(1, args.count(u"opt1"));
    TSUNIT_ASSERT(args.present(u"opt1"));
    TSUNIT_EQUAL(0, args.count(u"opt2"));
    TSUNIT_ASSERT(!args.present(u"opt2"));
    TSUNIT_EQUAL(3, args.count(u"opt3"));
    TSUNIT_ASSERT(args.present(u"opt3"));
    TSUNIT_EQUAL(6, args.intValue<int>(u"opt3", -1, 0));
    TSUNIT_EQUAL(0, args.intValue<int>(u"opt3", -1, 1));
    TSUNIT_EQUAL(6, args.intValue<int>(u"opt3", -1, 2));
    TSUNIT_EQUAL(-1, args.intValue<int>(u"opt3", -1, 3));
    TSUNIT_EQUAL(0, args.count(u"opt4"));
    TSUNIT_EQUAL(0, args.count(u"opt5"));
    TSUNIT_EQUAL(0, args.count(u"opt6"));
    TSUNIT_EQUAL(0, args.count(u"opt7"));
    TSUNIT_EQUAL(0, args.count(u"opt8"));
    TSUNIT_EQUAL(1, args.count(u"opt9"));
    TSUNIT_ASSERT(args.present(u"opt9"));
    TSUNIT_EQUAL(12, args.intValue<int>(u"opt9"));

    ts::UString s;
    args.getValue(s, u"", u"x", 0);
    TSUNIT_EQUAL(u"a", s);
    args.getValue(s, u"", u"x", 1);
    TSUNIT_EQUAL(u"b", s);
    args.getValue(s, u"", u"x", 2);
    TSUNIT_EQUAL(u"x", s);

    ts::UStringVector vs;
    args.getValues(vs, u"");
    ts::UStringVector ref;
    ref.push_back(u"a");
    ref.push_back(u"b");
    TSUNIT_ASSERT(vs == ref);

    int i;
    args.getIntValue(i, u"opt3", -1, 0);
    TSUNIT_EQUAL(6, i);
    args.getIntValue(i, u"opt3", -1, 1);
    TSUNIT_EQUAL(0, i);
    args.getIntValue(i, u"opt3", -1, 2);
    TSUNIT_EQUAL(6, i);
    args.getIntValue(i, u"opt3", -1, 3);
    TSUNIT_EQUAL(-1, i);

    std::vector<int> vi;
    args.getIntValues(vi, u"opt3");
    std::vector<int> iref;
    iref.push_back(6);
    iref.push_back(0);
    iref.push_back(6);
    TSUNIT_ASSERT(vi == iref);

    std::set<int> vis;
    args.getIntValues(vis, u"opt3");
    std::set<int> isref;
    isref.insert(0);
    isref.insert(6);
    TSUNIT_ASSERT(isref == vis);
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

    TSUNIT_ASSERT(args.analyze(argc, argv));

    TSUNIT_EQUAL(u"test", args.appName());
    TSUNIT_EQUAL(2, args.count(u""));
    TSUNIT_EQUAL(u"a", args.value(u"", u"", 0));
    TSUNIT_EQUAL(u"b", args.value(u"", u"", 1));
    TSUNIT_EQUAL(1, args.count(u"opt1"));
    TSUNIT_ASSERT(args.present(u"opt1"));
    TSUNIT_EQUAL(0, args.count(u"opt2"));
    TSUNIT_ASSERT(!args.present(u"opt2"));
    TSUNIT_EQUAL(2, args.count(u"opt3"));
    TSUNIT_ASSERT(args.present(u"opt3"));
    TSUNIT_EQUAL(6, args.intValue<int>(u"opt3", -1, 0));
    TSUNIT_EQUAL(0, args.intValue<int>(u"opt3", -1, 1));
    TSUNIT_EQUAL(-1, args.intValue<int>(u"opt3", -1, 2));
    TSUNIT_EQUAL(0, args.count(u"opt4"));
    TSUNIT_EQUAL(0, args.count(u"opt5"));
    TSUNIT_EQUAL(0, args.count(u"opt6"));
    TSUNIT_EQUAL(0, args.count(u"opt7"));
    TSUNIT_EQUAL(0, args.count(u"opt8"));
    TSUNIT_EQUAL(1, args.count(u"opt9"));
    TSUNIT_ASSERT(args.present(u"opt9"));
    TSUNIT_EQUAL(12, args.intValue<int>(u"opt9"));

    ts::UString s;
    args.getValue(s, u"", u"x", 0);
    TSUNIT_EQUAL(u"a", s);
    args.getValue(s, u"", u"x", 1);
    TSUNIT_EQUAL(u"b", s);
    args.getValue(s, u"", u"x", 2);
    TSUNIT_EQUAL(u"x", s);

    ts::UStringVector vs;
    args.getValues(vs, u"");
    ts::UStringVector ref;
    ref.push_back(u"a");
    ref.push_back(u"b");
    TSUNIT_ASSERT(vs == ref);

    int i;
    args.getIntValue(i, u"opt3", -1, 0);
    TSUNIT_EQUAL(6, i);
    args.getIntValue(i, u"opt3", -1, 1);
    TSUNIT_EQUAL(0, i);
    args.getIntValue(i, u"opt3", -1, 2);
    TSUNIT_EQUAL(-1, i);

    std::vector<int> vi;
    args.getIntValues(vi, u"opt3");
    std::vector<int> iref;
    iref.push_back(6);
    iref.push_back(0);
    TSUNIT_ASSERT(vi == iref);
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

    TSUNIT_ASSERT(args.analyze(u"test", arguments));

    TSUNIT_EQUAL(u"test", args.appName());
    TSUNIT_ASSERT(args.count(u"") == 2);
    TSUNIT_EQUAL(u"a", args.value(u"", u"", 0));
    TSUNIT_EQUAL(u"b", args.value(u"", u"", 1));
    TSUNIT_ASSERT(args.count(u"opt1") == 1);
    TSUNIT_ASSERT(args.present(u"opt1"));
    TSUNIT_ASSERT(args.count(u"opt2") == 0);
    TSUNIT_ASSERT(!args.present(u"opt2"));
    TSUNIT_ASSERT(args.count(u"opt3") == 2);
    TSUNIT_ASSERT(args.present(u"opt3"));
    TSUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 0) == 6);
    TSUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 1) == 0);
    TSUNIT_ASSERT(args.intValue<int>(u"opt3", -1, 2) == -1);
    TSUNIT_ASSERT(args.count(u"opt4") == 0);
    TSUNIT_ASSERT(args.count(u"opt5") == 0);
    TSUNIT_ASSERT(args.count(u"opt6") == 0);
    TSUNIT_ASSERT(args.count(u"opt7") == 0);
    TSUNIT_ASSERT(args.count(u"opt8") == 0);
    TSUNIT_ASSERT(args.count(u"opt9") == 1);
    TSUNIT_ASSERT(args.present(u"opt9"));
    TSUNIT_ASSERT(args.intValue<int>(u"opt9") == 12);

    ts::UString s;
    args.getValue(s, u"", u"x", 0);
    TSUNIT_EQUAL(u"a", s);
    args.getValue(s, u"", u"x", 1);
    TSUNIT_EQUAL(u"b", s);
    args.getValue(s, u"", u"x", 2);
    TSUNIT_EQUAL(u"x", s);

    ts::UStringVector vs;
    args.getValues(vs, u"");
    ts::UStringVector ref;
    ref.push_back(u"a");
    ref.push_back(u"b");
    TSUNIT_ASSERT(vs == ref);

    int i;
    args.getIntValue(i, u"opt3", -1, 0);
    TSUNIT_ASSERT(i == 6);
    args.getIntValue(i, u"opt3", -1, 1);
    TSUNIT_ASSERT(i == 0);
    args.getIntValue(i, u"opt3", -1, 2);
    TSUNIT_ASSERT(i == -1);

    std::vector<int> vi;
    args.getIntValues(vi, u"opt3");
    std::vector<int> iref;
    iref.push_back(6);
    iref.push_back(0);
    TSUNIT_ASSERT(vi == iref);
}

// Test case: presence of thousands separator
void ArgsTest::testThousandsSeparator()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(args.analyze(u"test", {u"a", u"-5", u"2000", u"--opt5=3,000", u"-50x4,000", u"-5", u"80 000", u"-5", u"2,000 000"}));
    TSUNIT_EQUAL(5, args.count(u"opt5"));
    TSUNIT_EQUAL(2000, args.intValue<int>(u"opt5", 0, 0));
    TSUNIT_EQUAL(3000, args.intValue<int>(u"opt5", 0, 1));
    TSUNIT_EQUAL(0x4000, args.intValue<int>(u"opt5", 0, 2));
    TSUNIT_EQUAL(80000, args.intValue<int>(u"opt5", 0, 3));
    TSUNIT_EQUAL(2000000, args.intValue<int>(u"opt5", 0, 4));
}


// Test case: syntax of optional values
void ArgsTest::testOptionalValue()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(args.analyze(u"test", {u"a", u"--opt8", u"2"}));
    TSUNIT_ASSERT(args.count(u"") == 2);
    TSUNIT_ASSERT(args.present(u"opt8"));
    TSUNIT_ASSERT(args.intValue<uint32_t>(u"opt8") == 0);

    TSUNIT_ASSERT(args.analyze(u"test", {u"a", u"--opt8=2"}));
    TSUNIT_ASSERT(args.count(u"") == 1);
    TSUNIT_ASSERT(args.present(u"opt8"));
    TSUNIT_ASSERT(args.intValue<uint32_t>(u"opt8") == 2);
}

// Test case:
void ArgsTest::testMissingParameter()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", USV({u"--opt1"})));
    debug() << "ArgsTest: testMissingParameter: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: missing parameter", log.getMessages());
}

// Test case:
void ArgsTest::testTooManyParameters()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", {u"a", u"b", u"c"}));
    debug() << "ArgsTest: testTooManyParameters: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: too many parameter, 2 maximum", log.getMessages());
}

// Test case:
void ArgsTest::testAmbiguousOption()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt", u"a", u"b"}));
    debug() << "ArgsTest: testAmbiguousOption: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: ambiguous option --opt (--opt1, --opt10)", log.getMessages());
}

// Test case:
void ArgsTest::testInvalidIntegerOption()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt3", u"x", u"a", u"b"}));
    debug() << "ArgsTest: testInvalidIntegerOption: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: invalid integer value x for option --opt3", log.getMessages());
}

// Test case:
void ArgsTest::testIntegerTooLow()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt3", u"-10", u"a", u"b"}));
    debug() << "ArgsTest: testIntegerTooLow: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: value for option --opt3 must be >= -4", log.getMessages());
}

// Test case:
void ArgsTest::testIntegerTooHigh()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test --opt3 10 a b"));
    debug() << "ArgsTest: testIntegerTooHigh: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: value for option --opt3 must be <= 7", log.getMessages());
}

// Test case:
void ArgsTest::testInvalidEnum()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test --opt9 x a b"));
    debug() << "ArgsTest: testInvalidEnum: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: invalid value x for option --opt9 (-c), use one of \"val1\", \"val2\", \"val3\"", log.getMessages());
}

// Test case:
void ArgsTest::testValidEnum()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(args.analyze(u"test", {u"--opt9", u"0x20", u"a", u"b"}));
    TSUNIT_EQUAL(32, args.intValue<int>(u"opt9"));
}

// Test case: bitmask of integer values.
void ArgsTest::testBitMask()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(args.analyze(u"test", USV({u"a"})));
    TSUNIT_EQUAL(0x10, args.bitMaskValue<int>(u"mask", 0x10));

    TSUNIT_ASSERT(args.analyze(u"test", {u"--mask", u"bit1", u"a"}));
    TSUNIT_EQUAL(0x01, args.bitMaskValue<int>(u"mask", 0x10));

    TSUNIT_ASSERT(args.analyze(u"test", {u"--mask", u"bit2", u"--mask", u"bit3", u"a"}));
    TSUNIT_EQUAL(0x06, args.bitMaskValue<int>(u"mask", 0x10));
}

// Test case: "gather parameters" option
void ArgsTest::testGatherParameters()
{
    ts::ReportBuffer<> log;
    ts::Args args(u"description", u"syntax", ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS);
    args.redirectReport(&log);

    args.option(u"");
    args.option(u"opt1");
    args.option(u"opt2", 'o', ts::Args::UNSIGNED);

    TSUNIT_ASSERT(args.analyze(u"test", {u"--opt1", u"--opt2", u"11", u"12", u"--opt2", u"13"}));
    TSUNIT_ASSERT(args.valid());
    TSUNIT_ASSERT(args.count(u"opt1") == 1);
    TSUNIT_ASSERT(args.count(u"opt2") == 1);
    TSUNIT_ASSERT(args.count(u"") == 3);
    TSUNIT_ASSERT(args.intValue<int>(u"opt2") == 11);
    TSUNIT_EQUAL(u"12", args.value(u"", u"", 0));
    TSUNIT_EQUAL(u"--opt2", args.value(u"", u"", 1));
    TSUNIT_EQUAL(u"13", args.value(u"", u"", 2));

    TSUNIT_ASSERT(args.valid());
    args.invalidate();
    TSUNIT_ASSERT(!args.valid());
}

// Test case: redirect parameters from file
void ArgsTest::testRedirection()
{
    TestArgs args(&CERR);

    TSUNIT_ASSERT(ts::UString::Save(ts::UStringVector({u"param2", u"--opt1", u"--opt2", u"@@foo"}), _tempFile1));
    TSUNIT_ASSERT(ts::UString::Save(ts::UStringVector({u"--opt4", u"3", u"@" + _tempFile1}), _tempFile2));

    TSUNIT_ASSERT(args.analyze(u"test", {u"param1", u"@" + _tempFile2, u"--opt4", u"5"}));
    TSUNIT_ASSERT(args.present(u""));
    TSUNIT_ASSERT(args.present(u"opt1"));
    TSUNIT_ASSERT(args.present(u"opt2"));
    TSUNIT_ASSERT(!args.present(u"opt3"));
    TSUNIT_ASSERT(args.present(u"opt4"));

    TSUNIT_EQUAL(2, args.count(u""));
    TSUNIT_EQUAL(1, args.count(u"opt1"));
    TSUNIT_EQUAL(1, args.count(u"opt2"));
    TSUNIT_EQUAL(2, args.count(u"opt4"));
    TSUNIT_EQUAL(u"param1", args.value(u"", u"", 0));
    TSUNIT_EQUAL(u"param2", args.value(u"", u"", 1));
    TSUNIT_EQUAL(u"@foo", args.value(u"opt2"));
    TSUNIT_EQUAL(3, args.intValue<int>(u"opt4", 0, 0));
    TSUNIT_EQUAL(5, args.intValue<int>(u"opt4", 0, 1));
}

// Test case: tristate parameters.
void ArgsTest::testTristate()
{
    ts::Args args(u"description", u"syntax", ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS);
    args.option(u"opt1", 0, ts::Args::TRISTATE);
    args.option(u"opt2", 0, ts::Args::TRISTATE);
    args.option(u"opt3", 0, ts::Args::TRISTATE);
    args.option(u"opt4", 0, ts::Args::TRISTATE, 0, 1, -255, 256, true);
    args.option(u"opt5", 0, ts::Args::TRISTATE, 0, 1, -255, 256, true);
    args.option(u"opt6", 0, ts::Args::TRISTATE, 0, 1, -255, 256, true);
    args.option(u"opt7", 0, ts::Args::TRISTATE, 0, 1, -255, 256, true);
    args.option(u"opt8", 0, ts::Args::TRISTATE, 0, 1, -255, 256, true);

    TSUNIT_ASSERT(args.analyze(u"test", {u"--opt1", u"true", u"--opt2", u"no", u"--opt3", u"unknown", u"--opt4", u"--opt5=off", u"--opt6=yes", u"--opt7=maybe"}));

    TSUNIT_ASSERT(args.present(u"opt1"));
    TSUNIT_ASSERT(args.present(u"opt2"));
    TSUNIT_ASSERT(args.present(u"opt3"));
    TSUNIT_ASSERT(args.present(u"opt4"));
    TSUNIT_ASSERT(args.present(u"opt5"));
    TSUNIT_ASSERT(args.present(u"opt6"));
    TSUNIT_ASSERT(args.present(u"opt7"));
    TSUNIT_ASSERT(!args.present(u"opt8"));

    TSUNIT_EQUAL(ts::Tristate::True,  args.tristateValue(u"opt1"));
    TSUNIT_EQUAL(ts::Tristate::False, args.tristateValue(u"opt2"));
    TSUNIT_EQUAL(ts::Tristate::Maybe, args.tristateValue(u"opt3"));
    TSUNIT_EQUAL(ts::Tristate::True,  args.tristateValue(u"opt4"));
    TSUNIT_EQUAL(ts::Tristate::False, args.tristateValue(u"opt5"));
    TSUNIT_EQUAL(ts::Tristate::True,  args.tristateValue(u"opt6"));
    TSUNIT_EQUAL(ts::Tristate::Maybe, args.tristateValue(u"opt7"));
    TSUNIT_EQUAL(ts::Tristate::Maybe, args.tristateValue(u"opt8"));
}

// Test case: ranges of integer values.
void ArgsTest::testRanges()
{
    ts::Args args(u"description", u"syntax", ts::Args::NO_EXIT_ON_ERROR);
    args.option(u"opt1", 0, ts::Args::UINT8, 0, ts::Args::UNLIMITED_COUNT);
    args.option(u"opt2", 0, ts::Args::UINT8, 0, 3, 0, 100);
    args.option(u"opt3", 0, ts::Args::INTEGER, 0, ts::Args::UNLIMITED_COUNT, 0, ts::Args::UNLIMITED_VALUE, true);

    ts::ReportBuffer<> log;
    args.redirectReport(&log);

    TSUNIT_ASSERT(args.analyze(u"test", {u"--opt1", u"0", u"--opt1", u"1,0-0x00C", u"--opt1", u"4,7"}));
    TSUNIT_ASSERT(args.present(u"opt1"));
    TSUNIT_ASSERT(!args.present(u"opt2"));
    TSUNIT_ASSERT(!args.present(u"opt3"));
    TSUNIT_EQUAL(5, args.count(u"opt1"));
    TSUNIT_EQUAL(0,  args.intValue<int>(u"opt1", -1, 0));
    TSUNIT_EQUAL(10, args.intValue<int>(u"opt1", -1, 1));
    TSUNIT_EQUAL(11, args.intValue<int>(u"opt1", -1, 2));
    TSUNIT_EQUAL(12, args.intValue<int>(u"opt1", -1, 3));
    TSUNIT_EQUAL(47, args.intValue<int>(u"opt1", -1, 4));
    TSUNIT_EQUAL(-1, args.intValue<int>(u"opt1", -1, 5));

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt2", u"1", u"--opt2", u"10-12"}));
    TSUNIT_EQUAL(u"Error: too many option --opt2, 3 maximum", log.getMessages());

    TSUNIT_ASSERT(args.analyze(u"test", {u"--opt2", u"1", u"--opt2", u"10-11"}));
    TSUNIT_EQUAL(3, args.count(u"opt2"));
    TSUNIT_EQUAL(1,  args.intValue<int>(u"opt2", -1, 0));
    TSUNIT_EQUAL(10, args.intValue<int>(u"opt2", -1, 1));
    TSUNIT_EQUAL(11, args.intValue<int>(u"opt2", -1, 2));
    TSUNIT_EQUAL(-1, args.intValue<int>(u"opt2", -1, 3));

    TSUNIT_ASSERT(args.analyze(u"test", {u"--opt3=100,000", u"--opt3", u"--opt3=9000-9,003"}));
    TSUNIT_EQUAL(6, args.count(u"opt3"));
    TSUNIT_EQUAL(100000, args.intValue<int>(u"opt3", -1, 0));
    TSUNIT_EQUAL(-1,   args.intValue<int>(u"opt3", -1, 1));
    TSUNIT_EQUAL(9000, args.intValue<int>(u"opt3", -1, 2));
    TSUNIT_EQUAL(9001, args.intValue<int>(u"opt3", -1, 3));
    TSUNIT_EQUAL(9002, args.intValue<int>(u"opt3", -1, 4));
    TSUNIT_EQUAL(9003, args.intValue<int>(u"opt3", -1, 5));
    TSUNIT_EQUAL(-1,   args.intValue<int>(u"opt3", -1, 6));
}

// Test case: decimal values.
void ArgsTest::testDecimals()
{
    TestArgs args(&CERR);

    TSUNIT_ASSERT(args.analyze(u"test", {u"param", u"--opt10", u"34", u"--opt10", u"0.1", u"--opt10", u"2.3456789-3"}));
    TSUNIT_EQUAL(658,   args.count(u"opt10"));
    TSUNIT_EQUAL(34000, args.intValue<int>(u"opt10", 0, 0));
    TSUNIT_EQUAL(100,   args.intValue<int>(u"opt10", 0, 1));
    TSUNIT_EQUAL(2345,  args.intValue<int>(u"opt10", 0, 2));
    TSUNIT_EQUAL(2346,  args.intValue<int>(u"opt10", 0, 3));
    TSUNIT_EQUAL(3000,  args.intValue<int>(u"opt10", 0, 657));
}

// Test case: fixed point types.
void ArgsTest::testFixedPoint()
{
    typedef ts::FixedPoint<int32_t, 3> Fixed;

    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);
    args.redirectReport(&CERR);
    args.option<Fixed>(u"");

    TSUNIT_ASSERT(args.analyze(u"test", {u"34", u"0.1", u"12.345678"}));
    TSUNIT_EQUAL(3, args.count(u""));

    TSUNIT_EQUAL(34000, args.numValue<Fixed>(u"", 0, 0).raw());
    TSUNIT_EQUAL(34,    args.numValue<Fixed>(u"", 0, 0).toInt());

    TSUNIT_EQUAL(100,   args.numValue<Fixed>(u"", 0, 1).raw());
    TSUNIT_EQUAL(0,     args.numValue<Fixed>(u"", 0, 1).toInt());

    TSUNIT_EQUAL(12345, args.numValue<Fixed>(u"", 0, 2).raw());
    TSUNIT_EQUAL(12,    args.numValue<Fixed>(u"", 0, 2).toInt());
}

// Test case: fraction types.
void ArgsTest::testFraction()
{
    typedef ts::Fraction<int32_t> Frac;

    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);
    args.redirectReport(&CERR);
    args.option<Frac>(u"");

    TSUNIT_ASSERT(args.analyze(u"test", {u"1", u" -2", u"12/345", u" -6/12"}));
    TSUNIT_EQUAL(4, args.count(u""));

    TSUNIT_EQUAL(1, args.numValue<Frac>(u"", 0, 0).numerator());
    TSUNIT_EQUAL(1, args.numValue<Frac>(u"", 0, 0).denominator());

    TSUNIT_EQUAL(-2, args.numValue<Frac>(u"", 0, 1).numerator());
    TSUNIT_EQUAL(1, args.numValue<Frac>(u"", 0, 1).denominator());

    TSUNIT_EQUAL(4, args.numValue<Frac>(u"", 0, 2).numerator());
    TSUNIT_EQUAL(115, args.numValue<Frac>(u"", 0, 2).denominator());

    TSUNIT_EQUAL(-1, args.numValue<Frac>(u"", 0, 3).numerator());
    TSUNIT_EQUAL(2, args.numValue<Frac>(u"", 0, 3).denominator());

    TSUNIT_EQUAL(5, args.numValue<Frac>(u"", 5, 4).numerator());
    TSUNIT_EQUAL(1, args.numValue<Frac>(u"", 5, 4).denominator());

    TSUNIT_EQUAL(3, args.numValue<Frac>(u"", Frac(3, 4), 4).numerator());
    TSUNIT_EQUAL(4, args.numValue<Frac>(u"", Frac(3, 4), 4).denominator());
}

// Test case: floating-point types.
void ArgsTest::testDouble()
{
    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);
    args.redirectReport(&CERR);
    args.option<Double>(u"");

    TSUNIT_ASSERT(args.analyze(u"test", {u"1", u"2.56", u"0", u" -6.12"}));
    TSUNIT_EQUAL(4, args.count(u""));

    TSUNIT_EQUAL(1.0,   args.numValue<Double>(u"", 0, 0).toDouble());
    TSUNIT_EQUAL(2.56,  args.numValue<Double>(u"", 0, 1).toDouble());
    TSUNIT_EQUAL(0.0,   args.numValue<Double>(u"", 0, 2).toDouble());
    TSUNIT_EQUAL(-6.12, args.numValue<Double>(u"", 0, 3).toDouble());
}

// Test case:
void ArgsTest::testInvalidFraction()
{
    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);

    ts::ReportBuffer<> log;
    args.redirectReport(&log);

    typedef ts::Fraction<int32_t> Frac;
    args.option<Frac>(u"opt");

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt", u"foo"}));
    debug() << "ArgsTest: testInvalidFraction: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: invalid value foo for option --opt", log.getMessages());
}

// Test case:
void ArgsTest::testInvalidDouble()
{
    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);

    ts::ReportBuffer<> log;
    args.redirectReport(&log);
    args.option<Double>(u"opt", 0, 0, 1, 12, 15);

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt", u"2.3"}));
    debug() << "ArgsTest: testInvalidDouble: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: value for option --opt must be in range 12 to 15", log.getMessages());
}
