//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(MasterPlaylist);
    TSUNIT_DECLARE_TEST(MasterPlaylistWithAlternate);
    TSUNIT_DECLARE_TEST(MediaPlaylist);
    TSUNIT_DECLARE_TEST(BuildMasterPlaylist);
    TSUNIT_DECLARE_TEST(BuildMediaPlaylist);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    int _previousSeverity = 0;
};

TSUNIT_REGISTER(HLSTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

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

TSUNIT_DEFINE_TEST(MasterPlaylist)
{
    // Test file downloaded from TSDuck web site.
    // Copied from Apple test file at
    // https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_ts/master.m3u8

    ts::hls::PlayList pl;
    TSUNIT_ASSERT(pl.loadURL(u"https://tsduck.io/teststreams/hls/img_bipbop_adv_example_ts/master.m3u8", true));
    TSUNIT_ASSERT(pl.isValid());
    TSUNIT_EQUAL(ts::hls::PlayListType::MASTER, pl.type());
    TSUNIT_EQUAL(6, pl.version());
    TSUNIT_EQUAL(u"https://tsduck.io/teststreams/hls/img_bipbop_adv_example_ts/master.m3u8", pl.url());
    ts::hls::MediaElement media;
    pl.buildURL(media, u"foo.bar");
    TSUNIT_EQUAL(u"foo.bar", media.relative_uri);
    TSUNIT_EQUAL(u"/teststreams/hls/img_bipbop_adv_example_ts/foo.bar", media.file_path);
    TSUNIT_EQUAL(u"https://tsduck.io/teststreams/hls/img_bipbop_adv_example_ts/foo.bar", media.url.toString());
    TSUNIT_EQUAL(u"https://tsduck.io/teststreams/hls/img_bipbop_adv_example_ts/foo.bar", media.urlString());
    TSUNIT_EQUAL(0, pl.segmentCount());
    TSUNIT_EQUAL(24, pl.playListCount());
    TSUNIT_EQUAL(5, pl.altPlayListCount());
    TSUNIT_EQUAL(0, pl.targetDuration().count());
    TSUNIT_EQUAL(0, pl.mediaSequence());
    TSUNIT_ASSERT(!pl.endList());
    TSUNIT_EQUAL(ts::hls::PlayListType::MASTER, pl.type());

    TSUNIT_EQUAL(u"v5/prog_index.m3u8", pl.playList(0).relative_uri);
    TSUNIT_EQUAL(2227464, pl.playList(0).bandwidth.toInt());
    TSUNIT_EQUAL(2218327, pl.playList(0).average_bandwidth.toInt());
    TSUNIT_EQUAL(960, pl.playList(0).width);
    TSUNIT_EQUAL(540, pl.playList(0).height);
    TSUNIT_EQUAL(60000, pl.playList(0).frame_rate);
    TSUNIT_EQUAL(u"avc1.640020,mp4a.40.2", pl.playList(0).codecs);
    TSUNIT_EQUAL(u"", pl.playList(0).hdcp);
    TSUNIT_EQUAL(u"", pl.playList(0).video_range);
    TSUNIT_EQUAL(u"", pl.playList(0).video);
    TSUNIT_EQUAL(u"aud1", pl.playList(0).audio);
    TSUNIT_EQUAL(u"sub1", pl.playList(0).subtitles);
    TSUNIT_EQUAL(u"cc1", pl.playList(0).closed_captions);
    TSUNIT_EQUAL(u"v5/prog_index.m3u8, 960x540, 2,227,464 b/s, @60 fps", pl.playList(0).toString());

    TSUNIT_EQUAL(u"v2/prog_index.m3u8", pl.playList(23).relative_uri);
    TSUNIT_EQUAL(582387, pl.playList(23).bandwidth.toInt());
    TSUNIT_EQUAL(570616, pl.playList(23).average_bandwidth.toInt());
    TSUNIT_EQUAL(480, pl.playList(23).width);
    TSUNIT_EQUAL(270, pl.playList(23).height);
    TSUNIT_EQUAL(30000, pl.playList(23).frame_rate);
    TSUNIT_EQUAL(u"avc1.640015,ec-3", pl.playList(23).codecs);
    TSUNIT_EQUAL(u"", pl.playList(23).hdcp);
    TSUNIT_EQUAL(u"", pl.playList(23).video_range);
    TSUNIT_EQUAL(u"", pl.playList(23).video);
    TSUNIT_EQUAL(u"aud3", pl.playList(23).audio);
    TSUNIT_EQUAL(u"sub1", pl.playList(23).subtitles);
    TSUNIT_EQUAL(u"cc1", pl.playList(23).closed_captions);
    TSUNIT_EQUAL(u"v2/prog_index.m3u8, 480x270, 582,387 b/s, @30 fps", pl.playList(23).toString());

    TSUNIT_EQUAL(0, pl.selectPlayList(0, 0, 0, 0, 0, 0));
    TSUNIT_EQUAL(ts::NPOS, pl.selectPlayList(10000000, 0, 0, 0, 0, 0));
    TSUNIT_EQUAL(9, pl.selectPlayListHighestBitRate());
    TSUNIT_EQUAL(7, pl.selectPlayListLowestBitRate());
    TSUNIT_EQUAL(1, pl.selectPlayListHighestResolution());
    TSUNIT_EQUAL(7, pl.selectPlayListLowestResolution());
}

TSUNIT_DEFINE_TEST(MasterPlaylistWithAlternate)
{
    // Test file downloaded from TSDuck web site.

    ts::hls::PlayList pl;
    TSUNIT_ASSERT(pl.loadURL(u"https://tsduck.io/teststreams/hls/alternative/index_hd.m3u8", true));
    TSUNIT_ASSERT(pl.isValid());
    TSUNIT_EQUAL(ts::hls::PlayListType::MASTER, pl.type());
    TSUNIT_EQUAL(4, pl.version());
    TSUNIT_EQUAL(u"https://tsduck.io/teststreams/hls/alternative/index_hd.m3u8", pl.url());
    TSUNIT_EQUAL(0, pl.segmentCount());
    TSUNIT_EQUAL(7, pl.playListCount());
    TSUNIT_EQUAL(2, pl.altPlayListCount());
    TSUNIT_EQUAL(0, pl.targetDuration().count());
    TSUNIT_EQUAL(0, pl.mediaSequence());
    TSUNIT_ASSERT(!pl.endList());
    TSUNIT_EQUAL(ts::hls::PlayListType::MASTER, pl.type());

    TSUNIT_EQUAL(u"04_hd.m3u8", pl.playList(0).relative_uri);
    TSUNIT_EQUAL(1209781, pl.playList(0).bandwidth.toInt());
    TSUNIT_EQUAL(768, pl.playList(0).width);
    TSUNIT_EQUAL(432, pl.playList(0).height);
    TSUNIT_EQUAL(25000, pl.playList(0).frame_rate);
    TSUNIT_EQUAL(u"avc1.4D4020,mp4a.40.2", pl.playList(0).codecs);
    TSUNIT_EQUAL(u"", pl.playList(0).hdcp);
    TSUNIT_EQUAL(u"", pl.playList(0).video_range);
    TSUNIT_EQUAL(u"", pl.playList(0).video);
    TSUNIT_EQUAL(u"audio2", pl.playList(0).audio);
    TSUNIT_EQUAL(u"", pl.playList(0).subtitles);
    TSUNIT_EQUAL(u"", pl.playList(0).closed_captions);
    TSUNIT_EQUAL(u"04_hd.m3u8, 768x432, 1,209,781 b/s, @25 fps", pl.playList(0).toString());

    TSUNIT_EQUAL(u"09_hd.m3u8", pl.altPlayList(0).relative_uri);
    TSUNIT_EQUAL(u"AUDIO", pl.altPlayList(0).type);
    TSUNIT_EQUAL(u"audio2", pl.altPlayList(0).group_id);
    TSUNIT_EQUAL(u"ENG", pl.altPlayList(0).name);
    TSUNIT_EQUAL(u"ENG", pl.altPlayList(0).language);
    TSUNIT_EQUAL(u"", pl.altPlayList(0).stable_rendition_id);
    TSUNIT_EQUAL(u"", pl.altPlayList(0).assoc_language);
    TSUNIT_EQUAL(u"", pl.altPlayList(0).in_stream_id);
    TSUNIT_EQUAL(u"", pl.altPlayList(0).characteristics);
    TSUNIT_EQUAL(u"", pl.altPlayList(0).channels);
    TSUNIT_ASSERT(pl.altPlayList(0).is_default);
    TSUNIT_ASSERT(pl.altPlayList(0).auto_select);
    TSUNIT_ASSERT(!pl.altPlayList(0).forced);

    TSUNIT_EQUAL(u"01_hd.m3u8", pl.altPlayList(1).relative_uri);
    TSUNIT_EQUAL(u"AUDIO", pl.altPlayList(1).type);
    TSUNIT_EQUAL(u"audio1", pl.altPlayList(1).group_id);
    TSUNIT_EQUAL(u"FOO", pl.altPlayList(1).name);
    TSUNIT_EQUAL(u"FOO", pl.altPlayList(1).language);
    TSUNIT_EQUAL(u"", pl.altPlayList(1).stable_rendition_id);
    TSUNIT_EQUAL(u"", pl.altPlayList(1).assoc_language);
    TSUNIT_EQUAL(u"", pl.altPlayList(1).in_stream_id);
    TSUNIT_EQUAL(u"", pl.altPlayList(1).characteristics);
    TSUNIT_EQUAL(u"", pl.altPlayList(1).channels);
    TSUNIT_ASSERT(!pl.altPlayList(1).is_default);
    TSUNIT_ASSERT(!pl.altPlayList(1).auto_select);
    TSUNIT_ASSERT(!pl.altPlayList(1).forced);
}

TSUNIT_DEFINE_TEST(MediaPlaylist)
{
    // Test file downloaded from TSDuck web site.
    // Copied from Apple test file at
    // https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_ts/v5/prog_index.m3u8

    ts::hls::PlayList pl;
    TSUNIT_ASSERT(pl.loadURL(u"https://tsduck.io/teststreams/hls/img_bipbop_adv_example_ts/v5/prog_index.m3u8", true));
    TSUNIT_ASSERT(pl.isValid());
    TSUNIT_EQUAL(ts::hls::PlayListType::VOD, pl.type());
    TSUNIT_EQUAL(3, pl.version());
    TSUNIT_EQUAL(u"https://tsduck.io/teststreams/hls/img_bipbop_adv_example_ts/v5/prog_index.m3u8", pl.url());
    ts::hls::MediaElement media;
    pl.buildURL(media, u"foo.bar");
    TSUNIT_EQUAL(u"https://tsduck.io/teststreams/hls/img_bipbop_adv_example_ts/v5/foo.bar", media.urlString());
    TSUNIT_EQUAL(100, pl.segmentCount());
    TSUNIT_EQUAL(0, pl.playListCount());
    TSUNIT_EQUAL(0, pl.altPlayListCount());
    TSUNIT_EQUAL(6, pl.targetDuration().count());
    TSUNIT_EQUAL(0, pl.mediaSequence());
    TSUNIT_ASSERT(pl.endList());

    TSUNIT_EQUAL(u"fileSequence0.ts", pl.segment(0).relative_uri);
    TSUNIT_EQUAL(u"", pl.segment(0).title);
    TSUNIT_EQUAL(2060 * 1024, pl.segment(0).bitrate.toInt());
    TSUNIT_EQUAL(6000, pl.segment(0).duration.count());
    TSUNIT_ASSERT(!pl.segment(0).gap);

    TSUNIT_EQUAL(u"fileSequence99.ts", pl.segment(99).relative_uri);
    TSUNIT_EQUAL(u"", pl.segment(99).title);
    TSUNIT_EQUAL(2055 * 1024, pl.segment(99).bitrate.toInt());
    TSUNIT_EQUAL(6000, pl.segment(99).duration.count());
    TSUNIT_ASSERT(!pl.segment(99).gap);

    ts::hls::MediaSegment seg;
    TSUNIT_ASSERT(pl.popFirstSegment(seg));
    TSUNIT_EQUAL(99, pl.segmentCount());

    TSUNIT_EQUAL(u"fileSequence0.ts", seg.relative_uri);
    TSUNIT_EQUAL(u"", seg.title);
    TSUNIT_EQUAL(2060 * 1024, seg.bitrate.toInt());
    TSUNIT_EQUAL(6000, seg.duration.count());
    TSUNIT_ASSERT(!seg.gap);
}

TSUNIT_DEFINE_TEST(BuildMasterPlaylist)
{
    ts::hls::PlayList pl;
    pl.reset(ts::hls::PlayListType::MASTER, u"/c/test/path/master/test.m3u8");

    TSUNIT_ASSERT(pl.isValid());
    TSUNIT_EQUAL(ts::hls::PlayListType::MASTER, pl.type());
    TSUNIT_EQUAL(3, pl.version());

    ts::hls::MediaPlayList mpl1;
    mpl1.relative_uri = u"/c/test/path/playlists/pl1.m3u8";
    mpl1.bandwidth = 1234567;
    mpl1.average_bandwidth = 1200000;
    mpl1.width = 720;
    mpl1.height = 576;
    mpl1.frame_rate = 30123;
    mpl1.codecs = u"cot,cot";
    mpl1.hdcp = u"NONE";
    mpl1.video_range = u"SDR";
    mpl1.video = u"vid1";
    mpl1.audio = u"aud3";
    mpl1.subtitles = u"sub1";
    mpl1.closed_captions = u"cc1";

    TSUNIT_ASSERT(pl.addPlayList(mpl1));

    ts::hls::MediaPlayList mpl2;
    mpl2.relative_uri = u"/c/test/path/playlists/pl2.m3u8";
    mpl2.bandwidth = 3456789;
    mpl2.average_bandwidth = 3400000;
    mpl2.width = 1920;
    mpl2.height = 1080;
    mpl2.frame_rate = 60567;

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

TSUNIT_DEFINE_TEST(BuildMediaPlaylist)
{
    ts::hls::PlayList pl;
    pl.reset(ts::hls::PlayListType::LIVE, u"/c/test/path/master/test.m3u8");

    TSUNIT_ASSERT(pl.isValid());
    TSUNIT_EQUAL(ts::hls::PlayListType::LIVE, pl.type());
    TSUNIT_EQUAL(3, pl.version());
    TSUNIT_EQUAL(0, pl.segmentCount());
    TSUNIT_EQUAL(0, pl.playListCount());

    TSUNIT_ASSERT(pl.setMediaSequence(7));
    TSUNIT_ASSERT(pl.setTargetDuration(cn::seconds(5)));
    TSUNIT_ASSERT(!pl.endList());
    TSUNIT_ASSERT(pl.setEndList(true));
    TSUNIT_ASSERT(pl.endList());
    TSUNIT_ASSERT(pl.setType(ts::hls::PlayListType::VOD));
    TSUNIT_EQUAL(ts::hls::PlayListType::VOD, pl.type());

    ts::hls::MediaSegment seg1;
    seg1.relative_uri = u"/c/test/path/segments/seg-0001.ts";
    seg1.title = u"Segment1";
    seg1.duration = cn::milliseconds(4920);
    seg1.bitrate = 1233920;
    TSUNIT_ASSERT(pl.addSegment(seg1));

    ts::hls::MediaSegment seg2;
    seg2.relative_uri = u"/c/test/path/segments/seg-0002.ts";
    seg2.duration = cn::milliseconds(4971);
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
    seg3.relative_uri = u"/c/test/path/segments/seg-0003.ts";
    seg3.duration = cn::milliseconds(4984);
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
