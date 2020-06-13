//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//  TSUnit test suite for JSON classes.
//
//----------------------------------------------------------------------------

#include "tsjson.h"
#include "tsjsonNull.h"
#include "tsjsonTrue.h"
#include "tsjsonFalse.h"
#include "tsjsonValue.h"
#include "tsjsonNumber.h"
#include "tsjsonString.h"
#include "tsjsonObject.h"
#include "tsjsonArray.h"
#include "tsCerrReport.h"
#include "tsNullReport.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class JsonTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testSimple();
    void testGitHub();

    TSUNIT_TEST_BEGIN(JsonTest);
    TSUNIT_TEST(testSimple);
    TSUNIT_TEST(testGitHub);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(JsonTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void JsonTest::beforeTest()
{
}

// Test suite cleanup method.
void JsonTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void JsonTest::testSimple()
{
    ts::json::ValuePtr jv;
    TSUNIT_ASSERT(!ts::json::Parse(jv, u"", NULLREP));
    TSUNIT_ASSERT(jv.isNull());

    TSUNIT_ASSERT(ts::json::Parse(jv, u" null  ", CERR));
    TSUNIT_ASSERT(!jv.isNull());
    TSUNIT_ASSERT(jv->isNull());

    TSUNIT_ASSERT(!ts::json::Parse(jv, u"   false  true  ", NULLREP));
    TSUNIT_ASSERT(!jv.isNull());
    TSUNIT_ASSERT(jv->isFalse());

    TSUNIT_ASSERT(ts::json::Parse(jv, u"[ true, {\"ab\":67, \"foo\" : \"bar\"} ]", CERR));
    TSUNIT_ASSERT(!jv.isNull());
    TSUNIT_ASSERT(jv->isArray());
    TSUNIT_EQUAL(2, jv->size());
    TSUNIT_ASSERT(jv->at(0).isTrue());
    TSUNIT_ASSERT(jv->at(1).isObject());
    TSUNIT_ASSERT(jv->at(2).isNull());
    TSUNIT_ASSERT(jv->at(2).value(u"jjj").at(3424).isNull());
    TSUNIT_EQUAL(2, jv->at(1).size());
    TSUNIT_EQUAL(67, jv->at(1).value(u"ab").toInteger());
    TSUNIT_EQUAL(u"bar", jv->at(1).value(u"foo").toString());
    TSUNIT_ASSERT(jv->at(1).value(u"ss").isNull());

    TSUNIT_EQUAL(
        u"[\n"
        u"  true,\n"
        u"  {\n"
        u"    \"ab\": 67,\n"
        u"    \"foo\": \"bar\"\n"
        u"  }\n"
        u"]",
        jv->printed());
}

void JsonTest::testGitHub()
{
    // Typical response from GitHub:
    static const ts::UChar* response =
        u"{\n"
        u"  \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/9074329\",\n"
        u"  \"assets_url\": \"https://api.github.com/repos/tsduck/tsduck/releases/9074329/assets\",\n"
        u"  \"upload_url\": \"https://uploads.github.com/repos/tsduck/tsduck/releases/9074329/assets{?name,label}\",\n"
        u"  \"html_url\": \"https://github.com/tsduck/tsduck/releases/tag/v3.5-419\",\n"
        u"  \"id\": 9074329,\n"
        u"  \"tag_name\": \"v3.5-419\",\n"
        u"  \"target_commitish\": \"master\",\n"
        u"  \"name\": \"Version 3.5-419\",\n"
        u"  \"draft\": false,\n"
        u"  \"author\": {\n"
        u"    \"login\": \"lelegard\",\n"
        u"    \"id\": 5641922,\n"
        u"    \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"    \"gravatar_id\": \"\",\n"
        u"    \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"    \"html_url\": \"https://github.com/lelegard\",\n"
        u"    \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"    \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"    \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"    \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"    \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"    \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"    \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"    \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"    \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"    \"type\": \"User\",\n"
        u"    \"site_admin\": false\n"
        u"  },\n"
        u"  \"prerelease\": false,\n"
        u"  \"created_at\": \"2018-01-01T18:42:41Z\",\n"
        u"  \"published_at\": \"2018-01-01T22:34:10Z\",\n"
        u"  \"assets\": [\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754862\",\n"
        u"      \"id\": 5754862,\n"
        u"      \"name\": \"tsduck-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"audio/x-pn-realaudio-plugin\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 5973796,\n"
        u"      \"download_count\": 3,\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:47Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-3.5-419.fc27.x86_64.rpm\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754864\",\n"
        u"      \"id\": 5754864,\n"
        u"      \"name\": \"tsduck-devel-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"audio/x-pn-realaudio-plugin\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 3985044,\n"
        u"      \"download_count\": 0,\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:48Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-devel-3.5-419.fc27.x86_64.rpm\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754863\",\n"
        u"      \"id\": 5754863,\n"
        u"      \"name\": \"tsduck-dev_3.5-419_amd64.deb\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"application/x-deb\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 3337710,\n"
        u"      \"download_count\": 0,\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:48Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-dev_3.5-419_amd64.deb\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754865\",\n"
        u"      \"id\": 5754865,\n"
        u"      \"name\": \"TSDuck-Win32-3.5-419.exe\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"application/octet-stream\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 35325653,\n"
        u"      \"download_count\": 1,\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:56Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win32-3.5-419.exe\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754866\",\n"
        u"      \"id\": 5754866,\n"
        u"      \"name\": \"TSDuck-Win64-3.5-419.exe\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"application/octet-stream\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 36267256,\n"
        u"      \"download_count\": 3,\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:34:06Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win64-3.5-419.exe\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754861\",\n"
        u"      \"id\": 5754861,\n"
        u"      \"name\": \"tsduck_3.5-419_amd64.deb\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"application/x-deb\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 3975010,\n"
        u"      \"download_count\": 1,\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:46Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck_3.5-419_amd64.deb\"\n"
        u"    }\n"
        u"  ],\n"
        u"  \"tarball_url\": \"https://api.github.com/repos/tsduck/tsduck/tarball/v3.5-419\",\n"
        u"  \"zipball_url\": \"https://api.github.com/repos/tsduck/tsduck/zipball/v3.5-419\",\n"
        u"  \"body\": \"Binaries for command-line tools and plugins:\\r\\n* Windows 32 bits: [TSDuck-Win32-3.5-419.exe](https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win32-3.5-419.exe)\\r\\n* Windows 64 bits: [TSDuck-Win64-3.5-419.exe](https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win64-3.5-419.exe)\\r\\n* Fedora 64 bits: [tsduck-3.5-419.fc27.x86_64.rpm](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-3.5-419.fc27.x86_64.rpm)\\r\\n* Ubuntu 64 bits: [tsduck_3.5-419_amd64.deb](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck_3.5-419_amd64.deb)\\r\\n* macOS: [use Homebrew](https://github.com/tsduck/homebrew-tsduck/blob/master/README.md)\\r\\n\\r\\nBinaries for development environment:\\r\\n* Windows: Included in installer (select option \\\"Development\\\")\\r\\n* Fedora 64 bits: [tsduck-devel-3.5-419.fc27.x86_64.rpm](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-devel-3.5-419.fc27.x86_64.rpm)\\r\\n* Ubuntu 64 bits: [tsduck-dev_3.5-419_amd64.deb](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-dev_3.5-419_amd64.deb)\\r\\n* macOS: Included in Homebrew package\\r\\n\"\n"
        u"}\n";

    ts::json::ValuePtr jv;
    TSUNIT_ASSERT(ts::json::Parse(jv, response, CERR));
    TSUNIT_ASSERT(!jv.isNull());
    TSUNIT_ASSERT(jv->isObject());
    TSUNIT_EQUAL(u"v3.5-419", jv->value(u"tag_name").toString());
    TSUNIT_EQUAL(u"https://api.github.com/repos/tsduck/tsduck/tarball/v3.5-419", jv->value(u"tarball_url").toString());
    TSUNIT_EQUAL(u"lelegard", jv->value(u"author").value(u"login").toString());
    TSUNIT_EQUAL(u"tsduck-devel-3.5-419.fc27.x86_64.rpm", jv->value(u"assets").at(1).value(u"name").toString());

    // Same as input but names are sorted in objects.
    TSUNIT_EQUAL(
        u"{\n"
        u"  \"assets\": [\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"content_type\": \"audio/x-pn-realaudio-plugin\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"download_count\": 3,\n"
        u"      \"id\": 5754862,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"tsduck-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"size\": 5973796,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:47Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754862\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-devel-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"content_type\": \"audio/x-pn-realaudio-plugin\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"download_count\": 0,\n"
        u"      \"id\": 5754864,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"tsduck-devel-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"size\": 3985044,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:48Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754864\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-dev_3.5-419_amd64.deb\",\n"
        u"      \"content_type\": \"application/x-deb\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"download_count\": 0,\n"
        u"      \"id\": 5754863,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"tsduck-dev_3.5-419_amd64.deb\",\n"
        u"      \"size\": 3337710,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:48Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754863\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win32-3.5-419.exe\",\n"
        u"      \"content_type\": \"application/octet-stream\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"download_count\": 1,\n"
        u"      \"id\": 5754865,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"TSDuck-Win32-3.5-419.exe\",\n"
        u"      \"size\": 35325653,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:56Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754865\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win64-3.5-419.exe\",\n"
        u"      \"content_type\": \"application/octet-stream\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"download_count\": 3,\n"
        u"      \"id\": 5754866,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"TSDuck-Win64-3.5-419.exe\",\n"
        u"      \"size\": 36267256,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:34:06Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754866\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck_3.5-419_amd64.deb\",\n"
        u"      \"content_type\": \"application/x-deb\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"download_count\": 1,\n"
        u"      \"id\": 5754861,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"tsduck_3.5-419_amd64.deb\",\n"
        u"      \"size\": 3975010,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:46Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754861\"\n"
        u"    }\n"
        u"  ],\n"
        u"  \"assets_url\": \"https://api.github.com/repos/tsduck/tsduck/releases/9074329/assets\",\n"
        u"  \"author\": {\n"
        u"    \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"    \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"    \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"    \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"    \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"    \"gravatar_id\": \"\",\n"
        u"    \"html_url\": \"https://github.com/lelegard\",\n"
        u"    \"id\": 5641922,\n"
        u"    \"login\": \"lelegard\",\n"
        u"    \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"    \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"    \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"    \"site_admin\": false,\n"
        u"    \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"    \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"    \"type\": \"User\",\n"
        u"    \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"  },\n"
        u"  \"body\": \"Binaries for command-line tools and plugins:\\r\\n* Windows 32 bits: [TSDuck-Win32-3.5-419.exe](https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win32-3.5-419.exe)\\r\\n* Windows 64 bits: [TSDuck-Win64-3.5-419.exe](https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win64-3.5-419.exe)\\r\\n* Fedora 64 bits: [tsduck-3.5-419.fc27.x86_64.rpm](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-3.5-419.fc27.x86_64.rpm)\\r\\n* Ubuntu 64 bits: [tsduck_3.5-419_amd64.deb](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck_3.5-419_amd64.deb)\\r\\n* macOS: [use Homebrew](https://github.com/tsduck/homebrew-tsduck/blob/master/README.md)\\r\\n\\r\\nBinaries for development environment:\\r\\n* Windows: Included in installer (select option \\\"Development\\\")\\r\\n* Fedora 64 bits: [tsduck-devel-3.5-419.fc27.x86_64.rpm](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-devel-3.5-419.fc27.x86_64.rpm)\\r\\n* Ubuntu 64 bits: [tsduck-dev_3.5-419_amd64.deb](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-dev_3.5-419_amd64.deb)\\r\\n* macOS: Included in Homebrew package\\r\\n\",\n"
        u"  \"created_at\": \"2018-01-01T18:42:41Z\",\n"
        u"  \"draft\": false,\n"
        u"  \"html_url\": \"https://github.com/tsduck/tsduck/releases/tag/v3.5-419\",\n"
        u"  \"id\": 9074329,\n"
        u"  \"name\": \"Version 3.5-419\",\n"
        u"  \"prerelease\": false,\n"
        u"  \"published_at\": \"2018-01-01T22:34:10Z\",\n"
        u"  \"tag_name\": \"v3.5-419\",\n"
        u"  \"tarball_url\": \"https://api.github.com/repos/tsduck/tsduck/tarball/v3.5-419\",\n"
        u"  \"target_commitish\": \"master\",\n"
        u"  \"upload_url\": \"https://uploads.github.com/repos/tsduck/tsduck/releases/9074329/assets{?name,label}\",\n"
        u"  \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/9074329\",\n"
        u"  \"zipball_url\": \"https://api.github.com/repos/tsduck/tsduck/zipball/v3.5-419\"\n"
        u"}",
        jv->printed());
}
