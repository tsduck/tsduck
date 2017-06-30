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

    CPPUNIT_TEST_SUITE (ArgsTest);
    CPPUNIT_TEST (testAccessors);
    CPPUNIT_TEST (testHelp);
    CPPUNIT_TEST (testCopyOptions);
    CPPUNIT_TEST (testValidCommandVariableArgs);
    CPPUNIT_TEST (testValidCommandArgcArgv);
    CPPUNIT_TEST (testValidCommandContainer);
    CPPUNIT_TEST (testOptionalValue);
    CPPUNIT_TEST (testThousandsSeparator);
    CPPUNIT_TEST (testMissingParameter);
    CPPUNIT_TEST (testTooManyParameters);
    CPPUNIT_TEST (testAmbiguousOption);
    CPPUNIT_TEST (testInvalidIntegerOption);
    CPPUNIT_TEST (testIntegerTooLow);
    CPPUNIT_TEST (testIntegerTooHigh);
    CPPUNIT_TEST (testInvalidEnum);
    CPPUNIT_TEST (testValidEnum);
    CPPUNIT_TEST (testBitMask);
    CPPUNIT_TEST (testGatherParameters);
    CPPUNIT_TEST_SUITE_END ();
};

CPPUNIT_TEST_SUITE_REGISTRATION (ArgsTest);


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
    ts::Args args ("description", "syntax", "help", ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS);

    CPPUNIT_ASSERT(args.getDescription() == "description");
    CPPUNIT_ASSERT(args.getSyntax() == "syntax");
    CPPUNIT_ASSERT(args.getHelp() == "help");
    CPPUNIT_ASSERT(args.getFlags() == (ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS));

    args.setDescription ("description-1");
    CPPUNIT_ASSERT(args.getDescription() == "description-1");

    args.setSyntax ("syntax-1");
    CPPUNIT_ASSERT(args.getSyntax() == "syntax-1");

    args.setHelp ("help-1");
    CPPUNIT_ASSERT(args.getHelp() == "help-1");

    args.setShell ("shell-1");
    CPPUNIT_ASSERT(args.getShell() == "shell-1");

    args.setFlags (ts::Args::NO_EXIT_ON_ERROR);
    CPPUNIT_ASSERT(args.getFlags() == ts::Args::NO_EXIT_ON_ERROR);
}

// Test case: help text
void ArgsTest::testHelp()
{
    ts::ReportBuffer<> log;
    ts::Args args("{description}", "{syntax}", "{help}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION);
    args.redirectReport(&log);

    CPPUNIT_ASSERT(!args.analyze("test", "--help", TS_NULL));
    CPPUNIT_ASSERT_EQUAL(std::string("\n"
                                     "{description}\n"
                                     "\n"
                                     "Usage: test {syntax}\n"
                                     "\n"
                                     "{help}"),
                         log.getMessages());

    args.setShell("{shell}");
    log.resetMessages();
    CPPUNIT_ASSERT(!args.analyze("test", "--help", TS_NULL));
    CPPUNIT_ASSERT_EQUAL(std::string("\n"
                                     "{description}\n"
                                     "\n"
                                     "Usage: {shell} test {syntax}\n"
                                     "\n"
                                     "{help}"),
                         log.getMessages());

    log.resetMessages();
    CPPUNIT_ASSERT(!args.analyze("test", "--version=short", TS_NULL));
    const std::string version(log.getMessages());
    const size_t dash = version.find('-');
    CPPUNIT_ASSERT(dash != std::string::npos);
    CPPUNIT_ASSERT_EQUAL(std::string(TS_STRINGIFY(TS_VERSION_MAJOR) "." TS_STRINGIFY(TS_VERSION_MINOR)), version.substr(0, dash));
}

// Test case: copy options
void ArgsTest::testCopyOptions()
{
    ts::ReportBuffer<> log;
    ts::Args args1("{description1}", "{syntax1}", "{help1}", ts::Args::NO_EXIT_ON_ERROR);
    ts::Args args2("{description2}", "{syntax2}", "{help2}", ts::Args::NO_EXIT_ON_ERROR);

    args1.redirectReport(&log);
    args2.redirectReport(&log);

    args1.option("opt1");
    args1.option("opt2", 'o', ts::Args::UNSIGNED);

    CPPUNIT_ASSERT(args1.analyze("test", "--opt1", "--opt2", "1", TS_NULL));
    CPPUNIT_ASSERT(!args2.analyze("test", "--opt1", "--opt2", "1", TS_NULL));

    args2.copyOptions(args1, false);
    CPPUNIT_ASSERT(args2.analyze("test", "--opt1", "--opt2", "1", TS_NULL));
}

// An Args class with a defined syntax
namespace {
    class TestArgs: public ts::Args
    {
    public:
        TestArgs(ts::ReportInterface* log) :
            ts::Args ("{description}", "{syntax}", "{help}", ts::Args::NO_EXIT_ON_ERROR)
        {
            redirectReport(log);
            option("",      0,  ts::Args::STRING, 1, 2);
            option("opt1",  0,  ts::Args::NONE);
            option("opt2", 'a', ts::Args::STRING);
            option("opt3",  0,  ts::Args::INTEGER, 0, ts::Args::UNLIMITED_COUNT, -4, 7);
            option("opt4",  0,  ts::Args::UNSIGNED, 0, 2);
            option("opt5", '5', ts::Args::POSITIVE, 0, ts::Args::UNLIMITED_COUNT);
            option("opt6", 'b', ts::Args::UINT8);
            option("opt7",  0,  ts::Args::UINT16);
            option("opt8",  0,  ts::Args::UINT32, 0, 0, 0, 0, true);
            option("opt9", 'c', ts::Enumeration("val1", 11, "val2", 12, "val3", 13, TS_NULL));
            option("mask",  0,  ts::Enumeration("bit1", 0x01, "bit2", 0x02, "bit3", 0x04, TS_NULL), 0, ts::Args::UNLIMITED_COUNT);
        }
    };
}

// Test case: analyze valid command, get option values, use analyze() with variable length argument list
void ArgsTest::testValidCommandVariableArgs()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    CPPUNIT_ASSERT(args.analyze ("test", "--opt3", "6", "a", "--opt1", "b", "--opt9", "val2", "--opt3", "0", "--opt3", "6", TS_NULL));

    CPPUNIT_ASSERT(args.appName() == "test");
    CPPUNIT_ASSERT(args.count ("") == 2);
    CPPUNIT_ASSERT(args.value ("", "", 0) == "a");
    CPPUNIT_ASSERT(args.value ("", "", 1) == "b");
    CPPUNIT_ASSERT(args.count ("opt1") == 1);
    CPPUNIT_ASSERT(args.present ("opt1"));
    CPPUNIT_ASSERT(args.count ("opt2") == 0);
    CPPUNIT_ASSERT(!args.present ("opt2"));
    CPPUNIT_ASSERT(args.count ("opt3") == 3);
    CPPUNIT_ASSERT(args.present ("opt3"));
    CPPUNIT_ASSERT(args.intValue<int> ("opt3", -1, 0) == 6);
    CPPUNIT_ASSERT(args.intValue<int> ("opt3", -1, 1) == 0);
    CPPUNIT_ASSERT(args.intValue<int> ("opt3", -1, 2) == 6);
    CPPUNIT_ASSERT(args.intValue<int> ("opt3", -1, 3) == -1);
    CPPUNIT_ASSERT(args.count ("opt4") == 0);
    CPPUNIT_ASSERT(args.count ("opt5") == 0);
    CPPUNIT_ASSERT(args.count ("opt6") == 0);
    CPPUNIT_ASSERT(args.count ("opt7") == 0);
    CPPUNIT_ASSERT(args.count ("opt8") == 0);
    CPPUNIT_ASSERT(args.count ("opt9") == 1);
    CPPUNIT_ASSERT(args.present ("opt9"));
    CPPUNIT_ASSERT(args.intValue<int> ("opt9") == 12);

    std::string s;
    args.getValue (s, "", "x", 0);
    CPPUNIT_ASSERT(s == "a");
    args.getValue (s, "", "x", 1);
    CPPUNIT_ASSERT(s == "b");
    args.getValue (s, "", "x", 2);
    CPPUNIT_ASSERT(s == "x");

    std::vector<std::string> vs;
    args.getValues (vs, "");
    std::vector<std::string> ref;
    ref.push_back ("a");
    ref.push_back ("b");
    CPPUNIT_ASSERT(vs == ref);

    int i;
    args.getIntValue (i, "opt3", -1, 0);
    CPPUNIT_ASSERT(i == 6);
    args.getIntValue (i, "opt3", -1, 1);
    CPPUNIT_ASSERT(i == 0);
    args.getIntValue (i, "opt3", -1, 2);
    CPPUNIT_ASSERT(i == 6);
    args.getIntValue (i, "opt3", -1, 3);
    CPPUNIT_ASSERT(i == -1);

    std::vector<int> vi;
    args.getIntValues(vi, "opt3");
    std::vector<int> iref;
    iref.push_back (6);
    iref.push_back (0);
    iref.push_back (6);
    CPPUNIT_ASSERT(vi == iref);

    std::set<int> vis;
    args.getIntValues(vis, "opt3");
    std::set<int> isref;
    isref.insert(0);
    isref.insert(6);
    CPPUNIT_ASSERT(isref == vis);
}

// Test case: analyze valid command, get option values, use analyze() with argc, argv parameters.
void ArgsTest::testValidCommandArgcArgv()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    char* argv[] = {
        const_cast<char*> ("test"),
        const_cast<char*> ("--opt3"),
        const_cast<char*> ("6"),
        const_cast<char*> ("a"),
        const_cast<char*> ("--opt1"),
        const_cast<char*> ("b"),
        const_cast<char*> ("--opt9"),
        const_cast<char*> ("val2"),
        const_cast<char*> ("--opt3"),
        const_cast<char*> ("0")};
    const int argc = 10;

    CPPUNIT_ASSERT(args.analyze (argc, argv));

    CPPUNIT_ASSERT(args.appName() == "test");
    CPPUNIT_ASSERT(args.count ("") == 2);
    CPPUNIT_ASSERT(args.value ("", "", 0) == "a");
    CPPUNIT_ASSERT(args.value ("", "", 1) == "b");
    CPPUNIT_ASSERT(args.count ("opt1") == 1);
    CPPUNIT_ASSERT(args.present ("opt1"));
    CPPUNIT_ASSERT(args.count ("opt2") == 0);
    CPPUNIT_ASSERT(!args.present ("opt2"));
    CPPUNIT_ASSERT(args.count ("opt3") == 2);
    CPPUNIT_ASSERT(args.present ("opt3"));
    CPPUNIT_ASSERT(args.intValue<int> ("opt3", -1, 0) == 6);
    CPPUNIT_ASSERT(args.intValue<int> ("opt3", -1, 1) == 0);
    CPPUNIT_ASSERT(args.intValue<int> ("opt3", -1, 2) == -1);
    CPPUNIT_ASSERT(args.count ("opt4") == 0);
    CPPUNIT_ASSERT(args.count ("opt5") == 0);
    CPPUNIT_ASSERT(args.count ("opt6") == 0);
    CPPUNIT_ASSERT(args.count ("opt7") == 0);
    CPPUNIT_ASSERT(args.count ("opt8") == 0);
    CPPUNIT_ASSERT(args.count ("opt9") == 1);
    CPPUNIT_ASSERT(args.present ("opt9"));
    CPPUNIT_ASSERT(args.intValue<int> ("opt9") == 12);

    std::string s;
    args.getValue (s, "", "x", 0);
    CPPUNIT_ASSERT(s == "a");
    args.getValue (s, "", "x", 1);
    CPPUNIT_ASSERT(s == "b");
    args.getValue (s, "", "x", 2);
    CPPUNIT_ASSERT(s == "x");

    std::vector<std::string> vs;
    args.getValues (vs, "");
    std::vector<std::string> ref;
    ref.push_back ("a");
    ref.push_back ("b");
    CPPUNIT_ASSERT(vs == ref);

    int i;
    args.getIntValue (i, "opt3", -1, 0);
    CPPUNIT_ASSERT(i == 6);
    args.getIntValue (i, "opt3", -1, 1);
    CPPUNIT_ASSERT(i == 0);
    args.getIntValue (i, "opt3", -1, 2);
    CPPUNIT_ASSERT(i == -1);

    std::vector<int> vi;
    args.getIntValues (vi, "opt3");
    std::vector<int> iref;
    iref.push_back (6);
    iref.push_back (0);
    CPPUNIT_ASSERT(vi == iref);
}

// Test case: analyze valid command, get option values, use analyze() with container of arguments
void ArgsTest::testValidCommandContainer()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    std::vector<std::string> arguments;
    arguments.push_back ("--opt3");
    arguments.push_back ("6");
    arguments.push_back ("a");
    arguments.push_back ("--opt1");
    arguments.push_back ("b");
    arguments.push_back ("--opt9");
    arguments.push_back ("val2");
    arguments.push_back ("--opt3");
    arguments.push_back ("0");

    CPPUNIT_ASSERT(args.analyze ("test", arguments));

    CPPUNIT_ASSERT(args.appName() == "test");
    CPPUNIT_ASSERT(args.count ("") == 2);
    CPPUNIT_ASSERT(args.value ("", "", 0) == "a");
    CPPUNIT_ASSERT(args.value ("", "", 1) == "b");
    CPPUNIT_ASSERT(args.count ("opt1") == 1);
    CPPUNIT_ASSERT(args.present ("opt1"));
    CPPUNIT_ASSERT(args.count ("opt2") == 0);
    CPPUNIT_ASSERT(!args.present ("opt2"));
    CPPUNIT_ASSERT(args.count ("opt3") == 2);
    CPPUNIT_ASSERT(args.present ("opt3"));
    CPPUNIT_ASSERT(args.intValue<int> ("opt3", -1, 0) == 6);
    CPPUNIT_ASSERT(args.intValue<int> ("opt3", -1, 1) == 0);
    CPPUNIT_ASSERT(args.intValue<int> ("opt3", -1, 2) == -1);
    CPPUNIT_ASSERT(args.count ("opt4") == 0);
    CPPUNIT_ASSERT(args.count ("opt5") == 0);
    CPPUNIT_ASSERT(args.count ("opt6") == 0);
    CPPUNIT_ASSERT(args.count ("opt7") == 0);
    CPPUNIT_ASSERT(args.count ("opt8") == 0);
    CPPUNIT_ASSERT(args.count ("opt9") == 1);
    CPPUNIT_ASSERT(args.present ("opt9"));
    CPPUNIT_ASSERT(args.intValue<int> ("opt9") == 12);

    std::string s;
    args.getValue (s, "", "x", 0);
    CPPUNIT_ASSERT(s == "a");
    args.getValue (s, "", "x", 1);
    CPPUNIT_ASSERT(s == "b");
    args.getValue (s, "", "x", 2);
    CPPUNIT_ASSERT(s == "x");

    std::vector<std::string> vs;
    args.getValues (vs, "");
    std::vector<std::string> ref;
    ref.push_back ("a");
    ref.push_back ("b");
    CPPUNIT_ASSERT(vs == ref);

    int i;
    args.getIntValue (i, "opt3", -1, 0);
    CPPUNIT_ASSERT(i == 6);
    args.getIntValue (i, "opt3", -1, 1);
    CPPUNIT_ASSERT(i == 0);
    args.getIntValue (i, "opt3", -1, 2);
    CPPUNIT_ASSERT(i == -1);

    std::vector<int> vi;
    args.getIntValues (vi, "opt3");
    std::vector<int> iref;
    iref.push_back (6);
    iref.push_back (0);
    CPPUNIT_ASSERT(vi == iref);
}

// Test case: presence of thousands separator
void ArgsTest::testThousandsSeparator()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(args.analyze("test", "a", "-5", "2000", "--opt5=3,000", "-50x4.000", "-5", "80 000", "-5", "2.000 000", TS_NULL));
    CPPUNIT_ASSERT_EQUAL(size_t(5), args.count("opt5"));
    CPPUNIT_ASSERT_EQUAL(2000, args.intValue<int>("opt5", 0, 0));
    CPPUNIT_ASSERT_EQUAL(3000, args.intValue<int>("opt5", 0, 1));
    CPPUNIT_ASSERT_EQUAL(0x4000, args.intValue<int>("opt5", 0, 2));
    CPPUNIT_ASSERT_EQUAL(80000, args.intValue<int>("opt5", 0, 3));
    CPPUNIT_ASSERT_EQUAL(2000000, args.intValue<int>("opt5", 0, 4));
}


// Test case: syntax of optional values
void ArgsTest::testOptionalValue()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    CPPUNIT_ASSERT(args.analyze ("test", "a", "--opt8", "2", TS_NULL));
    CPPUNIT_ASSERT(args.count ("") == 2);
    CPPUNIT_ASSERT(args.present ("opt8"));
    CPPUNIT_ASSERT(args.intValue<uint32_t> ("opt8") == 0);

    CPPUNIT_ASSERT(args.analyze ("test", "a", "--opt8=2", TS_NULL));
    CPPUNIT_ASSERT(args.count ("") == 1);
    CPPUNIT_ASSERT(args.present ("opt8"));
    CPPUNIT_ASSERT(args.intValue<uint32_t> ("opt8") == 2);
}

// Test case:
void ArgsTest::testMissingParameter()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    CPPUNIT_ASSERT(!args.analyze ("test", "--opt1", TS_NULL));
    utest::Out() << "ArgsTest: testMissingParameter: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT(log.getMessages() == "Error: missing parameter");
}

// Test case:
void ArgsTest::testTooManyParameters()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    CPPUNIT_ASSERT(!args.analyze ("test", "a", "b", "c", TS_NULL));
    utest::Out() << "ArgsTest: testTooManyParameters: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT(log.getMessages() == "Error: too many parameter, 2 maximum");
}

// Test case:
void ArgsTest::testAmbiguousOption()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    CPPUNIT_ASSERT(!args.analyze ("test", "--opt", "a", "b", TS_NULL));
    utest::Out() << "ArgsTest: testAmbiguousOption: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT(log.getMessages() == "Error: ambiguous option --opt (--opt1, --opt2)");
}

// Test case:
void ArgsTest::testInvalidIntegerOption()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    CPPUNIT_ASSERT(!args.analyze ("test", "--opt3", "x", "a", "b", TS_NULL));
    utest::Out() << "ArgsTest: testInvalidIntegerOption: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT(log.getMessages() == "Error: invalid integer value x for option --opt3");
}

// Test case:
void ArgsTest::testIntegerTooLow()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    CPPUNIT_ASSERT(!args.analyze ("test", "--opt3", "-10", "a", "b", TS_NULL));
    utest::Out() << "ArgsTest: testIntegerTooLow: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT(log.getMessages() == "Error: value for option --opt3 must be >= -4");
}

// Test case:
void ArgsTest::testIntegerTooHigh()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(!args.analyze ("test", "--opt3", "10", "a", "b", TS_NULL));
    utest::Out() << "ArgsTest: testIntegerTooHigh: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT(log.getMessages() == "Error: value for option --opt3 must be <= 7");
}

// Test case:
void ArgsTest::testInvalidEnum()
{
    ts::ReportBuffer<> log;
    TestArgs args(&log);

    CPPUNIT_ASSERT(!args.analyze ("test", "--opt9", "x", "a", "b", TS_NULL));
    utest::Out() << "ArgsTest: testInvalidEnum: \"" << log << "\"" << std::endl;
    CPPUNIT_ASSERT_EQUAL(std::string("Error: invalid value x for option --opt9 (-c), use one of val1, val2, val3"), log.getMessages());
}

// Test case:
void ArgsTest::testValidEnum()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    CPPUNIT_ASSERT(args.analyze("test", "--opt9", "0x20", "a", "b", TS_NULL));
    CPPUNIT_ASSERT_EQUAL(32, args.intValue<int>("opt9"));
}

// Test case:
void ArgsTest::testBitMask()
{
    ts::ReportBuffer<> log;
    TestArgs args (&log);

    CPPUNIT_ASSERT(args.analyze ("test", "a", TS_NULL));
    CPPUNIT_ASSERT(args.bitMaskValue<int> ("mask", 0x10) == 0x10);

    CPPUNIT_ASSERT(args.analyze ("test", "--mask", "bit1", "a", TS_NULL));
    CPPUNIT_ASSERT(args.bitMaskValue<int> ("mask", 0x10) == 0x01);

    CPPUNIT_ASSERT(args.analyze ("test", "--mask", "bit2", "--mask", "bit3", "a", TS_NULL));
    CPPUNIT_ASSERT(args.bitMaskValue<int> ("mask", 0x10) == 0x06);
}

// Test case: "gather parameters" option
void ArgsTest::testGatherParameters()
{
    ts::ReportBuffer<> log;
    ts::Args args ("description", "syntax", "help", ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS);
    args.redirectReport(&log);

    args.option ("");
    args.option ("opt1");
    args.option ("opt2", 'o', ts::Args::UNSIGNED);

    CPPUNIT_ASSERT(args.analyze ("test", "--opt1", "--opt2", "11", "12", "--opt2", "13", TS_NULL));
    CPPUNIT_ASSERT(args.valid());
    CPPUNIT_ASSERT(args.count ("opt1") == 1);
    CPPUNIT_ASSERT(args.count ("opt2") == 1);
    CPPUNIT_ASSERT(args.count ("") == 3);
    CPPUNIT_ASSERT(args.intValue<int> ("opt2") == 11);
    CPPUNIT_ASSERT(args.value ("", "", 0) == "12");
    CPPUNIT_ASSERT(args.value ("", "", 1) == "--opt2");
    CPPUNIT_ASSERT(args.value ("", "", 2) == "13");

    CPPUNIT_ASSERT(args.valid());
    args.invalidate();
    CPPUNIT_ASSERT(!args.valid());
}
