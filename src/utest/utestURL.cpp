//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class URL
//
//----------------------------------------------------------------------------

#include "tsURL.h"
#include "tsIPAddress.h"
#include "tsCerrReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class URLTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(IsURL);
    TSUNIT_DECLARE_TEST(Parse);
    TSUNIT_DECLARE_TEST(Query);
    TSUNIT_DECLARE_TEST(Base);
    TSUNIT_DECLARE_TEST(ToString);
    TSUNIT_DECLARE_TEST(ToRelative);
    TSUNIT_DECLARE_TEST(IPv6);
};

TSUNIT_REGISTER(URLTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(IsURL)
{
    TSUNIT_ASSERT(!ts::URL::IsURL(u""));
    TSUNIT_ASSERT(!ts::URL::IsURL(u"foo/bar"));
    TSUNIT_ASSERT(!ts::URL::IsURL(u"C:/foo/bar"));

    TSUNIT_ASSERT(ts::URL::IsURL(u"http://foo/bar"));
    TSUNIT_ASSERT(ts::URL::IsURL(u"file:///foo/bar"));
    TSUNIT_ASSERT(ts::URL::IsURL(u"file:///C:/foo/bar"));
    TSUNIT_ASSERT(ts::URL::IsURL(u"file://C:/foo/bar"));
}

TSUNIT_DEFINE_TEST(Parse)
{
    ts::URL url1(u"http://user:pwd@host.name:1234/foo/bar/?query+args#frag");
    TSUNIT_ASSERT(url1.isValid());
    TSUNIT_EQUAL(u"http", url1.getScheme());
    TSUNIT_EQUAL(u"user", url1.getUserName());
    TSUNIT_EQUAL(u"pwd", url1.getPassword());
    TSUNIT_EQUAL(u"host.name", url1.getHost());
    TSUNIT_EQUAL(1234, url1.getPort());
    TSUNIT_EQUAL(u"/foo/bar/", url1.getPath());
    TSUNIT_EQUAL(u"query+args", url1.getQuery());
    TSUNIT_EQUAL(u"frag", url1.getFragment());

    ts::URL url2(u"foo://host/bar/boo");
    TSUNIT_ASSERT(url2.isValid());
    TSUNIT_EQUAL(u"foo", url2.getScheme());
    TSUNIT_EQUAL(u"", url2.getUserName());
    TSUNIT_EQUAL(u"", url2.getPassword());
    TSUNIT_EQUAL(u"host", url2.getHost());
    TSUNIT_EQUAL(0, url2.getPort());
    TSUNIT_EQUAL(u"/bar/boo", url2.getPath());
    TSUNIT_EQUAL(u"", url2.getQuery());
    TSUNIT_EQUAL(u"", url2.getFragment());
}

TSUNIT_DEFINE_TEST(Query)
{
    ts::URL url(u"http://host.name:1234/foo/bar?ze=kve&ab=cd#frag");
    TSUNIT_ASSERT(url.isValid());
    TSUNIT_EQUAL(u"http", url.getScheme());
    TSUNIT_EQUAL(u"host.name", url.getHost());
    TSUNIT_EQUAL(1234, url.getPort());
    TSUNIT_EQUAL(u"/foo/bar", url.getPath());
    TSUNIT_EQUAL(u"ze=kve&ab=cd", url.getQuery());
    TSUNIT_EQUAL(u"frag", url.getFragment());

    static const std::vector<std::pair<ts::UString, ts::UString>> qref = {
        {u"ab", u"cd"},
        {u"ze", u"kve"},
    };

    TSUNIT_EQUAL(qref.size(), url.getQueryParameters().size());
    size_t qref_index = 0;
    for (const auto& q : url.getQueryParameters()) {
        TSUNIT_EQUAL(qref[qref_index].first, q.first);
        TSUNIT_EQUAL(qref[qref_index].second, q.second);
        qref_index++;
    }

    url.clear();
    url.setURL(u"http://:1234?ze=kve&ab=cd#frag2");
    TSUNIT_ASSERT(url.isValid());
    TSUNIT_EQUAL(u"http", url.getScheme());
    TSUNIT_EQUAL(u"", url.getHost());
    TSUNIT_EQUAL(1234, url.getPort());
    TSUNIT_EQUAL(u"", url.getPath());
    TSUNIT_EQUAL(u"ze=kve&ab=cd", url.getQuery());
    TSUNIT_EQUAL(u"frag2", url.getFragment());
    TSUNIT_EQUAL(qref.size(), url.getQueryParameters().size());
    qref_index = 0;
    for (const auto& q : url.getQueryParameters()) {
        TSUNIT_EQUAL(qref[qref_index].first, q.first);
        TSUNIT_EQUAL(qref[qref_index].second, q.second);
        qref_index++;
    }
}

TSUNIT_DEFINE_TEST(Base)
{
    TSUNIT_EQUAL(u"http://foo.com/bar/abc/def", ts::URL(u"abc/def", u"http://foo.com/bar/cool").toString());
    TSUNIT_EQUAL(u"http://foo.com/bar/cool/abc/def", ts::URL(u"abc/def", u"http://foo.com/bar/cool/").toString());
    TSUNIT_EQUAL(u"http://foo.com/abc/def", ts::URL(u"/abc/def", u"http://foo.com/bar/cool/").toString());
    TSUNIT_EQUAL(u"http://foo.com/bar/abc/def", ts::URL(u"../../abc/def", u"http://foo.com/bar/cool/taf/").toString());
}

TSUNIT_DEFINE_TEST(ToString)
{
    TSUNIT_EQUAL(u"http://foo.bar/", ts::URL(u"http://foo.bar").toString());
    TSUNIT_EQUAL(u"http://foo.bar/a/d/e", ts::URL(u"http://foo.bar/a/b/c/../../d/e").toString());

#if defined(TS_WINDOWS)
    TSUNIT_EQUAL(u"file://C:/ab/cd/ef", ts::URL(u"C:\\ab\\cd\\ef").toString());
    TSUNIT_EQUAL(u"file://C:/ab/cd/ef", ts::URL(u"C:\\ab\\cd\\ef").toString(true));
    TSUNIT_EQUAL(u"file:///C:/ab/cd/ef", ts::URL(u"C:\\ab\\cd\\ef").toString(false));
    TSUNIT_EQUAL(u"file://C:/ab/cd/ef", ts::URL(u"ef", u"C:\\ab\\cd\\").toString());
    TSUNIT_EQUAL(u"file://C:/ab/ef", ts::URL(u"ef", u"C:\\ab\\cd").toString());
#else
    TSUNIT_EQUAL(u"file:///ab/cd/ef", ts::URL(u"/ab/cd/ef").toString());
    TSUNIT_EQUAL(u"file:///ab/ef", ts::URL(u"ef", u"/ab/cd").toString());
    TSUNIT_EQUAL(u"file:///ab/cd/ef", ts::URL(u"ef", u"/ab/cd/").toString());
#endif
}

TSUNIT_DEFINE_TEST(ToRelative)
{
    TSUNIT_EQUAL(u"http://foo.bar/abc/def", ts::URL(u"http://foo.bar/abc/def").toRelative(u"http://foo.car/abc/def"));
    TSUNIT_EQUAL(u"/abc/def", ts::URL(u"http://foo.bar/abc/def").toRelative(u"http://foo.bar/xyz/def"));
    TSUNIT_EQUAL(u"def", ts::URL(u"http://foo.bar/abc/def").toRelative(u"http://foo.bar/abc/"));
    TSUNIT_EQUAL(u"abc/def", ts::URL(u"http://foo.bar/abc/def").toRelative(u"http://foo.bar/abc"));
}

TSUNIT_DEFINE_TEST(IPv6)
{
    ts::URL url(u"http://[::]");
    TSUNIT_ASSERT(url.isValid());
    TSUNIT_EQUAL(u"http", url.getScheme());
    TSUNIT_EQUAL(u"", url.getUserName());
    TSUNIT_EQUAL(u"", url.getPassword());
    TSUNIT_EQUAL(u"[::]", url.getHost());
    TSUNIT_EQUAL(0, url.getPort());
    TSUNIT_EQUAL(u"", url.getPath());
    TSUNIT_EQUAL(u"", url.getQuery());
    TSUNIT_EQUAL(u"", url.getFragment());

    ts::IPAddress a;
    TSUNIT_ASSERT(a.resolve(url.getHost(), CERR));
    TSUNIT_EQUAL(u"::", a.toString());
    TSUNIT_EQUAL(u"0000:0000:0000:0000:0000:0000:0000:0000", a.toFullString());

    url.clear();
    url.setURL(u"http://user:pass@[fe80::93a3:dea0:2108:b81e]:5678/foo?que#fr");
    TSUNIT_ASSERT(url.isValid());
    TSUNIT_EQUAL(u"http", url.getScheme());
    TSUNIT_EQUAL(u"user", url.getUserName());
    TSUNIT_EQUAL(u"pass", url.getPassword());
    TSUNIT_EQUAL(u"[fe80::93a3:dea0:2108:b81e]", url.getHost());
    TSUNIT_EQUAL(5678, url.getPort());
    TSUNIT_EQUAL(u"/foo", url.getPath());
    TSUNIT_EQUAL(u"que", url.getQuery());
    TSUNIT_EQUAL(u"fr", url.getFragment());

    TSUNIT_ASSERT(a.resolve(url.getHost(), CERR));
    TSUNIT_EQUAL(u"fe80::93a3:dea0:2108:b81e", a.toString());
    TSUNIT_EQUAL(u"fe80:0000:0000:0000:93a3:dea0:2108:b81e", a.toFullString());
}
