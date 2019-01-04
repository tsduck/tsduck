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
//  Transport stream processor shared library:
//  HLS stream input / output
//
//  The input plugin can read HLS playlists and media segments from local
//  files or receive them in real time using HTTP or HTTPS.
//
//  The output plugin generates playlists and media segments on local files
//  only. It can also purge obsolete media segments and regenerate live
//  playlists. To setup a complete HLS server, it is necessary to setup an
//  external HTTP server such as Apache which simply serves these files.
//
//----------------------------------------------------------------------------

#include "tsAbstractHTTPInputPlugin.h"
#include "tsPluginRepository.h"
#include "tshlsPlayList.h"
#include "tsWebRequest.h"
#include "tsWebRequestArgs.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

#define DEFAULT_MAX_QUEUED_PACKETS  1000    // Default size in packet of the inter-thread queue.
#define DEFAULT_OUT_DURATION          10    // Default segment target duration for output streams.
#define DEFAULT_OUT_LIVE_DURATION      5    // Default segment target duration for output live streams.


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {

    // Input plugin
    class HlsInput: public AbstractHTTPInputPlugin
    {
    public:
        // Implementation of plugin API
        HlsInput(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool isRealTime() override {return true;}
        virtual void processInput() override;

    private:
        UString        _url;
        BitRate        _minRate;
        BitRate        _maxRate;
        size_t         _minWidth;
        size_t         _maxWidth;
        size_t         _minHeight;
        size_t         _maxHeight;
        bool           _listVariants;
        bool           _lowestRate;
        bool           _highestRate;
        bool           _lowestRes;
        bool           _highestRes;
        size_t         _maxSegmentCount;
        WebRequestArgs _web_args;
        hls::PlayList  _playlist;

        // Inaccessible operations
        HlsInput() = delete;
        HlsInput(const HlsInput&) = delete;
        HlsInput& operator=(const HlsInput&) = delete;
    };

    // Output plugin
    class HlsOutput: public OutputPlugin
    {
    public:
        // Implementation of plugin API
        HlsOutput(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override {return true;}
        virtual bool send(const TSPacket*, size_t) override;

    private:

        // Inaccessible operations
        HlsOutput() = delete;
        HlsOutput(const HlsOutput&) = delete;
        HlsOutput& operator=(const HlsOutput&) = delete;
     };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(hls, ts::HlsInput)
TSPLUGIN_DECLARE_OUTPUT(hls, ts::HlsOutput)


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::HlsInput::HlsInput(TSP* tsp_) :
    AbstractHTTPInputPlugin(tsp_, u"Receive HTTP Live Streaming (HLS) media", u"[options] url"),
    _url(),
    _minRate(0),
    _maxRate(0),
    _minWidth(0),
    _maxWidth(0),
    _minHeight(0),
    _maxHeight(0),
    _listVariants(false),
    _lowestRate(false),
    _highestRate(false),
    _lowestRes(false),
    _highestRes(false),
    _maxSegmentCount(0),
    _web_args(),
    _playlist()
{
    _web_args.defineOptions(*this);

    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"Specify the URL of an HLS manifest or playlist. "
         u"This is typically an URL ending in .m3u8. "
         u"The playlist can be either a master one, referencing several versions "
         u"of the same content (with various bitrates or resolutions). "
         u"The playlist can also be a media playlist, referencing all segments "
         u"of one single content.");

    option(u"lowest-bitrate");
    help(u"lowest-bitrate", u"When the URL is a master playlist, use the content with the lowest bitrate.");

    option(u"highest-bitrate");
    help(u"highest-bitrate", u"When the URL is a master playlist, use the content with the highest bitrate.");

    option(u"lowest-resolution");
    help(u"lowest-resolution", u"When the URL is a master playlist, use the content with the lowest screen resolution.");

    option(u"highest-resolution");
    help(u"highest-resolution", u"When the URL is a master playlist, use the content with the highest screen resolution.");

    option(u"list-variants", 'l');
    help(u"list-variants", u"When the URL is a master playlist, list all possible streams bitrates and resolutions.");

    option(u"min-bitrate", 0, UINT32);
    help(u"min-bitrate", u"When the URL is a master playlist, select a content the bitrate of which is higher than the specified minimum.");

    option(u"max-bitrate", 0, UINT32);
    help(u"max-bitrate", u"When the URL is a master playlist, select a content the bitrate of which is lower than the specified maximum.");

    option(u"min-width", 0, UINT32);
    help(u"min-width", u"When the URL is a master playlist, select a content the resolution of which has a higher width than the specified minimum.");

    option(u"max-width", 0, UINT32);
    help(u"max-width", u"When the URL is a master playlist, select a content the resolution of which has a lower width than the specified maximum.");

    option(u"min-height", 0, UINT32);
    help(u"min-height", u"When the URL is a master playlist, select a content the resolution of which has a higher height than the specified minimum.");

    option(u"max-height", 0, UINT32);
    help(u"max-height", u"When the URL is a master playlist, select a content the resolution of which has a lower height than the specified maximum.");

    option(u"max-queue", 0, POSITIVE);
    help(u"max-queue",
         u"Specify the maximum number of queued TS packets before their insertion into the stream. "
         u"The default is " + UString::Decimal(DEFAULT_MAX_QUEUED_PACKETS) + u".");

    option(u"segment-count", 's', POSITIVE);
    help(u"segment-count",
         u"Stop receiving the HLS stream after receiving the specified number of media segments. "
         u"By default, receive the complete content.");
}


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::HlsOutput::HlsOutput(TSP* tsp_) :
    OutputPlugin(tsp_, u"Generate HTTP Live Streaming (HLS) media", u"[options] filename")
{
    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"Specify the name template of the output media segment files. "
         u"A number is automatically added to the name part so that successive segment "
         u"files receive distinct names. Example: if the specified file name is foo-.ts, "
         u"the various segment files are named foo-000000.ts, foo-000001.ts, etc.\n\n"
         u"If the specified template already contains trailing digits, this unmodified "
         u"name is used for the first segment. Then, the integer part is incremented. "
         u"Example: if the specified file name is foo-027.ts, the various segment files "
         u"are named foo-027.ts, foo-028.ts, etc.");

    option(u"duration", 'd', POSITIVE);
    help(u"duration",
         u"Specify the target duration in seconds of media segments. "
         u"The default is " TS_STRINGIFY(DEFAULT_OUT_DURATION) u" seconds for VoD streams "
         u"and " TS_STRINGIFY(DEFAULT_OUT_LIVE_DURATION) u" seconds for live streams.");

    option(u"live", 'l', POSITIVE);
    help(u"live",
         u"Specify that the output is a live stream. The specified value indicates the "
         u"number of simultaneously available media segments. Obsolete media segment files "
         u"are automatically deleted. By default, the output stream is considered as VoD "
         u"and all created media segments are preserved.");

    option(u"playlist", 'p', STRING);
    help(u"playlist", u"filename",
         u"Specify the name of the playlist file. "
         u"The playlist file is rewritten each time a new segment file is completed or an obsolete one is deleted. "
         u"The playlist and the segment files can be written to distinct directories but, in all cases, "
         u"the URI of the segment files in the playlist are always relative to the playlist location. "
         u"By default, no playlist file is created (media segments only).");
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::HlsInput::getOptions()
{
    // Decode options.
    _web_args.loadArgs(*this);
    getValue(_url, u"");
    getIntValue(_maxSegmentCount, u"segment-count");
    getIntValue(_minRate, u"min-bitrate");
    getIntValue(_maxRate, u"max-bitrate");
    getIntValue(_minWidth, u"min-width");
    getIntValue(_maxWidth, u"max-width");
    getIntValue(_minHeight, u"min-height");
    getIntValue(_maxHeight, u"max-height");
    _lowestRate = present(u"lowest-bitrate");
    _highestRate = present(u"highest-bitrate");
    _lowestRes = present(u"lowest-resolution");
    _highestRes = present(u"highest-resolution");
    _listVariants = present(u"list-variants");

    // Check consistency of selection options.
    const int singleSelect = _lowestRate + _highestRate + _lowestRes + _highestRes;
    const int multiSelect = (_minRate > 0) + (_maxRate > 0) + (_minWidth > 0) + (_maxWidth > 0) + (_minHeight > 0) + (_maxHeight > 0);
    if (singleSelect > 1) {
        tsp->error(u"specify only one of --lowest-bitrate, --highest-bitrate, --lowest-resolution, --highest-resolution");
        return false;
    }
    if (singleSelect > 0 && multiSelect > 0) {
        tsp->error(u"incompatible combination of stream selection options");
        return false;
    }

    // Resize the inter-thread packet queue.
    setQueueSize(intValue<size_t>(u"max-queue", DEFAULT_MAX_QUEUED_PACKETS));

    return true;
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::HlsInput::start()
{
    // Load the HLS playlist, can be a master playlist or a media playlist.
    _playlist.clear();
    if (!_playlist.loadURL(_url, false, _web_args, hls::UNKNOWN_PLAYLIST, *tsp)) {
        return false;
    }

    // In the case of a master play list, select one media playlist.
    if (_playlist.type() == hls::MASTER_PLAYLIST) {
        tsp->verbose(u"downloaded %s", {_playlist});

        // List all variants when requested.
        if (_listVariants) {
            for (size_t i = 0; i < _playlist.playListCount(); ++i) {
                tsp->info(_playlist.playList(i).toString());
            }
        }

        // Apply command line selection criteria.
        size_t index = 0;
        if (_lowestRate) {
            index = _playlist.selectPlayListLowestBitRate();
        }
        else if (_highestRate) {
            index = _playlist.selectPlayListHighestBitRate();
        }
        else if (_lowestRes) {
            index = _playlist.selectPlayListLowestResolution();
        }
        else if (_highestRes) {
            index = _playlist.selectPlayListHighestResolution();
        }
        else {
            index = _playlist.selectPlayList(_minRate, _maxRate, _minWidth, _maxWidth, _minHeight, _maxHeight);
        }
        if (index == NPOS) {
            tsp->error(u"could not find a matching stream in master playlist");
            return false;
        }
        assert(index < _playlist.playListCount());
        tsp->verbose(u"selected playlist: %s", {_playlist.playList(index)});

        // Download selected media playlist.
        const UString nextURL(_playlist.buildURL(_playlist.playList(index).uri));
        _playlist.clear();
        if (!_playlist.loadURL(nextURL, false, _web_args, hls::UNKNOWN_PLAYLIST, *tsp)) {
            return false;
        }
    }

    // Now, we must have a media playlist.
    if (_playlist.type() != hls::MEDIA_PLAYLIST) {
        tsp->error(u"invalid HLS playlist type, expected a media playlist");
        return false;
    }
    if (_playlist.segmentCount() == 0) {
        tsp->error(u"empty HLS media playlist");
        return false;
    }
    tsp->verbose(u"downloaded %s", {_playlist});

    // Invoke superclass.
    return AbstractHTTPInputPlugin::start();
}


//----------------------------------------------------------------------------
// Input method. Executed in a separate thread.
//----------------------------------------------------------------------------

void ts::HlsInput::processInput()
{
    // Loop on all segments in the media playlists.
    for (size_t count = 0; _playlist.segmentCount() > 0 && (_maxSegmentCount == 0 || count < _maxSegmentCount) && !tsp->aborting(); ++count) {

        // Remove first segment from the playlist.
        hls::MediaSegment seg;
        _playlist.popFirstSegment(seg);

        // Create a Web request to download the content.
        WebRequest request(*tsp);
        request.setURL(_playlist.buildURL(seg.uri));
        request.setAutoRedirect(true);
        request.setArgs(_web_args);

        // Perform the download of the current segment.
        // Ignore errors, continue to play next segments.
        request.downloadToApplication(this);

        // If there is only one or zero remaining segment, try to reload the playlist.
        if (_playlist.segmentCount() < 2 && _playlist.updatable() && !tsp->aborting()) {

            // Ignore errors, continue to play next segments.
            _playlist.reload(false, _web_args, *tsp);

            // If the playout is still empty, this means that we have read all segments before the server
            // could produce new segments. For live streams, this is possible because new segments
            // can be produced as late as the estimated end time of the previous playlist. So, we retry
            // at regular intervals until we get new segments.

            while (_playlist.segmentCount() == 0 && Time::CurrentUTC() <= _playlist.terminationUTC() && !tsp->aborting()) {
                // The wait between two retries is half the target duration of a segment, with a minimum of 2 seconds.
                SleepThread(std::max<MilliSecond>(2000, (MilliSecPerSec * _playlist.targetDuration()) / 2));
                // This time, we stop on error.
                if (!_playlist.reload(false, _web_args, *tsp)) {
                    break;
                }
            }
        }
    }
    tsp->verbose(u"HLS playlist completed");
}


//----------------------------------------------------------------------------
// Output command line options method
//----------------------------------------------------------------------------

bool ts::HlsOutput::getOptions()
{
    return true;
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::HlsOutput::start()
{
    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::HlsOutput::stop()
{
    return true;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::HlsOutput::send(const TSPacket* pkt, size_t packet_count)
{
    return true;
}
