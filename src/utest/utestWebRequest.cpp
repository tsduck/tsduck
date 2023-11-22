//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::WebRequest.
//
//  Warning: these tests fail if there is no Internet connection or if
//  a proxy is required.
//
//----------------------------------------------------------------------------

#include "tsWebRequest.h"
#include "tsNullReport.h"
#include "tsCerrReport.h"
#include "tsReportBuffer.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class WebRequestTest: public tsunit::Test
{
public:
    WebRequestTest();

    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testGitHub();
    void testGoogle();
    void testReadMeFile();
    void testNoRedirection();
    void testNonExistentHost();
    void testInvalidURL();

    TSUNIT_TEST_BEGIN(WebRequestTest);
    TSUNIT_TEST(testGitHub);
    TSUNIT_TEST(testGoogle);
    TSUNIT_TEST(testReadMeFile);
    TSUNIT_TEST(testNoRedirection);
    TSUNIT_TEST(testNonExistentHost);
    TSUNIT_TEST(testInvalidURL);
    TSUNIT_TEST_END();

private:
    ts::UString _tempFileName;
    ts::Report& report();
    void testURL(const ts::UString& url, bool expectRedirection, bool expectSSL, bool expectTextContent, bool expectInvariant);
};

TSUNIT_REGISTER(WebRequestTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
WebRequestTest::WebRequestTest() :
    _tempFileName()
{
}

// Test suite initialization method.
void WebRequestTest::beforeTest()
{
    if (_tempFileName.empty()) {
        _tempFileName = ts::TempFile();
    }
    fs::remove(_tempFileName, &ts::ErrCodeReport(NULLREP));
}

// Test suite cleanup method.
void WebRequestTest::afterTest()
{
    fs::remove(_tempFileName, &ts::ErrCodeReport(NULLREP));
}

ts::Report& WebRequestTest::report()
{
    if (tsunit::Test::debugMode()) {
        CERR.setMaxSeverity(ts::Severity::Debug);
        return CERR;
    }
    else {
        return NULLREP;
    }
}


//----------------------------------------------------------------------------
// Test one URL.
//----------------------------------------------------------------------------

void WebRequestTest::testURL(const ts::UString& url, bool expectRedirection, bool expectSSL, bool expectTextContent, bool expectInvariant)
{
    ts::WebRequest request(report());

    // Test binary download
    ts::ByteBlock data;
    TSUNIT_ASSERT(request.downloadBinaryContent(url, data));

    debug() << "WebRequestTest::testURL:" << std::endl
            << "    Original URL: " << request.originalURL() << std::endl
            << "    Final URL: " << request.finalURL() << std::endl
            << "    HTTP status: " << request.httpStatus() << std::endl
            << "    Content size: " << request.contentSize() << std::endl;

    TSUNIT_ASSERT(!data.empty());
    TSUNIT_EQUAL(url, request.originalURL());
    TSUNIT_ASSERT(!request.finalURL().empty());
    if (expectRedirection) {
        TSUNIT_ASSERT(request.finalURL() != request.originalURL());
    }
    if (expectSSL) {
        TSUNIT_ASSERT(request.finalURL().startWith(u"https:"));
    }

    // Test text download.
    if (expectTextContent) {
        ts::UString text;
        TSUNIT_ASSERT(request.downloadTextContent(url, text));

        if (text.size() < 2048) {
            debug() << "WebRequestTest::testURL: downloaded text: " << text << std::endl;
        }

        TSUNIT_ASSERT(!text.empty());
        TSUNIT_EQUAL(url, request.originalURL());
        TSUNIT_ASSERT(!request.finalURL().empty());
        if (expectRedirection) {
            TSUNIT_ASSERT(request.finalURL() != request.originalURL());
        }
        if (expectSSL) {
            TSUNIT_ASSERT(request.finalURL().startWith(u"https:"));
        }
    }

    // Test file download
    TSUNIT_ASSERT(!ts::FileExists(_tempFileName));
    TSUNIT_ASSERT(request.downloadFile(url, _tempFileName));
    TSUNIT_ASSERT(ts::FileExists(_tempFileName));
    TSUNIT_EQUAL(url, request.originalURL());
    TSUNIT_ASSERT(!request.finalURL().empty());
    if (expectRedirection) {
        TSUNIT_ASSERT(request.finalURL() != request.originalURL());
    }
    if (expectSSL) {
        TSUNIT_ASSERT(request.finalURL().startWith(u"https:"));
    }

    // Load downloaded file.
    ts::ByteBlock fileContent;
    TSUNIT_ASSERT(fileContent.loadFromFile(_tempFileName, 10000000, &report()));
    debug() << "WebRequestTest::testURL: downloaded file size: " << fileContent.size() << std::endl;
    TSUNIT_ASSERT(!fileContent.empty());
    if (expectInvariant) {
        TSUNIT_ASSERT(fileContent == data);
    }
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void WebRequestTest::testGitHub()
{
    testURL(u"http://www.github.com/",
            true,     // expectRedirection
            true,     // expectSSL
            true,     // expectTextContent
            false);   // expectInvariant
}

void WebRequestTest::testGoogle()
{
    testURL(u"http://www.google.com/",
            false,    // expectRedirection
            false,    // expectSSL
            true,     // expectTextContent
            false);   // expectInvariant
}

void WebRequestTest::testReadMeFile()
{
    testURL(u"https://raw.githubusercontent.com/tsduck/tsduck/master/README.md",
            false,    // expectRedirection
            true,     // expectSSL
            true,     // expectTextContent
            true);    // expectInvariant
}

void WebRequestTest::testNoRedirection()
{
    ts::WebRequest request(report());
    request.setAutoRedirect(false);

    ts::ByteBlock data;
    TSUNIT_ASSERT(request.downloadBinaryContent(u"http://www.github.com/", data));

    debug() << "WebRequestTest::testNoRedirection:" << std::endl
            << "    Original URL: " << request.originalURL() << std::endl
            << "    Final URL: " << request.finalURL() << std::endl
            << "    HTTP status: " << request.httpStatus() << std::endl
            << "    Content size: " << request.contentSize() << std::endl;

    TSUNIT_EQUAL(3, request.httpStatus() / 100);
    TSUNIT_ASSERT(!request.finalURL().empty());
    TSUNIT_ASSERT(request.finalURL() != request.originalURL());
}

void WebRequestTest::testNonExistentHost()
{
    ts::ReportBuffer<> rep;
    ts::WebRequest request(rep);

    ts::ByteBlock data;
    TSUNIT_ASSERT(!request.downloadBinaryContent(u"http://non.existent.fake-domain/", data));
    
    debug() << "WebRequestTest::testNonExistentHost: " << rep.messages() << std::endl;
}

void WebRequestTest::testInvalidURL()
{
    ts::ReportBuffer<> rep;
    ts::WebRequest request(rep);

    ts::ByteBlock data;
    TSUNIT_ASSERT(!request.downloadBinaryContent(u"pouette://tagada/tsoin/tsoin", data));
    
    debug() << "WebRequestTest::testInvalidURL: " << rep.messages() << std::endl;
}
