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
//  CppUnit test suite for subclasses of ts::ReportInterface
//
//----------------------------------------------------------------------------

#include "tsReportBuffer.h"
#include "tsReportFile.h"
#include "tsSysUtils.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ReportTest: public CppUnit::TestFixture
{
public:
    ReportTest();
    void setUp();
    void tearDown();
    void testSeverity();
    void testString();
    void testPrintf();
    void testByName();
    void testByStream();

    CPPUNIT_TEST_SUITE(ReportTest);
    CPPUNIT_TEST(testSeverity);
    CPPUNIT_TEST(testString);
    CPPUNIT_TEST(testPrintf);
    CPPUNIT_TEST(testByName);
    CPPUNIT_TEST(testByStream);
    CPPUNIT_TEST_SUITE_END();

private:
    std::string _fileName;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ReportTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
ReportTest::ReportTest() :
    _fileName()
{
}

// Test suite initialization method.
void ReportTest::setUp()
{
    _fileName = ts::TempFile();
}

// Test suite cleanup method.
void ReportTest::tearDown()
{
    // Returned value ignored on purpose, end of test, temporary file may not even exists.
    // coverity[CHECKED_RETURN]
    ts::DeleteFile(_fileName);
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test case: set/get severity
void ReportTest::testSeverity()
{
    ts::ReportBuffer<> log1;
    CPPUNIT_ASSERT(log1.debugLevel() == ts::Severity::Info);
    CPPUNIT_ASSERT(!log1.debug());
    CPPUNIT_ASSERT(!log1.verbose());

    ts::ReportBuffer<> log2(true);
    CPPUNIT_ASSERT(log2.debugLevel() == ts::Severity::Verbose);
    CPPUNIT_ASSERT(!log2.debug());
    CPPUNIT_ASSERT(log2.verbose());

    log2.setDebugLevel(4);
    CPPUNIT_ASSERT(log2.debugLevel() == 4);
    CPPUNIT_ASSERT(log2.debug());
    CPPUNIT_ASSERT(log2.verbose());

    log2.setDebugLevel(ts::Severity::Warning);
    CPPUNIT_ASSERT(log2.debugLevel() == ts::Severity::Warning);
    CPPUNIT_ASSERT(!log2.debug());
    CPPUNIT_ASSERT(!log2.verbose());
}

// Log sequence for testString
namespace {
    void _testStringSequence(ts::ReportBuffer<>& log, int level)
    {
        const std::string str1("1");
        const std::string str2("2");
        const std::string str3("3");
        const std::string str4("4");
        const std::string str5("5");
        const std::string str6("6");
        const std::string str7("7");
        const std::string str8("8");

        log.setDebugLevel(level);
        log.resetMessages();

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
void ReportTest::testString()
{
    ts::ReportBuffer<> log;
    CPPUNIT_ASSERT(log.emptyMessages());

    _testStringSequence(log, ts::Severity::Debug);
    CPPUNIT_ASSERT(!log.emptyMessages());
    CPPUNIT_ASSERT_EQUAL(std::string("1\n"
                                     "Debug: 2\n"
                                     "Debug: 3\n"
                                     "Warning: 4\n"
                                     "5\n"
                                     "FATAL ERROR: 6\n"
                                     "FATAL ERROR: 7\n"
                                     "Error: 8"),
                         log.getMessages());

    _testStringSequence (log, ts::Severity::Info);
    CPPUNIT_ASSERT(!log.emptyMessages());
    CPPUNIT_ASSERT_EQUAL(std::string("1\n"
                                     "Warning: 4\n"
                                     "5\n"
                                     "FATAL ERROR: 6\n"
                                     "FATAL ERROR: 7\n"
                                     "Error: 8"),
                         log.getMessages());

    _testStringSequence (log, ts::Severity::Warning);
    CPPUNIT_ASSERT(!log.emptyMessages());
    CPPUNIT_ASSERT_EQUAL(std::string("Warning: 4\n"
                                     "FATAL ERROR: 6\n"
                                     "FATAL ERROR: 7\n"
                                     "Error: 8"),
                         log.getMessages());

    _testStringSequence (log, ts::Severity::Error);
    CPPUNIT_ASSERT(!log.emptyMessages());
    CPPUNIT_ASSERT_EQUAL(std::string("FATAL ERROR: 6\n"
                                     "FATAL ERROR: 7\n"
                                     "Error: 8"),
                         log.getMessages());

    _testStringSequence (log, ts::Severity::Fatal);
    CPPUNIT_ASSERT(!log.emptyMessages());
    CPPUNIT_ASSERT_EQUAL(std::string("FATAL ERROR: 6\n"
                                     "FATAL ERROR: 7"),
                         log.getMessages());

    _testStringSequence (log, ts::Severity::None);
    CPPUNIT_ASSERT(log.emptyMessages());
    CPPUNIT_ASSERT_EQUAL(std::string(""), log.getMessages());
}

// Log sequence for testPrintf
namespace {
    void _testPrintfSequence(ts::ReportBuffer<>& log, int level)
    {
        log.setDebugLevel(level);
        log.resetMessages();

        log.log(ts::Severity::Info, "%d", 1);
        log.debug("%d", 2);
        log.log(ts::Severity::Debug, "%d", 3);
        log.warning("%d", 4);
        log.info("%d", 5);
        log.fatal("%d", 6);
        log.log(ts::Severity::Fatal, "%d", 7);
        log.error("%d", 8);
    }
}

// Test case: log using printf-like formats
void ReportTest::testPrintf()
{
    ts::ReportBuffer<> log;

    _testPrintfSequence(log, ts::Severity::Debug);
    CPPUNIT_ASSERT_EQUAL(std::string("1\n"
                                     "Debug: 2\n"
                                     "Debug: 3\n"
                                     "Warning: 4\n"
                                     "5\n"
                                     "FATAL ERROR: 6\n"
                                     "FATAL ERROR: 7\n"
                                     "Error: 8"),
                         log.getMessages());

    _testPrintfSequence(log, ts::Severity::Info);
    CPPUNIT_ASSERT_EQUAL(std::string("1\n"
                                     "Warning: 4\n"
                                     "5\n"
                                     "FATAL ERROR: 6\n"
                                     "FATAL ERROR: 7\n"
                                     "Error: 8"),
                         log.getMessages());

    _testPrintfSequence(log, ts::Severity::Warning);
    CPPUNIT_ASSERT_EQUAL(std::string("Warning: 4\n"
                                     "FATAL ERROR: 6\n"
                                     "FATAL ERROR: 7\n"
                                     "Error: 8"),
                         log.getMessages());

    _testPrintfSequence(log, ts::Severity::Error);
    CPPUNIT_ASSERT_EQUAL(std::string("FATAL ERROR: 6\n"
                                     "FATAL ERROR: 7\n"
                                     "Error: 8"),
                         log.getMessages());

    _testPrintfSequence(log, ts::Severity::Fatal);
    CPPUNIT_ASSERT_EQUAL(std::string("FATAL ERROR: 6\n"
                                     "FATAL ERROR: 7"),
                         log.getMessages());

    _testPrintfSequence(log, ts::Severity::None);
    CPPUNIT_ASSERT(log.emptyMessages());
    CPPUNIT_ASSERT_EQUAL(std::string(""), log.getMessages());
}

// Test case: log file by name
void ReportTest::testByName()
{
    {
        ts::ReportFile<> log(_fileName, false, false);
        log.info("info 1");
        log.error("error 1");
    }

    ts::StringVector ref;
    ref.push_back("info 1");
    ref.push_back("Error: error 1");

    ts::StringVector value;
    ts::LoadStrings(value, _fileName);
    CPPUNIT_ASSERT(value == ref);

    {
        ts::ReportFile<> log(_fileName, true, false);
        log.info("info 2");
        log.error("error 2");
    }

    ref.push_back("info 2");
    ref.push_back("Error: error 2");

    ts::LoadStrings(value, _fileName);
    CPPUNIT_ASSERT(value == ref);
}

// Test case: log file on open stream
void ReportTest::testByStream()
{
    std::ofstream stream(_fileName.c_str());
    CPPUNIT_ASSERT(stream.is_open());

    {
        ts::ReportFile<> log(stream);
        log.info("info 1");
        log.error("error 1");
    }
    CPPUNIT_ASSERT(stream.is_open());

    stream.close();
    CPPUNIT_ASSERT(!stream.is_open());

    ts::StringVector ref;
    ref.push_back("info 1");
    ref.push_back("Error: error 1");

    ts::StringVector value;
    ts::LoadStrings(value, _fileName);
    CPPUNIT_ASSERT(value == ref);
}
