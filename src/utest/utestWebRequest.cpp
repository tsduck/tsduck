//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
#include "tsReactiveWebRequest.h"
#include "tsNullReport.h"
#include "tsCerrReport.h"
#include "tsReportBuffer.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsjson.h"
#include "tsjsonValue.h"
#include "tsReactor.h"
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
    TSUNIT_DECLARE_TEST(Reactive);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    fs::path _temp_file_name {};
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
    if (_temp_file_name.empty()) {
        _temp_file_name = ts::TempFile();
    }
    fs::remove(_temp_file_name, &ts::ErrCodeReport());
}

// Test suite cleanup method.
void WebRequestTest::afterTest()
{
    fs::remove(_temp_file_name, &ts::ErrCodeReport());
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
    ts::WebRequest request(&report());

    debug() << "WebRequestTest::testURL: Test binary download: " << url << std::endl;

    ts::ByteBlockPtr data;
    TSUNIT_ASSERT(request.downloadBinaryContent(url, data));
    TSUNIT_ASSERT(data != nullptr);

    debug() << "WebRequestTest::testURL:" << std::endl
            << "    Original URL: " << request.status().originalURL() << std::endl
            << "    Final URL: " << request.status().finalURL() << std::endl
            << "    HTTP status: " << request.status().httpStatus() << std::endl
            << "    Content size: " << request.status().contentSize() << std::endl;

    TSUNIT_ASSERT(!data->empty());
    TSUNIT_EQUAL(url, request.status().originalURL());
    TSUNIT_ASSERT(!request.status().finalURL().empty());
    if (expectRedirection) {
        TSUNIT_ASSERT(request.status().finalURL() != request.status().originalURL());
    }
    if (expectSSL) {
        TSUNIT_ASSERT(request.status().finalURL().starts_with(u"https:"));
    }

    debug() << "WebRequestTest::testURL: Test text download" << std::endl;

    if (expectTextContent) {
        ts::UString text;
        TSUNIT_ASSERT(request.downloadTextContent(url, text));

        if (text.size() < 2048) {
            debug() << "WebRequestTest::testURL: downloaded text: " << text << std::endl;
        }

        TSUNIT_ASSERT(!text.empty());
        TSUNIT_EQUAL(url, request.status().originalURL());
        TSUNIT_ASSERT(!request.status().finalURL().empty());
        if (expectRedirection) {
            TSUNIT_ASSERT(request.status().finalURL() != request.status().originalURL());
        }
        if (expectSSL) {
            TSUNIT_ASSERT(request.status().finalURL().starts_with(u"https:"));
        }
    }

    debug() << "WebRequestTest::testURL: Test file download" << std::endl;

    TSUNIT_ASSERT(!fs::exists(_temp_file_name));
    TSUNIT_ASSERT(request.downloadFile(url, _temp_file_name));
    TSUNIT_ASSERT(fs::exists(_temp_file_name));
    TSUNIT_EQUAL(url, request.status().originalURL());
    TSUNIT_ASSERT(!request.status().finalURL().empty());
    if (expectRedirection) {
        TSUNIT_ASSERT(request.status().finalURL() != request.status().originalURL());
    }
    if (expectSSL) {
        TSUNIT_ASSERT(request.status().finalURL().starts_with(u"https:"));
    }

    // Load downloaded file.
    ts::ByteBlock fileContent;
    TSUNIT_ASSERT(fileContent.loadFromFile(_temp_file_name, 10000000, &report()));
    debug() << "WebRequestTest::testURL: downloaded file size: " << fileContent.size() << std::endl;
    TSUNIT_ASSERT(!fileContent.empty());
    if (expectInvariant) {
        TSUNIT_ASSERT(fileContent == *data);
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
    ts::WebRequest request(&report());
    request.args().setAutoRedirect(false);

    ts::ByteBlockPtr data;
    TSUNIT_ASSERT(request.downloadBinaryContent(u"http://www.github.com/", data));

    debug() << "WebRequestTest::testNoRedirection:" << std::endl
            << "    Original URL: " << request.status().originalURL() << std::endl
            << "    Final URL: " << request.status().finalURL() << std::endl
            << "    HTTP status: " << request.status().httpStatus() << std::endl
            << "    Content size: " << request.status().contentSize() << std::endl;
    for (const auto& h : request.status().responseHeaders()) {
        debug() << "    Header: " << h.first << " -> " << h.second << std::endl;
    }

    TSUNIT_EQUAL(3, request.status().httpStatus() / 100);
    TSUNIT_ASSERT(!request.status().finalURL().empty());
    TSUNIT_ASSERT(request.status().finalURL() != request.status().originalURL());
}

TSUNIT_DEFINE_TEST(NonExistentHost)
{
    ts::ReportBuffer<ts::ThreadSafety::None> rep;
    ts::WebRequest request(&rep);

    ts::ByteBlockPtr data;
    TSUNIT_ASSERT(!request.downloadBinaryContent(u"http://non.existent.fake-domain/", data));

    debug() << "WebRequestTest::testNonExistentHost: " << rep.messages() << std::endl;
}

TSUNIT_DEFINE_TEST(InvalidURL)
{
    ts::ReportBuffer<ts::ThreadSafety::None> rep;
    ts::WebRequest request(&rep);

    ts::ByteBlockPtr data;
    TSUNIT_ASSERT(!request.downloadBinaryContent(u"pouette://tagada/tsoin/tsoin", data));

    debug() << "WebRequestTest::testInvalidURL: " << rep.messages() << std::endl;
}

TSUNIT_DEFINE_TEST(Post)
{
    // These servers are known to return POST data into a JSON string.
    // 1. https://httpbin.org/post
    // 2. https://postman-echo.com/post
    const ts::UString url(u"https://postman-echo.com/post");
    const ts::UString post(u"foo bar\nqsdf=tif,dft=ty ryhrh=12,af\nfoo bar");

    ts::WebRequest request(&report());
    request.args().setPostData(post);

    // Use assumption instead of assertion because we do not fully trust the reliability to that site.
    ts::UString response;
    TSUNIT_ASSUME(request.downloadTextContent(url, response));

    debug() << "WebRequestTest::testPost:" << std::endl
            << "    Original URL: " << request.status().originalURL() << std::endl
            << "    Final URL: " << request.status().finalURL() << std::endl
            << "    HTTP status: " << request.status().httpStatus() << std::endl
            << "    Content size: " << request.status().contentSize() << std::endl
            << "    Content text: \"" << response << "\"" << std::endl;

    // Sometimes, these servers don't respond because they filter their load. So, ignore server errors.
    if (!request.status().httpServerError()) {
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
}


//----------------------------------------------------------------------------
// Test cases using reactors.
//----------------------------------------------------------------------------

namespace {
    class HandlerWeb: public ts::ReactiveWebHandlerInterface
    {
    private:
        ts::ReactiveWebRequest& _request; // just to check the address in handlers.

    public:
        bool open_called = false;
        bool receive_called = false;

        HandlerWeb(ts::ReactiveWebRequest& req) : _request(req) {}

        virtual void handleWebOpen(ts::ReactiveWebRequest& request, int error_code, const ts::ObjectPtr& user_data) override
        {
            tsunit::Test::debug() << "TestWebRequest::Reactive::handleWebOpen, error code: " << error_code << std::endl;
            open_called = true;
            TSUNIT_ASSERT(&request == &_request);
            TSUNIT_ASSERT(ts::SysSuccess(error_code));

            tsunit::Test::debug() << "    HTTP status: " << request.status().httpStatus() << std::endl;
            tsunit::Test::debug() << "    Original URL: \"" << request.status().originalURL() << "\"" << std::endl;
            tsunit::Test::debug() << "    Final URL: \"" << request.status().finalURL() << "\"" << std::endl;
            tsunit::Test::debug() << "    Announced content size: " << request.status().announcedContentSize() << std::endl;
            tsunit::Test::debug() << "    Content size: " << request.status().contentSize() << std::endl;
            tsunit::Test::debug() << "    MIME type: \"" << request.status().mimeType() << "\"" << std::endl;
            tsunit::Test::debug() << "    Header count: " << request.status().responseHeaders().size() << std::endl;
            for (const auto& h : request.status().responseHeaders()) {
                tsunit::Test::debug() << "    - " << h.first << ": " << h.second << std::endl;
            }
        }

        virtual void handleWebReceive(ts::ReactiveWebRequest& request, const ts::ByteBlockPtr& data, int error_code, const ts::ObjectPtr& user_data) override
        {
            receive_called = true;
            TSUNIT_ASSERT(&request == &_request);
            if (error_code == ts::SYS_EOF) {
                // End of transfer.
                tsunit::Test::debug() << "TestWebRequest::Reactive::handleWebReceive, end of session" << std::endl;
                tsunit::Test::debug() << "    Content size: " << request.status().contentSize() << std::endl;
                request.reactor().exitEventLoop();
            }
            else {
                TSUNIT_ASSERT(data != nullptr);
                tsunit::Test::debug() << "TestWebRequest::Reactive::handleWebReceive, error code: " << error_code << ", size: " << data->size() << std::endl;
                TSUNIT_EQUAL(ts::SYS_SUCCESS, error_code);
            }
        }
    };
}

TSUNIT_DEFINE_TEST(Reactive)
{
    ts::Reactor reactor(&CERR);
    ts::ReactiveWebRequest request(reactor);
    HandlerWeb test(request);

    const ts::UString url(ts::GetEnvironment(u"TSUNIT_REACTIVE_WEB_URL", u"http://tsduck.io"));
    const size_t buffer_size = ts::GetIntEnvironment(u"TSUNIT_REACTIVE_WEB_BUFFER_SIZE", ts::Device::DEFAULT_RECEIVE_BUFFER_SIZE);
    debug() << "WebRequestTest::Reactive: downloading " << url << ", buffer size: " << buffer_size << std::endl;

    TSUNIT_ASSERT(!reactor.isOpen());
    TSUNIT_ASSERT(reactor.open());
    TSUNIT_ASSERT(reactor.isOpen());

    TSUNIT_ASSERT(request.start(&test, url, buffer_size));
    TSUNIT_ASSERT(reactor.processEventLoop());
    TSUNIT_ASSERT(test.open_called);
    TSUNIT_ASSERT(test.receive_called);

    TSUNIT_ASSERT(reactor.isOpen());
    TSUNIT_ASSERT(reactor.close());
    TSUNIT_ASSERT(!reactor.isOpen());
}
