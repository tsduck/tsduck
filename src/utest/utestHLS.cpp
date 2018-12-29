//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  CppUnit test suite for HLS classes.
//
//----------------------------------------------------------------------------

#include "tshlsPlayList.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class HLSTest: public CppUnit::TestFixture
{
public:
    HLSTest();

    virtual void setUp() override;
    virtual void tearDown() override;

    void testMasterPlaylist();
    void testMediaPlaylist();

    CPPUNIT_TEST_SUITE(HLSTest);
    CPPUNIT_TEST(testMasterPlaylist);
    CPPUNIT_TEST(testMediaPlaylist);
    CPPUNIT_TEST_SUITE_END();

private:
    int _previousSeverity;
};

CPPUNIT_TEST_SUITE_REGISTRATION(HLSTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
HLSTest::HLSTest() :
    _previousSeverity(0)
{
}

// Test suite initialization method.
void HLSTest::setUp()
{
    _previousSeverity = CERR.maxSeverity();
    if (utest::DebugMode()) {
        CERR.setMaxSeverity(ts::Severity::Debug);
    }
}

// Test suite cleanup method.
void HLSTest::tearDown()
{
    CERR.setMaxSeverity(_previousSeverity);
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void HLSTest::testMasterPlaylist()
{
    // Test file downloaded from TSDuck web site.
    // Copied from Apple test file at
    // https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_ts/master.m3u8

    ts::hls::PlayList pl;
    CPPUNIT_ASSERT(pl.loadURL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/master.m3u8", true));
    CPPUNIT_ASSERT(pl.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::hls::MASTER_PLAYLIST, pl.type());
    CPPUNIT_ASSERT_EQUAL(6, pl.version());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/master.m3u8", pl.url());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/foo.bar", pl.buildURL(u"foo.bar"));
    CPPUNIT_ASSERT_EQUAL(size_t(0), pl.segmentCount());
    CPPUNIT_ASSERT_EQUAL(size_t(24), pl.playListCount());
    CPPUNIT_ASSERT_EQUAL(ts::Second(0), pl.targetDuration());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pl.mediaSequence());
    CPPUNIT_ASSERT(!pl.endList());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", pl.playlistType());

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"v5/prog_index.m3u8", pl.playList(0).uri);
    CPPUNIT_ASSERT_EQUAL(ts::BitRate(2227464), pl.playList(0).bandwidth);
    CPPUNIT_ASSERT_EQUAL(ts::BitRate(2218327), pl.playList(0).averageBandwidth);
    CPPUNIT_ASSERT_EQUAL(size_t(960), pl.playList(0).width);
    CPPUNIT_ASSERT_EQUAL(size_t(540), pl.playList(0).height);
    CPPUNIT_ASSERT_EQUAL(size_t(60000), pl.playList(0).frameRate);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"avc1.640020,mp4a.40.2", pl.playList(0).codecs);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", pl.playList(0).hdcp);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", pl.playList(0).videoRange);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", pl.playList(0).video);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"aud1", pl.playList(0).audio);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"sub1", pl.playList(0).subtitles);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"cc1", pl.playList(0).closedCaptions);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"v5/prog_index.m3u8, 960x540, 2,227,464 b/s, @60 fps", pl.playList(0).toString());

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"v2/prog_index.m3u8", pl.playList(23).uri);
    CPPUNIT_ASSERT_EQUAL(ts::BitRate(582387), pl.playList(23).bandwidth);
    CPPUNIT_ASSERT_EQUAL(ts::BitRate(570616), pl.playList(23).averageBandwidth);
    CPPUNIT_ASSERT_EQUAL(size_t(480), pl.playList(23).width);
    CPPUNIT_ASSERT_EQUAL(size_t(270), pl.playList(23).height);
    CPPUNIT_ASSERT_EQUAL(size_t(30000), pl.playList(23).frameRate);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"avc1.640015,ec-3", pl.playList(23).codecs);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", pl.playList(23).hdcp);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", pl.playList(23).videoRange);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", pl.playList(23).video);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"aud3", pl.playList(23).audio);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"sub1", pl.playList(23).subtitles);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"cc1", pl.playList(23).closedCaptions);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"v2/prog_index.m3u8, 480x270, 582,387 b/s, @30 fps", pl.playList(23).toString());

    CPPUNIT_ASSERT_EQUAL(size_t(0), pl.selectPlayList(0, 0, 0, 0, 0, 0));
    CPPUNIT_ASSERT_EQUAL(ts::NPOS, pl.selectPlayList(10000000, 0, 0, 0, 0, 0));
    CPPUNIT_ASSERT_EQUAL(size_t(9), pl.selectPlayListHighestBitRate());
    CPPUNIT_ASSERT_EQUAL(size_t(7), pl.selectPlayListLowestBitRate());
    CPPUNIT_ASSERT_EQUAL(size_t(1), pl.selectPlayListHighestResolution());
    CPPUNIT_ASSERT_EQUAL(size_t(7), pl.selectPlayListLowestResolution());
}

void HLSTest::testMediaPlaylist()
{
    // Test file downloaded from TSDuck web site.
    // Copied from Apple test file at
    // https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_ts/v5/prog_index.m3u8

    ts::hls::PlayList pl;
    CPPUNIT_ASSERT(pl.loadURL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/v5/prog_index.m3u8", true));
    CPPUNIT_ASSERT(pl.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::hls::MEDIA_PLAYLIST, pl.type());
    CPPUNIT_ASSERT_EQUAL(3, pl.version());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/v5/prog_index.m3u8", pl.url());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/v5/foo.bar", pl.buildURL(u"foo.bar"));
    CPPUNIT_ASSERT_EQUAL(size_t(100), pl.segmentCount());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pl.playListCount());
    CPPUNIT_ASSERT_EQUAL(ts::Second(6), pl.targetDuration());
    CPPUNIT_ASSERT_EQUAL(size_t(0), pl.mediaSequence());
    CPPUNIT_ASSERT(pl.endList());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"VOD", pl.playlistType());

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fileSequence0.ts", pl.segment(0).uri);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", pl.segment(0).title);
    CPPUNIT_ASSERT_EQUAL(ts::BitRate(2060 * 1024), pl.segment(0).bitrate);
    CPPUNIT_ASSERT_EQUAL(ts::MilliSecond(6000), pl.segment(0).duration);
    CPPUNIT_ASSERT(!pl.segment(0).gap);

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fileSequence99.ts", pl.segment(99).uri);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", pl.segment(99).title);
    CPPUNIT_ASSERT_EQUAL(ts::BitRate(2055 * 1024), pl.segment(99).bitrate);
    CPPUNIT_ASSERT_EQUAL(ts::MilliSecond(6000), pl.segment(99).duration);
    CPPUNIT_ASSERT(!pl.segment(99).gap);

    ts::hls::MediaSegment seg;
    CPPUNIT_ASSERT(pl.popFirstSegment(seg));
    CPPUNIT_ASSERT_EQUAL(size_t(99), pl.segmentCount());

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"fileSequence0.ts", seg.uri);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", seg.title);
    CPPUNIT_ASSERT_EQUAL(ts::BitRate(2060 * 1024), seg.bitrate);
    CPPUNIT_ASSERT_EQUAL(ts::MilliSecond(6000), seg.duration);
    CPPUNIT_ASSERT(!seg.gap);
}
