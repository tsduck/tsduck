//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for subclasses of ts::Report
//
//----------------------------------------------------------------------------

#include "tsReportBuffer.h"
#include "tsReportFile.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ReportTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Severity);
    TSUNIT_DECLARE_TEST(String);
    TSUNIT_DECLARE_TEST(Printf);
    TSUNIT_DECLARE_TEST(ByName);
    TSUNIT_DECLARE_TEST(ByStream);
    TSUNIT_DECLARE_TEST(ErrCodeReport);
    TSUNIT_DECLARE_TEST(Delegation);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    fs::path _fileName {};
};

TSUNIT_REGISTER(ReportTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ReportTest::beforeTest()
{
    _fileName = ts::TempFile();
    fs::remove(_fileName, &ts::ErrCodeReport());
}

// Test suite cleanup method.
void ReportTest::afterTest()
{
    // Returned value ignored on purpose, end of test, temporary file may not even exists.
    fs::remove(_fileName, &ts::ErrCodeReport());
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

namespace {
    using TestBuffer = ts::ReportBuffer<ts::ThreadSafety::Full>;
}

// Test case: set/get severity
TSUNIT_DEFINE_TEST(Severity)
{
    TestBuffer log1;
    TSUNIT_ASSERT(log1.maxSeverity() == ts::Severity::Info);
    TSUNIT_ASSERT(!log1.debug());
    TSUNIT_ASSERT(!log1.verbose());

    TestBuffer log2(ts::Severity::Verbose);
    TSUNIT_ASSERT(log2.maxSeverity() == ts::Severity::Verbose);
    TSUNIT_ASSERT(!log2.debug());
    TSUNIT_ASSERT(log2.verbose());

    log2.setMaxSeverity(4);
    TSUNIT_ASSERT(log2.maxSeverity() == 4);
    TSUNIT_ASSERT(log2.debug());
    TSUNIT_ASSERT(log2.verbose());

    log2.setMaxSeverity(ts::Severity::Warning);
    TSUNIT_ASSERT(log2.maxSeverity() == ts::Severity::Warning);
    TSUNIT_ASSERT(!log2.debug());
    TSUNIT_ASSERT(!log2.verbose());
}

// Log sequence for testString
namespace {
    void _testStringSequence(TestBuffer& log, int level)
    {
        const ts::UString str1(u"1");
        const ts::UString str2(u"2");
        const ts::UString str3(u"3");
        const ts::UString str4(u"4");
        const ts::UString str5(u"5");
        const ts::UString str6(u"6");
        const ts::UString str7(u"7");
        const ts::UString str8(u"8");

        log.setMaxSeverity(level);
        log.clear();

        log.log(ts::Severity::Info, str1);
        log.debug(str2);
        log.log(ts::Severity::Debug, str3);
        log.warning(str4);
        log.info(str5);
        log.fatal(str6);
        log.log(ts::Severity::Fatal, str7);
        log.error(str8);
    }
}

// Test case: log string messages
TSUNIT_DEFINE_TEST(String)
{
    TestBuffer log;
    TSUNIT_ASSERT(log.empty());

    _testStringSequence(log, ts::Severity::Debug);
    TSUNIT_ASSERT(!log.empty());
    TSUNIT_EQUAL(u"1\n"
                 u"Debug: 2\n"
                 u"Debug: 3\n"
                 u"Warning: 4\n"
                 u"5\n"
                 u"FATAL ERROR: 6\n"
                 u"FATAL ERROR: 7\n"
                 u"Error: 8",
                 log.messages());

    _testStringSequence(log, ts::Severity::Info);
    TSUNIT_ASSERT(!log.empty());
    TSUNIT_EQUAL(u"1\n"
                 u"Warning: 4\n"
                 u"5\n"
                 u"FATAL ERROR: 6\n"
                 u"FATAL ERROR: 7\n"
                 u"Error: 8",
                 log.messages());

    _testStringSequence(log, ts::Severity::Warning);
    TSUNIT_ASSERT(!log.empty());
    TSUNIT_EQUAL(u"Warning: 4\n"
                 u"FATAL ERROR: 6\n"
                 u"FATAL ERROR: 7\n"
                 u"Error: 8",
                 log.messages());

    _testStringSequence(log, ts::Severity::Error);
    TSUNIT_ASSERT(!log.empty());
    TSUNIT_EQUAL(u"FATAL ERROR: 6\n"
                 u"FATAL ERROR: 7\n"
                 u"Error: 8",
                 log.messages());

    _testStringSequence(log, ts::Severity::Fatal);
    TSUNIT_ASSERT(!log.empty());
    TSUNIT_EQUAL(u"FATAL ERROR: 6\n"
                 u"FATAL ERROR: 7",
                 log.messages());

    _testStringSequence(log, ts::Severity::Fatal - 1);
    TSUNIT_ASSERT(log.empty());
    TSUNIT_EQUAL(u"", log.messages());
}

// Log sequence for testPrintf
namespace {
    void _testPrintfSequence(TestBuffer& log, int level)
    {
        log.setMaxSeverity(level);
        log.clear();

        log.log(ts::Severity::Info, u"%d", 1);
        log.debug(u"%d", 2);
        log.log(ts::Severity::Debug, u"%d", 3);
        log.warning(u"%d", 4);
        log.info(u"%d", 5);
        log.fatal(u"%d", 6);
        log.log(ts::Severity::Fatal, u"%d", 7);
        log.error(u"%d", 8);
    }
}

// Test case: log using printf-like formats
TSUNIT_DEFINE_TEST(Printf)
{
    TestBuffer log;

    _testPrintfSequence(log, ts::Severity::Debug);
    TSUNIT_EQUAL(u"1\n"
                 u"Debug: 2\n"
                 u"Debug: 3\n"
                 u"Warning: 4\n"
                 u"5\n"
                 u"FATAL ERROR: 6\n"
                 u"FATAL ERROR: 7\n"
                 u"Error: 8",
                 log.messages());

    _testPrintfSequence(log, ts::Severity::Info);
    TSUNIT_EQUAL(u"1\n"
                 u"Warning: 4\n"
                 u"5\n"
                 u"FATAL ERROR: 6\n"
                 u"FATAL ERROR: 7\n"
                 u"Error: 8",
                 log.messages());

    _testPrintfSequence(log, ts::Severity::Warning);
    TSUNIT_EQUAL(u"Warning: 4\n"
                 u"FATAL ERROR: 6\n"
                 u"FATAL ERROR: 7\n"
                 u"Error: 8",
                 log.messages());

    _testPrintfSequence(log, ts::Severity::Error);
    TSUNIT_EQUAL(u"FATAL ERROR: 6\n"
                 u"FATAL ERROR: 7\n"
                 u"Error: 8",
                 log.messages());

    _testPrintfSequence(log, ts::Severity::Fatal);
    TSUNIT_EQUAL(u"FATAL ERROR: 6\n"
                 u"FATAL ERROR: 7",
                 log.messages());

    _testPrintfSequence(log, ts::Severity::Fatal - 1);
    TSUNIT_ASSERT(log.empty());
    TSUNIT_EQUAL(u"", log.messages());
}

// Test case: log file by name
TSUNIT_DEFINE_TEST(ByName)
{
    {
        ts::ReportFile<ts::ThreadSafety::Full> log(_fileName, false, false);
        log.info(u"info %d 1");
        log.error(u"error %s 1");
    }

    ts::UStringVector ref;
    ref.push_back(u"info %d 1");
    ref.push_back(u"Error: error %s 1");

    ts::UStringVector value;
    ts::UString::Load(value, _fileName);
    TSUNIT_ASSERT(value == ref);

    {
        ts::ReportFile<ts::ThreadSafety::Full> log(_fileName, true, false);
        log.info(u"info 2");
        log.error(u"error 2");
    }

    ref.push_back(u"info 2");
    ref.push_back(u"Error: error 2");

    ts::UString::Load(value, _fileName);
    TSUNIT_ASSERT(value == ref);
}

// Test case: log file on open stream
TSUNIT_DEFINE_TEST(ByStream)
{
    std::ofstream stream(_fileName);
    TSUNIT_ASSERT(stream.is_open());

    {
        ts::ReportFile<ts::ThreadSafety::Full> log(stream);
        log.info(u"info 1");
        log.error(u"error 1");
    }
    TSUNIT_ASSERT(stream.is_open());

    stream.close();
    TSUNIT_ASSERT(!stream.is_open());

    ts::UStringVector ref;
    ref.push_back(u"info 1");
    ref.push_back(u"Error: error 1");

    ts::UStringVector value;
    ts::UString::Load(value, _fileName);
    TSUNIT_ASSERT(value == ref);
}

// Test case: std::error_code logging.
TSUNIT_DEFINE_TEST(ErrCodeReport)
{
    TestBuffer log;

    // Test existent directory.
    fs::path dir(fs::temp_directory_path());
    debug() << "ReportTest::testErrCodeReport: testing \"" << dir.string() << "\"" << std::endl;

    TSUNIT_ASSERT(fs::is_directory(dir, &ts::ErrCodeReport(log, u"isdir", dir)));
    debug() << "ReportTest::testErrCodeReport: log: \"" << log.messages() << "\"" << std::endl;
    TSUNIT_ASSERT(log.empty());

    // Test non-existent directory.
    fs::path nodir(dir);
    nodir /= "nonexistent";
    debug() << "ReportTest::testErrCodeReport: testing \"" << nodir.string() << "\"" << std::endl;

    log.clear();
    TSUNIT_ASSERT(!fs::is_directory(nodir, &ts::ErrCodeReport(log, u"isdir", nodir)));
    debug() << "ReportTest::testErrCodeReport: log: \"" << log.messages() << "\"" << std::endl;
    TSUNIT_ASSERT(!log.empty());
    TSUNIT_ASSERT(log.messages().startWith(u"Error: isdir " + nodir + u":"));
}

// Test case: report delegation.
TSUNIT_DEFINE_TEST(Delegation)
{
    TestBuffer log;
    ts::Report rep;
    ts::Report* previous = nullptr;

    previous = rep.delegateReport(&log);
    TSUNIT_ASSERT(previous == nullptr);

    rep.info(u"text 1");
    TSUNIT_EQUAL(u"text 1", log.messages());

    log.clear();
    TSUNIT_EQUAL(u"", log.messages());

    rep.setMaxSeverity(ts::Severity::Verbose);
    TSUNIT_EQUAL(ts::Severity::Verbose, rep.maxSeverity());
    TSUNIT_EQUAL(ts::Severity::Verbose, log.maxSeverity());

    rep.verbose(u"text 2");
    TSUNIT_EQUAL(u"text 2", log.messages());

    log.clear();
    TSUNIT_EQUAL(u"", log.messages());

    log.setMaxSeverity(ts::Severity::Debug);
    TSUNIT_EQUAL(ts::Severity::Debug, rep.maxSeverity());
    TSUNIT_EQUAL(ts::Severity::Debug, log.maxSeverity());

    rep.debug(u"text 3");
    TSUNIT_EQUAL(u"Debug: text 3", log.messages());

    log.clear();
    TSUNIT_EQUAL(u"", log.messages());

    rep.setMaxSeverity(ts::Severity::Info);
    TSUNIT_EQUAL(ts::Severity::Info, rep.maxSeverity());
    TSUNIT_EQUAL(ts::Severity::Info, log.maxSeverity());

    rep.verbose(u"text 4");
    TSUNIT_EQUAL(u"", log.messages());

    rep.info(u"text 5");
    TSUNIT_EQUAL(u"text 5", log.messages());

    log.clear();
    TSUNIT_EQUAL(u"", log.messages());

    log.setReportPrefix(u"LOG: ");
    rep.info(u"text 6");
    TSUNIT_EQUAL(u"LOG: text 6", log.messages());

    log.clear();
    TSUNIT_EQUAL(u"", log.messages());

    rep.setReportPrefix(u"REP: ");
    rep.info(u"text 7");
    TSUNIT_EQUAL(u"LOG: REP: text 7", log.messages());

    log.clear();
    TSUNIT_EQUAL(u"", log.messages());

    previous = rep.delegateReport(nullptr);
    TSUNIT_ASSERT(previous == &log);

    previous = rep.delegateReport(nullptr);
    TSUNIT_ASSERT(previous == nullptr);

    rep.info(u"text 6");
    TSUNIT_EQUAL(u"", log.messages());
}
