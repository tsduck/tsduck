//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsFileUtils.h"
#include "tsAES128.h"
#include "tsAES256.h"

#if !defined(TS_UNIX) || !defined(TS_NO_CURL)
TS_REGISTER_INPUT_PLUGIN(u"hls", ts::hls::InputPlugin);
#endif


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::hls::InputPlugin::InputPlugin(TSP* tsp_) :
    AbstractHTTPInputPlugin(tsp_, u"Receive HTTP Live Streaming (HLS) media", u"[options] url")
{
    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"Specify the URL of an HLS manifest or playlist. "
         u"This is typically an URL ending in .m3u8. "
         u"The playlist can be either a master one, referencing several versions "
         u"of the same content (with various bitrates or resolutions). "
         u"The playlist can also be a media playlist, referencing all segments "
         u"of one single content.");

    option(u"alt-group-id", 0, STRING);
    help(u"alt-group-id", u"'string'",
         u"When the URL is a master playlist, use the 'alternative rendition content' with the specified group id. "
         u"If several --alt-* options are specified, the selected 'alternative rendition content' must match all of them.");

    option(u"alt-language", 0, STRING);
    help(u"alt-language", u"'string'",
         u"When the URL is a master playlist, use the first 'alternative rendition content' with the specified language. "
         u"If several --alt-* options are specified, the selected 'alternative rendition content' must match all of them.");

    option(u"alt-name", 0, STRING);
    help(u"alt-name", u"'string'",
         u"When the URL is a master playlist, use the 'alternative rendition content' with the specified name. "
         u"If several --alt-* options are specified, the selected 'alternative rendition content' must match all of them.");

    option(u"alt-type", 0, STRING);
    help(u"alt-type", u"'string'",
         u"When the URL is a master playlist, use the first 'alternative rendition content' with the specified type. "
         u"If several --alt-* options are specified, the selected 'alternative rendition content' must match all of them.");

    option(u"lowest-bitrate");
    help(u"lowest-bitrate",
         u"When the URL is a master playlist, use the content with the lowest bitrate.");

    option(u"highest-bitrate");
    help(u"highest-bitrate",
         u"When the URL is a master playlist, use the content with the highest bitrate.");

    option(u"lowest-resolution");
    help(u"lowest-resolution",
         u"When the URL is a master playlist, use the content with the lowest screen resolution.");

    option(u"highest-resolution");
    help(u"highest-resolution",
         u"When the URL is a master playlist, use the content with the highest screen resolution.");

    option(u"list-variants", 'l');
    help(u"list-variants",
         u"When the URL is a master playlist, list all possible streams bitrates and resolutions.");

    option<BitRate>(u"min-bitrate");
    help(u"min-bitrate",
         u"When the URL is a master playlist, select a content the bitrate of which is higher "
         u"than the specified minimum.");

    option<BitRate>(u"max-bitrate");
    help(u"max-bitrate",
         u"When the URL is a master playlist, select a content the bitrate of which is lower "
         u"than the specified maximum.");

    option(u"min-width", 0, UINT32);
    help(u"min-width",
         u"When the URL is a master playlist, select a content the resolution of which has a "
         u"higher width than the specified minimum.");

    option(u"max-width", 0, UINT32);
    help(u"max-width",
         u"When the URL is a master playlist, select a content the resolution of which has a "
         u"lower width than the specified maximum.");

    option(u"min-height", 0, UINT32);
    help(u"min-height",
         u"When the URL is a master playlist, select a content the resolution of which has a "
         u"higher height than the specified minimum.");

    option(u"max-height", 0, UINT32);
    help(u"max-height",
         u"When the URL is a master playlist, select a content the resolution of which has a "
         u"lower height than the specified maximum.");

    option(u"save-files", 0, DIRECTORY);
    help(u"save-files",
         u"Specify a directory where all downloaded files, media segments and playlists, are saved "
         u"before being passed to the next plugin. "
         u"This is typically a debug option to analyze the input HLS structure.");

    option(u"segment-count", 's', POSITIVE);
    help(u"segment-count",
         u"Stop receiving the HLS stream after receiving the specified number of media segments. "
         u"By default, receive the complete content.");

    option(u"live");
    help(u"live",
         u"Specify that the input is a live stream and the playout shall start at the last segment in the playlist.\n"
         u"This is an alias for --start-segment -1.");

    option(u"start-segment", 0, INT32);
    help(u"start-segment",
         u"Start at the specified segment in the initial playlist. "
         u"By default, start with the first media segment.\n\n"
         u"The value can be positive or negative. "
         u"Positive values are indexes from the start of the playlist: "
         u"0 is the first segment (the default), +1 is the second segment, etc. "
         u"Negative values are indexes from the end of the playlist: "
         u"-1 is the last segment, -2 is the preceding segment, etc.");
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::hls::InputPlugin::isRealTime()
{
    return true;
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::hls::InputPlugin::getOptions()
{
    _url.setURL(value(u""));
    const UString save_directory(value(u"save-files"));
    getIntValue(_max_segment_count, u"segment-count");
    getValue(_min_rate, u"min-bitrate");
    getValue(_max_rate, u"max-bitrate");
    getIntValue(_min_width, u"min-width");
    getIntValue(_max_width, u"max-width");
    getIntValue(_min_height, u"min-height");
    getIntValue(_max_height, u"max-height");
    getIntValue(_start_segment, u"start-segment");
    _lowest_rate = present(u"lowest-bitrate");
    _highest_rate = present(u"highest-bitrate");
    _lowest_res = present(u"lowest-resolution");
    _highest_res = present(u"highest-resolution");
    _list_variants = present(u"list-variants");

    getValue(_alt_group_id, u"alt-group-id");
    getValue(_alt_language, u"alt-language");
    getValue(_alt_name, u"alt-name");
    getValue(_alt_type, u"alt-type");
    _alt_selection = !_alt_group_id.empty() || !_alt_language.empty() || !_alt_name.empty() || !_alt_type.empty();

    // Invoke superclass to initialize webArgs.
    AbstractHTTPInputPlugin::getOptions();

    // Enable authentication tokens from master playlist to media playlist and from media playlists to media segments.
    // On Linux and macOS, use a specific cookies file to make sure that all Web requests use the same one.
    web_args.use_cookies = true;
    web_args.cookies_file = TempFile(u".cookies");

    if (present(u"live")) {
        // With live streams, start at the last segment.
        if (_start_segment != 0) {
            error(u"--live and --start-segment are mutually exclusive");
            return false;
        }
        _start_segment = -1;
    }

    if (!_url.isValid()) {
        error(u"invalid URL");
        return false;
    }

    // Check consistency of selection options.
    const int single_select = _lowest_rate + _highest_rate + _lowest_res + _highest_res;
    const bool multi_select = _min_rate > 0 || _max_rate > 0 || _min_width > 0 || _max_width > 0 || _min_height > 0 || _max_height > 0;

    if (single_select > 1) {
        error(u"specify only one of --lowest-bitrate, --highest-bitrate, --lowest-resolution, --highest-resolution");
        return false;
    }
    if (single_select > 0 && multi_select) {
        error(u"incompatible combination of stream selection options");
        return false;
    }
    if (_alt_selection && (single_select > 0 || multi_select)) {
        error(u"--alt-* options and incompatible with main stream selection options");
        return false;
    }

    // Automatically save media segments and playlists.
    setAutoSaveDirectory(save_directory);
    _playlist.setAutoSaveDirectory(save_directory);

    return true;
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::hls::InputPlugin::start()
{
    _seg_key.clear();
    _seg_iv.clear();
    _seg_data.clear();
    _seg_size  = _seg_next = _seg_end = 0;

    // Load the HLS playlist, can be a master playlist or a media playlist.
    _playlist.clear();
    if (!_playlist.loadURL(_url.toString(), false, web_args, hls::PlayListType::UNKNOWN, *this)) {
        return false;
    }

    // In the case of a master play list, select one media playlist.
    if (_playlist.type() == hls::PlayListType::MASTER) {
        verbose(u"downloaded %s", _playlist);

        // Get a copy of the master playlist. The media playlist will be loaded in _playlist.
        PlayList master(_playlist);

        // List all variants when requested.
        if (_list_variants) {
            for (size_t i = 0; i < master.playListCount(); ++i) {
                info(master.playList(i).toString());
            }
            if (master.altPlayListCount() > 0) {
                info(u"%s alternative rendition contents:", master.altPlayListCount());
                for (size_t i = 0; i < master.altPlayListCount(); ++i) {
                    info(master.altPlayList(i).toString());
                }
            }
        }

        // Apply command line selection criteria.
        if (_alt_selection) {
            // Select an 'alternative rendition' playlist according to --alt-* parameters.
            _playlist.clear();
            const size_t index = master.selectAltPlayList(_alt_type, _alt_name, _alt_group_id, _alt_language);
            if (index == NPOS) {
                error(u"no alternative rendition media playlist found with selected criteria");
                return false;
            }
            else {
                assert(index < master.altPlayListCount());
                verbose(u"selected playlist: %s", master.altPlayList(index));
                if (!_playlist.loadURL(master.altPlayList(index).urlString(), false, web_args, hls::PlayListType::UNKNOWN, *this)) {
                    return false;
                }
            }
        }
        else {
            // Select a main content playlist.
            // Loop until one media playlist is loaded (skip missing playlists).
            for (;;) {
                size_t index = 0;
                if (_lowest_rate) {
                    index = master.selectPlayListLowestBitRate();
                }
                else if (_highest_rate) {
                    index = master.selectPlayListHighestBitRate();
                }
                else if (_lowest_res) {
                    index = master.selectPlayListLowestResolution();
                }
                else if (_highest_res) {
                    index = master.selectPlayListHighestResolution();
                }
                else {
                    index = master.selectPlayList(_min_rate, _max_rate, _min_width, _max_width, _min_height, _max_height);
                }
                if (index == NPOS) {
                    error(u"could not find a matching stream in master playlist");
                    return false;
                }
                assert(index < master.playListCount());
                verbose(u"selected playlist: %s", master.playList(index));
                const UString nextURL(master.playList(index).urlString());

                // Download selected media playlist.
                _playlist.clear();
                if (_playlist.loadURL(nextURL, false, web_args, hls::PlayListType::UNKNOWN, *this)) {
                    break; // media playlist loaded
                }
                else if (master.playListCount() == 1) {
                    error(u"no more media playlist to try, giving up");
                    return false;
                }
                else {
                    // Remove the failing playlist and retry playlist selection.
                    master.deletePlayList(index);
                }
            }
        }
    }

    // Now, we must have a media playlist.
    if (!_playlist.isMedia()) {
        error(u"invalid HLS playlist type, expected a media playlist");
        return false;
    }
    verbose(u"downloaded %s", _playlist);

    // Manage the number of media segments and starting point.
    size_t seg_count = _playlist.segmentCount();
    if (seg_count == 0) {
        error(u"empty HLS media playlist");
        return false;
    }
    else if (_start_segment > 0) {
        // Start index from the start of playlist.
        if (seg_count + 1 < size_t(_start_segment)) {
            warning(u"playlist has only %d segments, starting at last one", seg_count);
            seg_count = 1;
        }
        else {
            // Remaining number of segments to play.
            seg_count = seg_count - size_t(_start_segment);
        }
    }
    else if (_start_segment < 0) {
        // Start index from the end of playlist.
        if (seg_count < size_t(- _start_segment)) {
            warning(u"playlist has only %d segments, starting at first one", seg_count);
        }
        else {
            // Remaining number of segments to play.
            seg_count = size_t(- _start_segment);
        }
    }

    // If the start point is not the first segment, then drop unused initial segments.
    while (_playlist.segmentCount() > seg_count) {
        _playlist.popFirstSegment();
        debug(u"dropped initial segment, %d remaining segments", _playlist.segmentCount());
    }

    _segment_count = 0;

    // Invoke superclass.
    return AbstractHTTPInputPlugin::start();
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::hls::InputPlugin::stop()
{
    // Invoke superclass first.
    const bool stopped = AbstractHTTPInputPlugin::stop();

    // Then delete the cookie file. Must be done after complete stop to avoid recreation.
    return deleteCookiesFile() && stopped;
}


//----------------------------------------------------------------------------
// Called by AbstractHTTPInputPlugin to open an URL.
//----------------------------------------------------------------------------

bool ts::hls::InputPlugin::openURL(WebRequest& request)
{
    // Check if the playlist is completed
    bool completed =
        // the playlist is originally empty
        (_segment_count == 0 && _playlist.segmentCount() == 0) ||
        // reached maximum number of segments
        (_max_segment_count > 0 && _segment_count >= _max_segment_count) ||
        // user interruption
        tsp->aborting();

    // If there is only one or zero remaining segment, try to reload the playlist.
    if (!completed && _playlist.segmentCount() < 2 && _playlist.isUpdatable()) {

        // Reload the playlist, ignore errors, continue to play next segments.
        _playlist.reload(false, web_args, *this);

        // If the playlist is still empty, this means that we have read all segments before the server
        // could produce new segments. For live streams, this is possible because new segments
        // can be produced as late as the estimated end time of the previous playlist. So, we retry
        // at regular intervals until we get new segments.

        while (_playlist.segmentCount() == 0 && Time::CurrentUTC() <= _playlist.terminationUTC() && !tsp->aborting()) {
            // The wait between two retries is half the target duration of a segment, with a minimum of 2 seconds.
            std::this_thread::sleep_for(std::max(cn::seconds(2), _playlist.targetDuration() / 2));
            // This time, we stop on reload error.
            if (!_playlist.reload(false, web_args, *this)) {
                break;
            }
        }

        // End of playlist if we cannot find new segments.
        completed = _playlist.segmentCount() == 0;
    }

    if (completed) {
        verbose(u"HLS playlist completed");
        return false;
    }

    // Remove first segment from the playlist.
    MediaSegment seg;
    _playlist.popFirstSegment(seg);
    _segment_count++;

    // Prepare decryption in case of encrypted HLS.
    if (!initSegmentDecryption(request, seg)) {
        return false;
    }

    // Open the segment.
    const UString seg_url(seg.urlString());
    debug(u"downloading segment %s", seg_url);
    request.enableCookies(web_args.cookies_file);
    return request.open(seg_url);
}


//----------------------------------------------------------------------------
// Prepare segment decryption.
//----------------------------------------------------------------------------

bool ts::hls::InputPlugin::initSegmentDecryption(WebRequest& request, const MediaSegment& seg)
{
    // Encryption method is one of:
    // - NONE : no encryption.
    // - AES-128 : raw AES-128 CBC with explicit IV in #EXT-X-KEY.
    // - SAMPLE-AES : encryption of audio/video samples, unsupported in TSDuck.
    // - SAMPLE-AES-CTR (optional) : encryption of audio/video samples, unsupported in TSDuck.
    // - AES-256-GCM (optional) : raw AES-256 GCM with 128-bit IV at start of segment and 128-bit authentication tag at end of segment.

    size_t required_key_size = 0;
    size_t required_iv_size = 0;

    if (!seg.key.isEncrypted()) {
        // Unencrypted segment.
        _seg_key.clear();
        _seg_iv.clear();
        _seg_data.clear();
        _seg_size = _seg_next = _seg_end = 0;
        return true;
    }
    else if (seg.key.method.similar(u"AES-128")) {
        required_key_size = AES128::KEY_SIZE;
        required_iv_size = AES128::BLOCK_SIZE;
    }
    else if (seg.key.method.similar(u"AES-256-GCM")) {
        required_key_size = AES256::KEY_SIZE;
        required_iv_size = 0;  // will be at start of segment, not in #EXT-X-KEY
        // TODO: add AES-256-GCM support
        error(u"HLS encryption method \"%s\" is not supported yet, maybe later", seg.key.method);
        return false;
    }
    else {
        error(u"HLS encryption method \"%s\" is not supported", seg.key.method);
        return false;
    }

    // Check initialization vector from #EXT-X-KEY.
    if (seg.key.iv.size() != required_iv_size) {
        error(u"invalid initialization vector size for HLS encryption method \"%s\", got %d bytes, expected %d", seg.key.method, seg.key.iv.size(), required_iv_size);
        return false;
    }
    _seg_iv = seg.key.iv;

    // Load decryption key.
    if (!seg.key.isRawEncryptionKey()) {
        error(u"HLS encryption key format \"%s\" is not supported", seg.key.format);
        return false;
    }
    const UString key_url(seg.key.urlString());
    if (key_url.empty()) {
        error(u"no decryption key specified for HLS encryption method \"%s\"", seg.key.method);
        return false;
    }
    debug(u"downloading decryption key %s", key_url);
    if (!request.downloadBinaryContent(key_url, _seg_key)) {
        return false;
    }
    if (_seg_key.size() != required_key_size) {
        error(u"invalid key size for HLS encryption method \"%s\", got %d bytes, expected %d", seg.key.method, _seg_key.size(), required_key_size);
        return false;
    }

    // The possible segment size is computed from the segment bitrate and duraction, if specified.
    _seg_size = size_t(ByteDistance(seg.bitrate, seg.duration));
    _seg_next = NPOS;
    _seg_end = 0;

    return true;
}


//----------------------------------------------------------------------------
// Receive data from the URL.
//----------------------------------------------------------------------------

bool ts::hls::InputPlugin::receiveURL(WebRequest& request, void* buffer, size_t max_size, size_t& ret_size)
{
    ret_size = 0;

    // With clear segment (most common case), let the superclass do the job.
    if (_seg_key.empty()) {
        return AbstractHTTPInputPlugin::receiveURL(request, buffer, max_size, ret_size);
    }

    // The first time, download the entire encrypted segment in memory.
    if (_seg_next == NPOS) {
        // Try to preallocate a large enough buffer.
        static constexpr size_t chunk_size = WebRequest::DEFAULT_CHUNK_SIZE;
        const size_t size = std::max(chunk_size, std::max(_seg_size, request.announcedContentSize())) + 128;
        if (size > _seg_data.size()) {
            _seg_data.resize(size);
        }
        _seg_next = _seg_end = 0;

        // Download the encrypted segment.
        for (;;) {
            // Download one chunk.
            size_t this_size = 0;
            if (!request.receive(_seg_data.data() + _seg_end, _seg_data.size() - _seg_end, this_size)) {
                return false;
            }
            _seg_end += std::min(this_size, _seg_data.size() - _seg_end);

            // Error or end of transfer.
            if (this_size == 0) {
                break;
            }

            // Enlarge the buffer for next chunk. Don't do that too often in case of very short transfers.
            if (_seg_data.size() - _seg_end < chunk_size / 2) {
                _seg_data.resize(_seg_end + chunk_size);
            }
        }

        // Decrypt the segment.
        // Currently, only AES-128-CBC is supported.
        debug(u"decrypting segment, %d bytes, %d-bit key", _seg_end, 8 * _seg_key.size());
        CBC<AES128> algo;
        if (!algo.setKey(_seg_key, _seg_iv)) {
            error(u"error setting key/IV while decrypting HLS segment, segment size: %d, key size: %d, IV size: %d bytes", _seg_next, _seg_key.size(), _seg_iv.size());
            return false;
        }
        if (!algo.decrypt(_seg_data.data(), _seg_end, _seg_data.data(), _seg_data.size(), &_seg_end)) {
            error(u"error decrypting HLS segment, segment size: %d, key size: %d, IV size: %d bytes", _seg_next, _seg_key.size(), _seg_iv.size());
            return false;
        }

        // AES-CBC encrypted block must be a multiple of the block size.
        if (_seg_end < AES128::BLOCK_SIZE || _seg_end % AES128::BLOCK_SIZE != 0) {
            error(u"invalid decrypted HLS segment size: %d", _seg_next);
            return false;
        }

        // Check and remove PKCS #7 padding.
        const size_t pad = _seg_data[_seg_end - 1];
        bool pad_valid = pad <= AES128::BLOCK_SIZE;
        for (size_t index = 2; pad_valid && index <= pad; ++index) {
            pad_valid = _seg_data[_seg_end - index] == pad;
        }
        if (!pad_valid) {
            error(u"invalid PKCS #7 padding in decrypted HLS segment, segment size: %d, last block: %s",
                  _seg_next, UString::Dump(_seg_data.data() + _seg_end - AES128::BLOCK_SIZE, AES128::BLOCK_SIZE, UString::SINGLE_LINE));
            return false;
        }
        _seg_end -= pad;

        // Check that we got plausible TS packets.
        if (_seg_end > 0) {
            if (_seg_data[0] != SYNC_BYTE) {
                error(u"invalid decrypted HLS segment, start with 0x%02X instead of 0x%02X", _seg_data[0], SYNC_BYTE);
                return false;
            }
            if (_seg_end % PKT_SIZE != 0) {
                warning(u"suspect decrypted HLS segment size: %d, not a multiple of %d", _seg_next, PKT_SIZE);
            }
        }
    }

    // Return part of decrypted segment.
    assert(_seg_next <= _seg_end);
    ret_size = std::min(max_size, _seg_end - _seg_next);
    MemCopy(buffer, _seg_data.data() + _seg_next, ret_size);
    _seg_next += ret_size;
    return true;
}
