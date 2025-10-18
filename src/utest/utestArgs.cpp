//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::Args
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsReportBuffer.h"
#include "tsCerrReport.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsFixedPoint.h"
#include "tsFraction.h"
#include "tsFloatingPoint.h"
#include "tsVersion.h"
#include "tsBitRate.h"
#include "tsCryptoLibrary.h"
#include "tsDektecSupport.h"
#include "tsVatekUtils.h"
#include "tsRIST.h"
#include "tsSRTSocket.h"
#include "tsTLSConnection.h"
#include "tsunit.h"

// Force inclusion of some modules if statically linked.
TS_STATIC_REFERENCE(Ref, reinterpret_cast<const void*>(ts::GetBitRateDescription));
TS_STATIC_REFERENCE(Ref, reinterpret_cast<const void*>(ts::GetCryptographicLibraryVersion));
TS_STATIC_REFERENCE(Ref, reinterpret_cast<const void*>(ts::HasDektecSupport));
TS_STATIC_REFERENCE(Ref, reinterpret_cast<const void*>(ts::SRTSocket::GetLibraryVersion));
TS_STATIC_REFERENCE(Ref, reinterpret_cast<const void*>(ts::GetRISTLibraryVersion));
TS_STATIC_REFERENCE(Ref, reinterpret_cast<const void*>(&ts::TLSConnection::FEATURE));
TS_STATIC_REFERENCE(Ref, reinterpret_cast<const void*>(ts::GetVatekVersion));


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ArgsTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Accessors);
    TSUNIT_DECLARE_TEST(HelpDefault);
    TSUNIT_DECLARE_TEST(CopyOptions);
    TSUNIT_DECLARE_TEST(HelpCustom);
    TSUNIT_DECLARE_TEST(ValidCommandVariableArgs);
    TSUNIT_DECLARE_TEST(ValidCommandArgcArgv);
    TSUNIT_DECLARE_TEST(ValidCommandContainer);
    TSUNIT_DECLARE_TEST(OptionalValue);
    TSUNIT_DECLARE_TEST(ThousandsSeparator);
    TSUNIT_DECLARE_TEST(MissingParameter);
    TSUNIT_DECLARE_TEST(TooManyParameters);
    TSUNIT_DECLARE_TEST(AmbiguousOption);
    TSUNIT_DECLARE_TEST(InvalidIntegerOption);
    TSUNIT_DECLARE_TEST(IntegerTooLow);
    TSUNIT_DECLARE_TEST(IntegerTooHigh);
    TSUNIT_DECLARE_TEST(InvalidEnum);
    TSUNIT_DECLARE_TEST(ValidEnum);
    TSUNIT_DECLARE_TEST(BitMask);
    TSUNIT_DECLARE_TEST(GatherParameters);
    TSUNIT_DECLARE_TEST(Redirection);
    TSUNIT_DECLARE_TEST(Tristate);
    TSUNIT_DECLARE_TEST(Ranges);
    TSUNIT_DECLARE_TEST(Decimals);
    TSUNIT_DECLARE_TEST(FixedPoint);
    TSUNIT_DECLARE_TEST(Fraction);
    TSUNIT_DECLARE_TEST(Double);
    TSUNIT_DECLARE_TEST(Chrono);
    TSUNIT_DECLARE_TEST(InvalidFraction);
    TSUNIT_DECLARE_TEST(InvalidDouble);
    TSUNIT_DECLARE_TEST(LegacyOption);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    using Double = ts::FloatingPoint<double>;
    using USV = ts::UStringVector;
    fs::path _tempFile1 {};
    fs::path _tempFile2 {};
};

TSUNIT_REGISTER(ArgsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ArgsTest::beforeTest()
{
    _tempFile1 = ts::TempFile();
    _tempFile2 = ts::TempFile();
}

// Test suite cleanup method.
void ArgsTest::afterTest()
{
    fs::remove(_tempFile1, &ts::ErrCodeReport());
    fs::remove(_tempFile2, &ts::ErrCodeReport());
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test case: basic accessors
TSUNIT_DEFINE_TEST(Accessors)
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
TSUNIT_DEFINE_TEST(HelpDefault)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);
    args.delegateReport(&log);

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
                 u"  --version[=name]\n"
                 u"      Display the TSDuck version number.\n"
                 u"      The 'name' must be one of \"acceleration\", \"all\", \"bitrate\", \"compiler\",\n"
                 u"      \"crypto\", \"date\", \"dektec\", \"http\", \"integer\", \"long\", \"rist\", \"short\",\n"
                 u"      \"srt\", \"system\", \"tls\", \"vatek\", \"zlib\".\n",
                 log.messages());

    args.setShell(u"{shell}");
    log.clear();
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
                 u"  --version[=name]\n"
                 u"      Display the TSDuck version number.\n"
                 u"      The 'name' must be one of \"acceleration\", \"all\", \"bitrate\", \"compiler\",\n"
                 u"      \"crypto\", \"date\", \"dektec\", \"http\", \"integer\", \"long\", \"rist\", \"short\",\n"
                 u"      \"srt\", \"system\", \"tls\", \"vatek\", \"zlib\".\n",
                 log.messages());

    log.clear();
    TSUNIT_ASSERT(!args.analyze(u"test", USV({u"--version=short"})));
    const ts::UString version(log.messages());
    debug() << "ArgsTest::testHelpDefault: version = \"" << version << "\"" << std::endl;
    const size_t dash = version.find(u'-');
    TSUNIT_ASSERT(dash != ts::NPOS);
    TSUNIT_EQUAL(TS_USTRINGIFY(TS_VERSION_MAJOR) u"." TS_USTRINGIFY(TS_VERSION_MINOR), version.substr(0, dash));
}

// Test case: copy options
TSUNIT_DEFINE_TEST(CopyOptions)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    ts::Args args1(u"{description1}", u"{syntax1}", ts::Args::NO_EXIT_ON_ERROR);
    ts::Args args2(u"{description2}", u"{syntax2}", ts::Args::NO_EXIT_ON_ERROR);

    args1.delegateReport(&log);
    args2.delegateReport(&log);

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
            delegateReport(log);
            option(u"",      0,  ts::Args::STRING, 1, 2);
            option(u"opt1",  0,  ts::Args::NONE);
            option(u"opt2", 'a', ts::Args::STRING);
            option(u"opt3",  0,  ts::Args::INTEGER, 0, ts::Args::UNLIMITED_COUNT, -4, 7);
            option(u"opt4",  0,  ts::Args::UNSIGNED, 0, 2);
            option(u"opt5", '5', ts::Args::POSITIVE, 0, ts::Args::UNLIMITED_COUNT);
            option(u"opt6", 'b', ts::Args::UINT8);
            option(u"opt7",  0,  ts::Args::UINT16);
            option(u"opt8",  0,  ts::Args::UINT32, 0, 0, 0, 0, true);
            option(u"opt9", 'c', ts::Names({{u"val1", 11}, {u"val2", 12}, {u"val3", 13}}));
            option(u"mask",  0,  ts::Names({{u"bit1", 0x01}, {u"bit2", 0x02}, {u"bit3", 0x04}}), 0, ts::Args::UNLIMITED_COUNT);
            option(u"opt10", 0,  ts::Args::UNSIGNED, 0, ts::Args::UNLIMITED_COUNT, 0, 0, false, 3);
            option<cn::seconds>(u"opt11");

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
            help(u"opt11", u"Number of seconds.");
        }
    };
}

// Test case: help text of a custom commmand.
TSUNIT_DEFINE_TEST(HelpCustom)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
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
                 u"      The 'value' must be one of \"bit1\", \"bit2\", \"bit3\".\n"
                 u"\n"
                 u"  --opt1\n"
                 u"      No value.\n"
                 u"\n"
                 u"  --opt10 value\n"
                 u"      Unsigned int 3 decimal digits.\n"
                 u"      The value may include up to 3 meaningful decimal digits.\n"
                 u"\n"
                 u"  --opt11 seconds\n"
                 u"      Number of seconds.\n"
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
                 u"      The 'value' must be one of \"val1\", \"val2\", \"val3\".\n"
                 u"\n"
                 u"  -v\n"
                 u"  --verbose\n"
                 u"      Produce verbose output.\n"
                 u"\n"
                 u"  --version[=name]\n"
                 u"      Display the TSDuck version number.\n"
                 u"      The 'name' must be one of \"acceleration\", \"all\", \"bitrate\", \"compiler\",\n"
                 u"      \"crypto\", \"date\", \"dektec\", \"http\", \"integer\", \"long\", \"rist\", \"short\",\n"
                 u"      \"srt\", \"system\", \"tls\", \"vatek\", \"zlib\".\n",
                 log.messages());
}

// Test case: analyze valid command, get option values, use analyze() with variable length argument list
TSUNIT_DEFINE_TEST(ValidCommandVariableArgs)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
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
TSUNIT_DEFINE_TEST(ValidCommandArgcArgv)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
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
TSUNIT_DEFINE_TEST(ValidCommandContainer)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
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
TSUNIT_DEFINE_TEST(ThousandsSeparator)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
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
TSUNIT_DEFINE_TEST(OptionalValue)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
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
TSUNIT_DEFINE_TEST(MissingParameter)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", USV({u"--opt1"})));
    debug() << "ArgsTest: testMissingParameter: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: missing parameter", log.messages());
}

// Test case:
TSUNIT_DEFINE_TEST(TooManyParameters)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", {u"a", u"b", u"c"}));
    debug() << "ArgsTest: testTooManyParameters: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: too many parameter, 2 maximum", log.messages());
}

// Test case:
TSUNIT_DEFINE_TEST(AmbiguousOption)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt", u"a", u"b"}));
    debug() << "ArgsTest: testAmbiguousOption: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: ambiguous option --opt (--opt1, --opt10)", log.messages());
}

// Test case:
TSUNIT_DEFINE_TEST(InvalidIntegerOption)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt3", u"x", u"a", u"b"}));
    debug() << "ArgsTest: testInvalidIntegerOption: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: invalid integer value x for option --opt3", log.messages());
}

// Test case:
TSUNIT_DEFINE_TEST(IntegerTooLow)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt3", u"-10", u"a", u"b"}));
    debug() << "ArgsTest: testIntegerTooLow: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: value for option --opt3 must be >= -4", log.messages());
}

// Test case:
TSUNIT_DEFINE_TEST(IntegerTooHigh)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test --opt3 10 a b"));
    debug() << "ArgsTest: testIntegerTooHigh: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: value for option --opt3 must be <= 7", log.messages());
}

// Test case:
TSUNIT_DEFINE_TEST(InvalidEnum)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(!args.analyze(u"test --opt9 x a b"));
    debug() << "ArgsTest: testInvalidEnum: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: invalid value x for option --opt9 (-c), use one of \"val1\", \"val2\", \"val3\"", log.messages());
}

// Test case:
TSUNIT_DEFINE_TEST(ValidEnum)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(args.analyze(u"test", {u"--opt9", u"0x20", u"a", u"b"}));
    TSUNIT_EQUAL(32, args.intValue<int>(u"opt9"));
}

// Test case: bitmask of integer values.
TSUNIT_DEFINE_TEST(BitMask)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    TestArgs args(&log);

    TSUNIT_ASSERT(args.analyze(u"test", USV({u"a"})));
    TSUNIT_EQUAL(0x10, args.bitMaskValue<int>(u"mask", 0x10));

    TSUNIT_ASSERT(args.analyze(u"test", {u"--mask", u"bit1", u"a"}));
    TSUNIT_EQUAL(0x01, args.bitMaskValue<int>(u"mask", 0x10));

    TSUNIT_ASSERT(args.analyze(u"test", {u"--mask", u"bit2", u"--mask", u"bit3", u"a"}));
    TSUNIT_EQUAL(0x06, args.bitMaskValue<int>(u"mask", 0x10));
}

// Test case: "gather parameters" option
TSUNIT_DEFINE_TEST(GatherParameters)
{
    ts::ReportBuffer<ts::ThreadSafety::None> log;
    ts::Args args(u"description", u"syntax", ts::Args::NO_EXIT_ON_ERROR | ts::Args::GATHER_PARAMETERS);
    args.delegateReport(&log);

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
TSUNIT_DEFINE_TEST(Redirection)
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
TSUNIT_DEFINE_TEST(Tristate)
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
TSUNIT_DEFINE_TEST(Ranges)
{
    ts::Args args(u"description", u"syntax", ts::Args::NO_EXIT_ON_ERROR);
    args.option(u"opt1", 0, ts::Args::UINT8, 0, ts::Args::UNLIMITED_COUNT);
    args.option(u"opt2", 0, ts::Args::UINT8, 0, 3, 0, 100);
    args.option(u"opt3", 0, ts::Args::INTEGER, 0, ts::Args::UNLIMITED_COUNT, 0, ts::Args::UNLIMITED_VALUE, true);

    ts::ReportBuffer<ts::ThreadSafety::None> log;
    args.delegateReport(&log);

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
    TSUNIT_EQUAL(u"Error: too many option --opt2, 3 maximum", log.messages());

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
TSUNIT_DEFINE_TEST(Decimals)
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
TSUNIT_DEFINE_TEST(FixedPoint)
{
    using Fixed = ts::FixedPoint<int32_t, 3>;

    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);
    args.delegateReport(&CERR);
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
TSUNIT_DEFINE_TEST(Fraction)
{
    using Frac = ts::Fraction<int32_t>;

    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);
    args.delegateReport(&CERR);
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
TSUNIT_DEFINE_TEST(Double)
{
    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);
    args.delegateReport(&CERR);
    args.option<Double>(u"");

    TSUNIT_ASSERT(args.analyze(u"test", {u"1", u"2.56", u"0", u" -6.12"}));
    TSUNIT_EQUAL(4, args.count(u""));

    TSUNIT_EQUAL(1.0,   args.numValue<Double>(u"", 0, 0).toDouble());
    TSUNIT_EQUAL(2.56,  args.numValue<Double>(u"", 0, 1).toDouble());
    TSUNIT_EQUAL(0.0,   args.numValue<Double>(u"", 0, 2).toDouble());
    TSUNIT_EQUAL(-6.12, args.numValue<Double>(u"", 0, 3).toDouble());
}

// Test case: std::chrono::duration types.
TSUNIT_DEFINE_TEST(Chrono)
{
    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);
    args.delegateReport(&CERR);
    args.option<cn::seconds>(u"foo", 0, 0, ts::Args::UNLIMITED_COUNT);

    TSUNIT_ASSERT(args.analyze(u"test", {u"--foo", u"23", u"--foo", u"6"}));
    TSUNIT_EQUAL(2, args.count(u"foo"));

    cn::seconds s;
    cn::milliseconds ms;

    args.getChronoValue(s, u"foo");
    TSUNIT_EQUAL(23, s.count());

    args.getChronoValue(ms, u"foo");
    TSUNIT_EQUAL(23'000, ms.count());

    args.getChronoValue(s, u"foo", 1);
    TSUNIT_EQUAL(6, s.count());

    args.getChronoValue(ms, u"foo", 1);
    TSUNIT_EQUAL(6'000, ms.count());

    args.getChronoValue(s, u"foo", 2);
    TSUNIT_EQUAL(0, s.count());

    args.getChronoValue(ms, u"foo", 2);
    TSUNIT_EQUAL(0, ms.count());

    args.getChronoValue(s, u"foo", cn::minutes(1), 2);
    TSUNIT_EQUAL(60, s.count());

    args.getChronoValue(ms, u"foo", cn::minutes(1), 2);
    TSUNIT_EQUAL(60'000, ms.count());
}

// Test case:
TSUNIT_DEFINE_TEST(InvalidFraction)
{
    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);

    ts::ReportBuffer<ts::ThreadSafety::None> log;
    args.delegateReport(&log);

    using Frac = ts::Fraction<int32_t>;
    args.option<Frac>(u"opt");

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt", u"foo"}));
    debug() << "ArgsTest: testInvalidFraction: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: invalid value foo for option --opt", log.messages());
}

// Test case:
TSUNIT_DEFINE_TEST(InvalidDouble)
{
    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);

    ts::ReportBuffer<ts::ThreadSafety::None> log;
    args.delegateReport(&log);
    args.option<Double>(u"opt", 0, 0, 1, 12, 15);

    TSUNIT_ASSERT(!args.analyze(u"test", {u"--opt", u"2.3"}));
    debug() << "ArgsTest: testInvalidDouble: \"" << log << "\"" << std::endl;
    TSUNIT_EQUAL(u"Error: value for option --opt must be in range 12 to 15", log.messages());
}


// Test case: legacy options
TSUNIT_DEFINE_TEST(LegacyOption)
{
    ts::Args args(u"{description}", u"{syntax}", ts::Args::NO_EXIT_ON_ERROR | ts::Args::NO_EXIT_ON_HELP | ts::Args::NO_EXIT_ON_VERSION | ts::Args::HELP_ON_THIS);
    args.option(u"new1", '1');
    args.help(u"new1", u"New option 1.");
    args.legacyOption(u"old1", u"new1");
    args.option(u"new2", '2', ts::Args::STRING);
    args.help(u"new2", u"New option 2.");
    args.legacyOption(u"old2", u"new2");
    args.option(u"new3", '3', ts::Args::INT32);
    args.help(u"new3", u"New option 3.");
    args.legacyOption(u"old3", u"new3");

    ts::ReportBuffer<ts::ThreadSafety::None> log;
    args.delegateReport(&log);

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
                 u"  -1\n"
                 u"  --new1\n"
                 u"      New option 1.\n"
                 u"\n"
                 u"  -2 value\n"
                 u"  --new2 value\n"
                 u"      New option 2.\n"
                 u"\n"
                 u"  -3 value\n"
                 u"  --new3 value\n"
                 u"      New option 3.\n"
                 u"\n"
                 u"  -v\n"
                 u"  --verbose\n"
                 u"      Produce verbose output.\n"
                 u"\n"
                 u"  --version[=name]\n"
                 u"      Display the TSDuck version number.\n"
                 u"      The 'name' must be one of \"acceleration\", \"all\", \"bitrate\", \"compiler\",\n"
                 u"      \"crypto\", \"date\", \"dektec\", \"http\", \"integer\", \"long\", \"rist\", \"short\",\n"
                 u"      \"srt\", \"system\", \"tls\", \"vatek\", \"zlib\".\n",
                 log.messages());
    log.clear();

    TSUNIT_ASSERT(args.analyze(u"test", USV({u"--old1", u"--old2", u"foo"})));
    TSUNIT_ASSERT(args.present(u"new1"));
    TSUNIT_ASSERT(args.present(u"new2"));
    TSUNIT_ASSERT(!args.present(u"new3"));
    TSUNIT_ASSERT(args.present(u"old1"));
    TSUNIT_ASSERT(args.present(u"old2"));
    TSUNIT_ASSERT(!args.present(u"old3"));
    TSUNIT_EQUAL(u"foo", args.value(u"new2"));
    TSUNIT_EQUAL(u"foo", args.value(u"old2"));
    TSUNIT_EQUAL(0, args.intValue<int>(u"new3"));
    TSUNIT_EQUAL(0, args.intValue<int>(u"old3"));
}
