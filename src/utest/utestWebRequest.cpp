//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsjson.h"
#include "tsjsonValue.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class WebRequestTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(GitHub);
    TSUNIT_DECLARE_TEST(Google);
    TSUNIT_DECLARE_TEST(ReadMeFile);
    TSUNIT_DECLARE_TEST(NoRedirection);
    TSUNIT_DECLARE_TEST(NonExistentHost);
    TSUNIT_DECLARE_TEST(InvalidURL);
    TSUNIT_DECLARE_TEST(Post);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    fs::path _tempFileName {};
    ts::Report& report();
    void testURL(const ts::UString& url, bool expectRedirection, bool expectSSL, bool expectTextContent, bool expectInvariant);
};

TSUNIT_REGISTER(WebRequestTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void WebRequestTest::beforeTest()
{
    if (_tempFileName.empty()) {
        _tempFileName = ts::TempFile();
    }
    fs::remove(_tempFileName, &ts::ErrCodeReport());
}

// Test suite cleanup method.
void WebRequestTest::afterTest()
{
    fs::remove(_tempFileName, &ts::ErrCodeReport());
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
        TSUNIT_ASSERT(request.finalURL().starts_with(u"https:"));
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
            TSUNIT_ASSERT(request.finalURL().starts_with(u"https:"));
        }
    }

    // Test file download
    TSUNIT_ASSERT(!fs::exists(_tempFileName));
    TSUNIT_ASSERT(request.downloadFile(url, _tempFileName));
    TSUNIT_ASSERT(fs::exists(_tempFileName));
    TSUNIT_EQUAL(url, request.originalURL());
    TSUNIT_ASSERT(!request.finalURL().empty());
    if (expectRedirection) {
        TSUNIT_ASSERT(request.finalURL() != request.originalURL());
    }
    if (expectSSL) {
        TSUNIT_ASSERT(request.finalURL().starts_with(u"https:"));
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

TSUNIT_DEFINE_TEST(GitHub)
{
    testURL(u"http://www.github.com/",
            true,     // expectRedirection
            true,     // expectSSL
            true,     // expectTextContent
            false);   // expectInvariant
}

TSUNIT_DEFINE_TEST(Google)
{
    testURL(u"http://www.google.com/",
            false,    // expectRedirection
            false,    // expectSSL
            true,     // expectTextContent
            false);   // expectInvariant
}

TSUNIT_DEFINE_TEST(ReadMeFile)
{
    testURL(u"https://raw.githubusercontent.com/tsduck/tsduck/master/README.md",
            false,    // expectRedirection
            true,     // expectSSL
            true,     // expectTextContent
            true);    // expectInvariant
}

TSUNIT_DEFINE_TEST(NoRedirection)
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

TSUNIT_DEFINE_TEST(NonExistentHost)
{
    ts::ReportBuffer<ts::ThreadSafety::None> rep;
    ts::WebRequest request(rep);

    ts::ByteBlock data;
    TSUNIT_ASSERT(!request.downloadBinaryContent(u"http://non.existent.fake-domain/", data));

    debug() << "WebRequestTest::testNonExistentHost: " << rep.messages() << std::endl;
}

TSUNIT_DEFINE_TEST(InvalidURL)
{
    ts::ReportBuffer<ts::ThreadSafety::None> rep;
    ts::WebRequest request(rep);

    ts::ByteBlock data;
    TSUNIT_ASSERT(!request.downloadBinaryContent(u"pouette://tagada/tsoin/tsoin", data));

    debug() << "WebRequestTest::testInvalidURL: " << rep.messages() << std::endl;
}

TSUNIT_DEFINE_TEST(Post)
{
    // These servers are known to return POST data into a JSON string.
    // 1. https://httpbin.org/post
    // 2. https://postman-echo.com/post
    const ts::UString url(u"https://httpbin.org/post");
    const ts::UString post(u"foo bar\nqsdf=tif,dft=ty ryhrh=12,af\nfoo bar");

    ts::WebRequest request(report());
    request.setPostData(post);

    // Use assumption instead of assertion because we do not fully trust the reliability to that site.
    ts::UString response;
    TSUNIT_ASSUME(request.downloadTextContent(url, response));

    debug() << "WebRequestTest::testPost:" << std::endl
            << "    Original URL: " << request.originalURL() << std::endl
            << "    Final URL: " << request.finalURL() << std::endl
            << "    HTTP status: " << request.httpStatus() << std::endl
            << "    Content size: " << request.contentSize() << std::endl
            << "    Content text: \"" << response << "\"" << std::endl;

    ts::json::ValuePtr jv;
    bool success = true;
    TSUNIT_ASSUME(success = ts::json::Parse(jv, response, CERR));
    if (success) {
        TSUNIT_ASSERT(jv != nullptr);
        TSUNIT_ASSERT(jv->isObject());
        TSUNIT_ASSERT(jv->value(u"data").isString());
        TSUNIT_EQUAL(post, jv->value(u"data").toString());
    }
}
