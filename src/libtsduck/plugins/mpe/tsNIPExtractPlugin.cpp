//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIPExtractPlugin.h"
#include "tsPluginRepository.h"
#include "tsMPEPacket.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"nipextract", ts::mcast::NIPExtractPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::mcast::NIPExtractPlugin::NIPExtractPlugin(TSP* tsp_) :
    AbstractSingleMPEPlugin(tsp_, u"DVB-NIP (Native IP) live service extraction", u"[options]", u"DVB-NIP stream")
{
    option(u"lcn", 'l', UINT32);
    help(u"lcn",
         u"Logical channel number of the DVB-NIP service to extract. "
         u"If neither --lcn nor --name are specified, extract the first service that is found.");

    option(u"name", 'n', STRING);
    help(u"name", u"'string'",
         u"Name of the DVB-NIP service to extract. "
         u"The name is case-insensitive and blanks are ignored. "
         u"If neither --lcn nor --name are specified, extract the first service that is found.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::mcast::NIPExtractPlugin::getOptions()
{
    _use_first_service = !present(u"lcn") && !present(u"name");
    getIntValue(_lcn, u"lcn");
    getValue(_service_name, u"name");

    if (present(u"lcn") && present(u"name")) {
        error(u"--lcn and --name are mutually exclusive");
        return false;
    }

    return AbstractSingleMPEPlugin::getOptions();
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::mcast::NIPExtractPlugin::start()
{
    _playlist_url.clear();
    _playlist.clear();
    _service_session.clear();
    _output_next = 0;
    _output.clear();
    _initial_playlist_cache.clear();
    _ahead_segment_cache.clear();
    _last_segment_path.clear();
    _last_segment_ext.clear();

    return AbstractSingleMPEPlugin::start() && _demux.reset(FluteDemuxArgs());
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::PacketProcessStatus ts::mcast::NIPExtractPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Call superclass to filter and decapsulate MPE.
    PacketProcessStatus status = AbstractSingleMPEPlugin::processPacket(pkt, pkt_data);

    // If superclass does not want to terminate, pull a replacement packet from the extracted service.
    if (status != TSP_END) {
        if (_output.empty()) {
            // Output queue empty, drop packet.
            status = TSP_DROP;
        }
        else {
            // Get next packet to output.
            assert(_output_next + PKT_SIZE <= _output.front()->size());
            pkt.copyFrom(_output.front()->data() + _output_next);
            _output_next += PKT_SIZE;
            status = TSP_OK;

            // Drop completed or empty segments if necessary.
            while (!_output.empty() && _output_next + PKT_SIZE > _output.front()->size()) {
                _output.pop_front();
                _output_next = 0;
            }
        }
    }

    return status;
}


//----------------------------------------------------------------------------
// MPE packet processing method
//----------------------------------------------------------------------------

void ts::mcast::NIPExtractPlugin::handleSingleMPEPacket(PCR timestamp, TimeSource source, const MPEPacket& mpe)
{
    _demux.feedPacket(timestamp, mpe.sourceSocket(), mpe.destinationSocket(), mpe.udpMessage(), mpe.udpMessageSize());
}


//----------------------------------------------------------------------------
// Check if a file can be a HLS playlist.
//----------------------------------------------------------------------------

bool ts::mcast::NIPExtractPlugin::IsValidPlayListName(const UString& file_name, const UString& file_type)
{
    return file_type.similar(u"application/vnd.apple.mpegurl") || file_name.ends_with(u".m3u8", CASE_INSENSITIVE);
}


//----------------------------------------------------------------------------
// Invoked when a new DVB-NIP service is found.
//----------------------------------------------------------------------------

void ts::mcast::NIPExtractPlugin::handleNewService(const NIPService& service)
{
    debug(u"new service '%s', LCN: %d", service.service_name, service.channel_number);

    // Ignore new services when ours is already found.
    if (!_playlist_url.empty()) {
        return;
    }

    // Check service name or LCN.
    bool found = false;
    if (!_service_name.empty()) {
        found = _service_name.similar(service.service_name);
    }
    else if (!_use_first_service) {
        found = _lcn == service.channel_number;
    }

    // Look for an instance with HLS playlist, hoping it is made of TS segments.
    if (found || _use_first_service) {
        for (const auto& ins : service.instances) {
            // Does it look like a HLS playlist?
            if (IsValidPlayListName(ins.first, ins.second.media_type)) {
                _playlist_url = ins.first;
                found = true;
                break;
            }
        }
    }

    if (found) {
        if (_playlist_url.empty()) {
            // This was an explicit service, by LCN or by name.
            error(u"no HLS instance found for service '%s', LCN: %d", service.service_name, service.channel_number);
            setError();
        }
        else {
            verbose(u"using service '%s', LCN: %d, provider '%s'", service.service_name, service.channel_number, service.provider_name);
            debug(u"service playlist: %s", _playlist_url);
            // Check if we already received that playlist.
            const auto it = _initial_playlist_cache.find(_playlist_url);
            if (it != _initial_playlist_cache.end()) {
                processPlayList(it->second);
            }
            // Clear the cache of initial playlists (no longer need it).
            _initial_playlist_cache.clear();
        }
    }
}


//----------------------------------------------------------------------------
// Process an update of the playlist of the service.
//----------------------------------------------------------------------------

void ts::mcast::NIPExtractPlugin::processPlayList(const FluteFile& file)
{
    // Load or reload the playlist.
    debug(u"%sload playlist %s", _playlist.isValid() ? u"re" : u"", file.name());
    bool success = _playlist.isValid() ?
                       _playlist.reloadText(file.toText(), false, *this) :
                       _playlist.loadText(file.toText(), false, hls::PlayListType::UNKNOWN, *this);
    if (!success) {
        error(u"error reloading service playlist");
        return;
    }
    _playlist.setURL(_playlist_url, *this);
    debug(u"loaded a %s playlist", hls::PlayListTypeNames().name(_playlist.type()));

    if (_playlist.isMaster()) {
        // In case of master playlist, select a media playlist.
        // Find the playlist with highest resolution.
        const size_t pl_index = _playlist.selectPlayListHighestResolution();
        if (pl_index == NPOS) {
            error(u"could not find a media playlist from the master playlist %s", _playlist_url);
            setError();
            return;
        }

        // Replace the playlist of the service with the media playlist.
        const hls::MediaPlayList& pl(_playlist.playList(pl_index));
        _playlist_url = pl.urlString();
        _playlist.clear();
        debug(u"selected media playlist: %s", _playlist_url);

        // Check if we already received that media playlist.
        const auto it = _initial_playlist_cache.find(_playlist_url);
        if (it != _initial_playlist_cache.end()) {
            processPlayList(it->second);
        }
    }
    else if (_playlist.isMedia()) {
        // In case of media playlist, process and empty the case of possible future segments.
        // Each file in the cache is processed in order of arrival. All files are removed from
        // the cache, either they are known in the new playlist and their content is enqueues
        // for output, or they will never be part of a future playlist.
        while (!_ahead_segment_cache.empty()) {
            const bool ok = processSegment(_ahead_segment_cache.front());
            debug(u"remove from cache %s %s", ok ? u"segment" : u"unused file", _ahead_segment_cache.front().name());
            _ahead_segment_cache.pop_front();
        }
    }
}


//----------------------------------------------------------------------------
// Invoked for each FLUTE file.
//----------------------------------------------------------------------------

void ts::mcast::NIPExtractPlugin::handleFluteFile(const FluteFile& file)
{
    debug(u"got file %s", file.name());

    // Ignore all files as long as the playlist is unknown.
    if (_playlist_url.empty()) {
        // Cache initial playlists (see comments in class declaration).
        if (IsValidPlayListName(file.name(), file.type())) {
            debug(u"cache initial playlist %s, toi %d", file.name(), file.toi());
            _initial_playlist_cache[file.name()] = file;
        }
        return;
    }

    // Reload the service playlist when found.
    if (file.name() == _playlist_url) {
        processPlayList(file);
        return;
    }

    // Now, we only need the segments of the service. To speed up the lookup, we ignore files from other sessions.
    if (!_playlist.isValid() || (_service_session.isValid() && file.sessionId() != _service_session)) {
        return;
    }

    // Check if the file is a known segment of the service.
    if (!processSegment(file)) {
        // Not a known segment, check if this is a possible future segment.
        if (maybeFutureSegment(file.name())) {
            // Then enqueue it in a cache, to be processed in the next update of the playlist.
            _ahead_segment_cache.push_back(file);
            debug(u"cache file for next playlist: %s", file.name());
        }
    }
}


//----------------------------------------------------------------------------
// Check if a file is a known segment in the playlist.
//----------------------------------------------------------------------------

bool ts::mcast::NIPExtractPlugin::processSegment(const FluteFile& file)
{
    // Search the file in the playlist. Once the session id of the playlist is identified, if no segment is lost,
    // this is fast because the file should match the first segment.
    const size_t seg_count = _playlist.segmentCount();
    size_t seg_index = 0;
    while (seg_index < seg_count && file.name() != _playlist.segment(seg_index).urlString()) {
        seg_index++;
    }
    if (seg_index >= seg_count) {
        // Not a segment for the service.
        return false;
    }

    // Check that the segment contains TS packets. Check at most the first 10 TS packets.
    const ByteBlockPtr content(file.contentPointer());
    const size_t check_count = std::min<size_t>(10, content->size() / PKT_SIZE);
    for (size_t i = 0; i < check_count; ++i) {
        if ((*content)[i * PKT_SIZE] != SYNC_BYTE) {
            error(u"segment does not contain TS packets: %s", file.name());
            setError();
            return false;
        }
    }

    // Enqueue the segment file content.
    _output.push_back(content);
    debug(u"enqueue segment for output: %s", file.name());

    if (!_service_session.isValid()) {
        // Record the content session id for faster filtering of next files.
        _service_session = file.sessionId();
    }
    else if (seg_index > 0) {
        // Not the first segment of the playlist and not the beginning of the service transmission.
        warning(u"lost %d segments in service", seg_index);
        debug(u"first segment in playlist: %s", _playlist.segment(0).urlString());
    }

    // Drop used segments from the playlist.
    for (size_t i = 0; i <= seg_index; ++i) {
        debug(u"drop segment from playlist: %s", _playlist.segment(0).urlString());
        _playlist.popFirstSegment();
    }
    return true;
}


//----------------------------------------------------------------------------
// This method "guesses" if a file is maybe a future segment of the service.
//----------------------------------------------------------------------------

bool ts::mcast::NIPExtractPlugin::maybeFutureSegment(const UString& name)
{
    // Current algorithm: A file may be a segment if it has the same path and extension
    // as at least one segment of the playlist.
    if (_playlist.isMedia()) {

        // Locate path and extension in input file name.
        const size_t slash = name.rfind(u'/');
        const size_t dot = name.rfind(u'.');
        if (slash == NPOS || dot == NPOS) {
            // No directory or extension in the name.
            return false;
        }
        const UString path(name.substr(0, slash + 1));
        const UString ext(name.substr(dot));

        if (_playlist.segmentCount() > 0) {
            // Compare with all segments in playlist if not empty.
            for (size_t i = 0; i < _playlist.segmentCount(); ++i) {
                const UString seg_name(_playlist.segment(i).urlString());
                const size_t seg_slash = seg_name.rfind(u'/');
                const size_t seg_dot = seg_name.rfind(u'.');
                if (seg_slash != NPOS && seg_dot != NPOS) {
                    _last_segment_path = seg_name.substr(0, seg_slash + 1);
                    _last_segment_ext = seg_name.substr(seg_dot);
                    if (path == _last_segment_path && ext == _last_segment_ext) {
                        return true;
                    }
                }
            }
        }
        else if (path == _last_segment_path && ext == _last_segment_ext) {
            // Compare with last saved path and extension if the playlist is empty.
            return true;
        }
    }
    return false;
}
