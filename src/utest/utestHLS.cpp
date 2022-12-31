//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  TSUnit test suite for HLS classes.
//
//----------------------------------------------------------------------------

#include "tshlsPlayList.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class HLSTest: public tsunit::Test
{
public:
    HLSTest();

    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testMasterPlaylist();
    void testMasterPlaylistWithAlternate();
    void testMediaPlaylist();
    void testBuildMasterPlaylist();
    void testBuildMediaPlaylist();

    TSUNIT_TEST_BEGIN(HLSTest);
    TSUNIT_TEST(testMasterPlaylist);
    TSUNIT_TEST(testMasterPlaylistWithAlternate);
    TSUNIT_TEST(testMediaPlaylist);
    TSUNIT_TEST(testBuildMasterPlaylist);
    TSUNIT_TEST(testBuildMediaPlaylist);
    TSUNIT_TEST_END();

private:
    int _previousSeverity;
};

TSUNIT_REGISTER(HLSTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
HLSTest::HLSTest() :
    _previousSeverity(0)
{
}

// Test suite initialization method.
void HLSTest::beforeTest()
{
    _previousSeverity = CERR.maxSeverity();
    if (tsunit::Test::debugMode()) {
        CERR.setMaxSeverity(ts::Severity::Debug);
    }
}

// Test suite cleanup method.
void HLSTest::afterTest()
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
    TSUNIT_ASSERT(pl.loadURL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/master.m3u8", true));
    TSUNIT_ASSERT(pl.isValid());
    TSUNIT_EQUAL(ts::hls::PlayListType::MASTER, pl.type());
    TSUNIT_EQUAL(6, pl.version());
    TSUNIT_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/master.m3u8", pl.url());
    ts::hls::MediaElement media;
    pl.buildURL(media, u"foo.bar");
    TSUNIT_EQUAL(u"foo.bar", media.relativeURI);
    TSUNIT_EQUAL(u"/download/test/hls/img_bipbop_adv_example_ts/foo.bar", media.filePath);
    TSUNIT_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/foo.bar", media.url.toString());
    TSUNIT_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/foo.bar", media.urlString());
    TSUNIT_EQUAL(0, pl.segmentCount());
    TSUNIT_EQUAL(24, pl.playListCount());
    TSUNIT_EQUAL(5, pl.altPlayListCount());
    TSUNIT_EQUAL(0, pl.targetDuration());
    TSUNIT_EQUAL(0, pl.mediaSequence());
    TSUNIT_ASSERT(!pl.endList());
    TSUNIT_EQUAL(ts::hls::PlayListType::MASTER, pl.type());

    TSUNIT_EQUAL(u"v5/prog_index.m3u8", pl.playList(0).relativeURI);
    TSUNIT_EQUAL(2227464, pl.playList(0).bandwidth.toInt());
    TSUNIT_EQUAL(2218327, pl.playList(0).averageBandwidth.toInt());
    TSUNIT_EQUAL(960, pl.playList(0).width);
    TSUNIT_EQUAL(540, pl.playList(0).height);
    TSUNIT_EQUAL(60000, pl.playList(0).frameRate);
    TSUNIT_EQUAL(u"avc1.640020,mp4a.40.2", pl.playList(0).codecs);
    TSUNIT_EQUAL(u"", pl.playList(0).hdcp);
    TSUNIT_EQUAL(u"", pl.playList(0).videoRange);
    TSUNIT_EQUAL(u"", pl.playList(0).video);
    TSUNIT_EQUAL(u"aud1", pl.playList(0).audio);
    TSUNIT_EQUAL(u"sub1", pl.playList(0).subtitles);
    TSUNIT_EQUAL(u"cc1", pl.playList(0).closedCaptions);
    TSUNIT_EQUAL(u"v5/prog_index.m3u8, 960x540, 2,227,464 b/s, @60 fps", pl.playList(0).toString());

    TSUNIT_EQUAL(u"v2/prog_index.m3u8", pl.playList(23).relativeURI);
    TSUNIT_EQUAL(582387, pl.playList(23).bandwidth.toInt());
    TSUNIT_EQUAL(570616, pl.playList(23).averageBandwidth.toInt());
    TSUNIT_EQUAL(480, pl.playList(23).width);
    TSUNIT_EQUAL(270, pl.playList(23).height);
    TSUNIT_EQUAL(30000, pl.playList(23).frameRate);
    TSUNIT_EQUAL(u"avc1.640015,ec-3", pl.playList(23).codecs);
    TSUNIT_EQUAL(u"", pl.playList(23).hdcp);
    TSUNIT_EQUAL(u"", pl.playList(23).videoRange);
    TSUNIT_EQUAL(u"", pl.playList(23).video);
    TSUNIT_EQUAL(u"aud3", pl.playList(23).audio);
    TSUNIT_EQUAL(u"sub1", pl.playList(23).subtitles);
    TSUNIT_EQUAL(u"cc1", pl.playList(23).closedCaptions);
    TSUNIT_EQUAL(u"v2/prog_index.m3u8, 480x270, 582,387 b/s, @30 fps", pl.playList(23).toString());

    TSUNIT_EQUAL(0, pl.selectPlayList(0, 0, 0, 0, 0, 0));
    TSUNIT_EQUAL(ts::NPOS, pl.selectPlayList(10000000, 0, 0, 0, 0, 0));
    TSUNIT_EQUAL(9, pl.selectPlayListHighestBitRate());
    TSUNIT_EQUAL(7, pl.selectPlayListLowestBitRate());
    TSUNIT_EQUAL(1, pl.selectPlayListHighestResolution());
    TSUNIT_EQUAL(7, pl.selectPlayListLowestResolution());
}

void HLSTest::testMasterPlaylistWithAlternate()
{
    // Test file downloaded from TSDuck web site.

    ts::hls::PlayList pl;
    TSUNIT_ASSERT(pl.loadURL(u"https://tsduck.io/download/test/hls/alternative/index_hd.m3u8", true));
    TSUNIT_ASSERT(pl.isValid());
    TSUNIT_EQUAL(ts::hls::PlayListType::MASTER, pl.type());
    TSUNIT_EQUAL(4, pl.version());
    TSUNIT_EQUAL(u"https://tsduck.io/download/test/hls/alternative/index_hd.m3u8", pl.url());
    TSUNIT_EQUAL(0, pl.segmentCount());
    TSUNIT_EQUAL(7, pl.playListCount());
    TSUNIT_EQUAL(2, pl.altPlayListCount());
    TSUNIT_EQUAL(0, pl.targetDuration());
    TSUNIT_EQUAL(0, pl.mediaSequence());
    TSUNIT_ASSERT(!pl.endList());
    TSUNIT_EQUAL(ts::hls::PlayListType::MASTER, pl.type());

    TSUNIT_EQUAL(u"04_hd.m3u8", pl.playList(0).relativeURI);
    TSUNIT_EQUAL(1209781, pl.playList(0).bandwidth.toInt());
    TSUNIT_EQUAL(768, pl.playList(0).width);
    TSUNIT_EQUAL(432, pl.playList(0).height);
    TSUNIT_EQUAL(25000, pl.playList(0).frameRate);
    TSUNIT_EQUAL(u"avc1.4D4020,mp4a.40.2", pl.playList(0).codecs);
    TSUNIT_EQUAL(u"", pl.playList(0).hdcp);
    TSUNIT_EQUAL(u"", pl.playList(0).videoRange);
    TSUNIT_EQUAL(u"", pl.playList(0).video);
    TSUNIT_EQUAL(u"audio2", pl.playList(0).audio);
    TSUNIT_EQUAL(u"", pl.playList(0).subtitles);
    TSUNIT_EQUAL(u"", pl.playList(0).closedCaptions);
    TSUNIT_EQUAL(u"04_hd.m3u8, 768x432, 1,209,781 b/s, @25 fps", pl.playList(0).toString());

    TSUNIT_EQUAL(u"09_hd.m3u8", pl.altPlayList(0).relativeURI);
    TSUNIT_EQUAL(u"AUDIO", pl.altPlayList(0).type);
    TSUNIT_EQUAL(u"audio2", pl.altPlayList(0).groupId);
    TSUNIT_EQUAL(u"ENG", pl.altPlayList(0).name);
    TSUNIT_EQUAL(u"ENG", pl.altPlayList(0).language);
    TSUNIT_EQUAL(u"", pl.altPlayList(0).stableRenditionId);
    TSUNIT_EQUAL(u"", pl.altPlayList(0).assocLanguage);
    TSUNIT_EQUAL(u"", pl.altPlayList(0).inStreamId);
    TSUNIT_EQUAL(u"", pl.altPlayList(0).characteristics);
    TSUNIT_EQUAL(u"", pl.altPlayList(0).channels);
    TSUNIT_ASSERT(pl.altPlayList(0).isDefault);
    TSUNIT_ASSERT(pl.altPlayList(0).autoselect);
    TSUNIT_ASSERT(!pl.altPlayList(0).forced);

    TSUNIT_EQUAL(u"01_hd.m3u8", pl.altPlayList(1).relativeURI);
    TSUNIT_EQUAL(u"AUDIO", pl.altPlayList(1).type);
    TSUNIT_EQUAL(u"audio1", pl.altPlayList(1).groupId);
    TSUNIT_EQUAL(u"FOO", pl.altPlayList(1).name);
    TSUNIT_EQUAL(u"FOO", pl.altPlayList(1).language);
    TSUNIT_EQUAL(u"", pl.altPlayList(1).stableRenditionId);
    TSUNIT_EQUAL(u"", pl.altPlayList(1).assocLanguage);
    TSUNIT_EQUAL(u"", pl.altPlayList(1).inStreamId);
    TSUNIT_EQUAL(u"", pl.altPlayList(1).characteristics);
    TSUNIT_EQUAL(u"", pl.altPlayList(1).channels);
    TSUNIT_ASSERT(!pl.altPlayList(1).isDefault);
    TSUNIT_ASSERT(!pl.altPlayList(1).autoselect);
    TSUNIT_ASSERT(!pl.altPlayList(1).forced);
}

void HLSTest::testMediaPlaylist()
{
    // Test file downloaded from TSDuck web site.
    // Copied from Apple test file at
    // https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_ts/v5/prog_index.m3u8

    ts::hls::PlayList pl;
    TSUNIT_ASSERT(pl.loadURL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/v5/prog_index.m3u8", true));
    TSUNIT_ASSERT(pl.isValid());
    TSUNIT_EQUAL(ts::hls::PlayListType::VOD, pl.type());
    TSUNIT_EQUAL(3, pl.version());
    TSUNIT_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/v5/prog_index.m3u8", pl.url());
    ts::hls::MediaElement media;
    pl.buildURL(media, u"foo.bar");
    TSUNIT_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/v5/foo.bar", media.urlString());
    TSUNIT_EQUAL(100, pl.segmentCount());
    TSUNIT_EQUAL(0, pl.playListCount());
    TSUNIT_EQUAL(0, pl.altPlayListCount());
    TSUNIT_EQUAL(6, pl.targetDuration());
    TSUNIT_EQUAL(0, pl.mediaSequence());
    TSUNIT_ASSERT(pl.endList());

    TSUNIT_EQUAL(u"fileSequence0.ts", pl.segment(0).relativeURI);
    TSUNIT_EQUAL(u"", pl.segment(0).title);
    TSUNIT_EQUAL(2060 * 1024, pl.segment(0).bitrate.toInt());
    TSUNIT_EQUAL(6000, pl.segment(0).duration);
    TSUNIT_ASSERT(!pl.segment(0).gap);

    TSUNIT_EQUAL(u"fileSequence99.ts", pl.segment(99).relativeURI);
    TSUNIT_EQUAL(u"", pl.segment(99).title);
    TSUNIT_EQUAL(2055 * 1024, pl.segment(99).bitrate.toInt());
    TSUNIT_EQUAL(6000, pl.segment(99).duration);
    TSUNIT_ASSERT(!pl.segment(99).gap);

    ts::hls::MediaSegment seg;
    TSUNIT_ASSERT(pl.popFirstSegment(seg));
    TSUNIT_EQUAL(99, pl.segmentCount());

    TSUNIT_EQUAL(u"fileSequence0.ts", seg.relativeURI);
    TSUNIT_EQUAL(u"", seg.title);
    TSUNIT_EQUAL(2060 * 1024, seg.bitrate.toInt());
    TSUNIT_EQUAL(6000, seg.duration);
    TSUNIT_ASSERT(!seg.gap);
}

void HLSTest::testBuildMasterPlaylist()
{
    ts::hls::PlayList pl;
    pl.reset(ts::hls::PlayListType::MASTER, u"/c/test/path/master/test.m3u8");

    TSUNIT_ASSERT(pl.isValid());
    TSUNIT_EQUAL(ts::hls::PlayListType::MASTER, pl.type());
    TSUNIT_EQUAL(3, pl.version());

    ts::hls::MediaPlayList mpl1;
    mpl1.relativeURI = u"/c/test/path/playlists/pl1.m3u8";
    mpl1.bandwidth = 1234567;
    mpl1.averageBandwidth = 1200000;
    mpl1.width = 720;
    mpl1.height = 576;
    mpl1.frameRate = 30123;
    mpl1.codecs = u"cot,cot";
    mpl1.hdcp = u"NONE";
    mpl1.videoRange = u"SDR";
    mpl1.video = u"vid1";
    mpl1.audio = u"aud3";
    mpl1.subtitles = u"sub1";
    mpl1.closedCaptions = u"cc1";

    TSUNIT_ASSERT(pl.addPlayList(mpl1));

    ts::hls::MediaPlayList mpl2;
    mpl2.relativeURI = u"/c/test/path/playlists/pl2.m3u8";
    mpl2.bandwidth = 3456789;
    mpl2.averageBandwidth = 3400000;
    mpl2.width = 1920;
    mpl2.height = 1080;
    mpl2.frameRate = 60567;

    TSUNIT_ASSERT(pl.addPlayList(mpl2));

    TSUNIT_EQUAL(0, pl.segmentCount());
    TSUNIT_EQUAL(2, pl.playListCount());

    static const ts::UChar* const refContent =
        u"#EXTM3U\n"
        u"#EXT-X-VERSION:3\n"
        u"#EXT-X-STREAM-INF:BANDWIDTH=1234567,AVERAGE-BANDWIDTH=1200000,FRAME-RATE=30.123,RESOLUTION=720x576,"
        u"CODECS=\"cot,cot\",HDCP-LEVEL=NONE,VIDEO-RANGE=SDR,VIDEO=\"vid1\",AUDIO=\"aud3\",SUBTITLES=\"sub1\",CLOSED-CAPTIONS=\"cc1\"\n"
        u"../playlists/pl1.m3u8\n"
        u"#EXT-X-STREAM-INF:BANDWIDTH=3456789,AVERAGE-BANDWIDTH=3400000,FRAME-RATE=60.567,RESOLUTION=1920x1080\n"
        u"../playlists/pl2.m3u8\n";

    TSUNIT_EQUAL(refContent, pl.textContent());
}

void HLSTest::testBuildMediaPlaylist()
{
    ts::hls::PlayList pl;
    pl.reset(ts::hls::PlayListType::LIVE, u"/c/test/path/master/test.m3u8");

    TSUNIT_ASSERT(pl.isValid());
    TSUNIT_EQUAL(ts::hls::PlayListType::LIVE, pl.type());
    TSUNIT_EQUAL(3, pl.version());
    TSUNIT_EQUAL(0, pl.segmentCount());
    TSUNIT_EQUAL(0, pl.playListCount());

    TSUNIT_ASSERT(pl.setMediaSequence(7));
    TSUNIT_ASSERT(pl.setTargetDuration(5));
    TSUNIT_ASSERT(!pl.endList());
    TSUNIT_ASSERT(pl.setEndList(true));
    TSUNIT_ASSERT(pl.endList());
    TSUNIT_ASSERT(pl.setType(ts::hls::PlayListType::VOD));
    TSUNIT_EQUAL(ts::hls::PlayListType::VOD, pl.type());

    ts::hls::MediaSegment seg1;
    seg1.relativeURI = u"/c/test/path/segments/seg-0001.ts";
    seg1.title = u"Segment1";
    seg1.duration = 4920;
    seg1.bitrate = 1233920;
    TSUNIT_ASSERT(pl.addSegment(seg1));

    ts::hls::MediaSegment seg2;
    seg2.relativeURI = u"/c/test/path/segments/seg-0002.ts";
    seg2.duration = 4971;
    seg2.bitrate = 1653760;
    TSUNIT_ASSERT(pl.addSegment(seg2));

    TSUNIT_EQUAL(2, pl.segmentCount());
    TSUNIT_EQUAL(0, pl.playListCount());

    static const ts::UChar* const refContent1 =
        u"#EXTM3U\n"
        u"#EXT-X-VERSION:3\n"
        u"#EXT-X-TARGETDURATION:5\n"
        u"#EXT-X-MEDIA-SEQUENCE:7\n"
        u"#EXT-X-PLAYLIST-TYPE:VOD\n"
        u"#EXTINF:4.920,Segment1\n"
        u"#EXT-X-BITRATE:1205\n"
        u"../segments/seg-0001.ts\n"
        u"#EXTINF:4.971,\n"
        u"#EXT-X-BITRATE:1615\n"
        u"../segments/seg-0002.ts\n"
        u"#EXT-X-ENDLIST\n";

    TSUNIT_EQUAL(refContent1, pl.textContent());

    ts::hls::MediaSegment seg3;
    seg3.relativeURI = u"/c/test/path/segments/seg-0003.ts";
    seg3.duration = 4984;
    seg3.bitrate = 1653760;
    TSUNIT_ASSERT(pl.addSegment(seg3));

    TSUNIT_EQUAL(3, pl.segmentCount());
    TSUNIT_EQUAL(0, pl.playListCount());

    TSUNIT_ASSERT(pl.popFirstSegment(seg3));
    TSUNIT_EQUAL(2, pl.segmentCount());

    static const ts::UChar* const refContent2 =
        u"#EXTM3U\n"
        u"#EXT-X-VERSION:3\n"
        u"#EXT-X-TARGETDURATION:5\n"
        u"#EXT-X-MEDIA-SEQUENCE:8\n"
        u"#EXT-X-PLAYLIST-TYPE:VOD\n"
        u"#EXTINF:4.971,\n"
        u"#EXT-X-BITRATE:1615\n"
        u"../segments/seg-0002.ts\n"
        u"#EXTINF:4.984,\n"
        u"#EXT-X-BITRATE:1615\n"
        u"../segments/seg-0003.ts\n"
        u"#EXT-X-ENDLIST\n";

    TSUNIT_EQUAL(refContent2, pl.textContent());
}
