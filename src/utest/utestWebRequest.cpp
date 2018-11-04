//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  CppUnit test suite for class ts::WebRequest.
//
//  Warning: these tests fail if there is no Internet connection or if
//  a proxy is required.
//
//----------------------------------------------------------------------------

#include "tsWebRequest.h"
#include "tsNullReport.h"
#include "tsCerrReport.h"
#include "tsReportBuffer.h"
#include "tsSysUtils.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class WebRequestTest: public CppUnit::TestFixture
{
public:
    WebRequestTest();

    virtual void setUp() override;
    virtual void tearDown() override;

    void testGitHub();
    void testGoogle();
    void testReadMeFile();
    void testNoRedirection();
    void testNonExistentHost();
    void testInvalidURL();

    CPPUNIT_TEST_SUITE(WebRequestTest);
    CPPUNIT_TEST(testGitHub);
    CPPUNIT_TEST(testGoogle);
    CPPUNIT_TEST(testReadMeFile);
    CPPUNIT_TEST(testNoRedirection);
    CPPUNIT_TEST(testNonExistentHost);
    CPPUNIT_TEST(testInvalidURL);
    CPPUNIT_TEST_SUITE_END();

private:
    ts::UString _tempFileName;
    ts::Report& report();
    void testURL(const ts::UString& url, bool expectRedirection, bool expectSSL, bool expectTextContent, bool expectInvariant);
};

CPPUNIT_TEST_SUITE_REGISTRATION (WebRequestTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
WebRequestTest::WebRequestTest() :
    _tempFileName(ts::TempFile())
{
}

// Test suite initialization method.
void WebRequestTest::setUp()
{
    ts::DeleteFile(_tempFileName);
}

// Test suite cleanup method.
void WebRequestTest::tearDown()
{
    ts::DeleteFile(_tempFileName);
}

ts::Report& WebRequestTest::report()
{
    if (utest::DebugMode()) {
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
    request.setURL(url);
    CPPUNIT_ASSERT(request.downloadBinaryContent(data));

    utest::Out() << "WebRequestTest::testURL:" << std::endl
                 << "    Original URL: " << request.originalURL() << std::endl
                 << "    Final URL: " << request.finalURL() << std::endl
                 << "    HTTP status: " << request.httpStatus() << std::endl
                 << "    Content size: " << request.contentSize() << std::endl;

    CPPUNIT_ASSERT(!data.empty());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(url, request.originalURL());
    CPPUNIT_ASSERT(!request.finalURL().empty());
    if (expectRedirection) {
        CPPUNIT_ASSERT(request.finalURL() != request.originalURL());
    }
    if (expectSSL) {
        CPPUNIT_ASSERT(request.finalURL().startWith(u"https:"));
    }

    // Reset URL's.
    request.setURL(ts::UString());
    CPPUNIT_ASSERT(request.originalURL().empty());
    CPPUNIT_ASSERT(request.finalURL().empty());

    // Test text download.
    if (expectTextContent) {
        ts::UString text;
        request.setURL(url);
        CPPUNIT_ASSERT(request.downloadTextContent(text));

        if (text.size() < 2048) {
            utest::Out() << "WebRequestTest::testURL: downloaded text: " << text << std::endl;
        }

        CPPUNIT_ASSERT(!text.empty());
        CPPUNIT_ASSERT_USTRINGS_EQUAL(url, request.originalURL());
        CPPUNIT_ASSERT(!request.finalURL().empty());
        if (expectRedirection) {
            CPPUNIT_ASSERT(request.finalURL() != request.originalURL());
        }
        if (expectSSL) {
            CPPUNIT_ASSERT(request.finalURL().startWith(u"https:"));
        }

        // Reset URL's.
        request.setURL(ts::UString());
        CPPUNIT_ASSERT(request.originalURL().empty());
        CPPUNIT_ASSERT(request.finalURL().empty());
    }

    // Test file download
    request.setURL(url);
    CPPUNIT_ASSERT(!ts::FileExists(_tempFileName));
    CPPUNIT_ASSERT(request.downloadFile(_tempFileName));
    CPPUNIT_ASSERT(ts::FileExists(_tempFileName));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(url, request.originalURL());
    CPPUNIT_ASSERT(!request.finalURL().empty());
    if (expectRedirection) {
        CPPUNIT_ASSERT(request.finalURL() != request.originalURL());
    }
    if (expectSSL) {
        CPPUNIT_ASSERT(request.finalURL().startWith(u"https:"));
    }

    // Load downloaded file.
    ts::ByteBlock fileContent;
    CPPUNIT_ASSERT(fileContent.loadFromFile(_tempFileName, 10000000, &report()));
    utest::Out() << "WebRequestTest::testURL: downloaded file size: " << fileContent.size() << std::endl;
    CPPUNIT_ASSERT(!fileContent.empty());
    if (expectInvariant) {
        CPPUNIT_ASSERT(fileContent == data);
    }

    // Reset URL's.
    request.setURL(ts::UString());
    CPPUNIT_ASSERT(request.originalURL().empty());
    CPPUNIT_ASSERT(request.finalURL().empty());

    // Test with application callback.
    class Transfer : public ts::WebRequestHandlerInterface
    {
    public:
        ts::ByteBlock data;

        Transfer(): data() {}

        virtual bool handleWebStart(const ts::WebRequest& req, size_t size) override
        {
            utest::Out() << "WebRequestTest::handleWebStart: size: " << size << std::endl;
            return true;
        }

        virtual bool handleWebData(const ts::WebRequest& req, const void* addr, size_t size) override
        {
            data.append(addr, size);
            return true;
        }
    };

    Transfer transfer;
    request.setURL(url);
    CPPUNIT_ASSERT(request.downloadToApplication(&transfer));
    utest::Out() << "WebRequestTest::testURL: downloaded size by callback: " << transfer.data.size() << std::endl;
    CPPUNIT_ASSERT(!transfer.data.empty());
    if (expectInvariant) {
        CPPUNIT_ASSERT(transfer.data == data);
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
    request.setURL(u"http://www.github.com/");
    request.setAutoRedirect(false);

    ts::ByteBlock data;
    CPPUNIT_ASSERT(request.downloadBinaryContent(data));

    utest::Out() << "WebRequestTest::testNoRedirection:" << std::endl
        << "    Original URL: " << request.originalURL() << std::endl
        << "    Final URL: " << request.finalURL() << std::endl
        << "    HTTP status: " << request.httpStatus() << std::endl
        << "    Content size: " << request.contentSize() << std::endl;

    CPPUNIT_ASSERT_EQUAL(3, request.httpStatus() / 100);
    CPPUNIT_ASSERT(!request.finalURL().empty());
    CPPUNIT_ASSERT(request.finalURL() != request.originalURL());
}

void WebRequestTest::testNonExistentHost()
{
    ts::ReportBuffer<> rep;
    ts::WebRequest request(rep);

    ts::ByteBlock data;
    request.setURL(u"http://non.existent.fake-domain/");
    CPPUNIT_ASSERT(!request.downloadBinaryContent(data));

    utest::Out() << "WebRequestTest::testNonExistentHost: " << rep.getMessages() << std::endl;
}

void WebRequestTest::testInvalidURL()
{
    ts::ReportBuffer<> rep;
    ts::WebRequest request(rep);

    ts::ByteBlock data;
    request.setURL(u"pouette://tagada/tsoin/tsoin");
    CPPUNIT_ASSERT(!request.downloadBinaryContent(data));

    utest::Out() << "WebRequestTest::testInvalidURL: " << rep.getMessages() << std::endl;
}
