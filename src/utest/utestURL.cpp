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
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class URLTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(IsURL);
    TSUNIT_DECLARE_TEST(Parse);
    TSUNIT_DECLARE_TEST(Base);
    TSUNIT_DECLARE_TEST(ToString);
    TSUNIT_DECLARE_TEST(ToRelative);
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
