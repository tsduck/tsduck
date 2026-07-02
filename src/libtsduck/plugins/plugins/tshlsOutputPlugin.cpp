//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsOutputPlugin.h"
#include "tsPluginRepository.h"
#include "tsOneShotPacketizer.h"
#include "tsErrCodeReport.h"
#include "tsPESPacket.h"
#include "tsPAT.h"
#include "tsPMT.h"

TS_REGISTER_OUTPUT_PLUGIN(u"hls", ts::hls::OutputPlugin);


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::hls::OutputPlugin::OutputPlugin(TSP* tsp_) :
    ts::OutputPlugin(tsp_, u"Generate HTTP Live Streaming (HLS) media", u"[options] filename"),
    _demux(duck, this),
    _cc_fixer(NoPID(), this)
{
    option(u"", 0, FILENAME, 1, 1);
    help(u"",
         u"Specify the name template of the output media segment files. "
         u"A number is automatically added to the name part so that successive segment "
         u"files receive distinct names. Example: if the specified file name is foo.ts, "
         u"the various segment files are named foo-000000.ts, foo-000001.ts, etc.\n\n"
         u"If the specified template already contains trailing digits, this unmodified "
         u"name is used for the first segment. Then, the integer part is incremented. "
         u"Example: if the specified file name is foo-027.ts, the various segment files "
         u"are named foo-027.ts, foo-028.ts, etc.");

    option(u"align-first-segment", 'a');
    help(u"align-first-segment",
         u"Force the first output segment to start with a PAT and PMT. "
         u"Also force the reference video PID to start on a PES packet boundary. "
         u"With --intra-close, also force this video PID to start on an intra-coded image (I-Frame). "
         u"By default, the first output segment starts with the first packets in the TS. "
         u"Using this option, all packets before all starting conditions are dropped. "
         u"Note that subsequent output segments always start with a copy of the last PAT and PMT.");

    option(u"custom-tag", 'c', STRING, 0, UNLIMITED_COUNT);
    help(u"custom-tag", u"'string'",
         u"Specify a custom tag to add in the playlist files. "
         u"The specified string shall start with '#'. If omitted, the leading '#' is automatically added. "
         u"Several --custom-tag can be specified. Each tag is added as an independent tag line.");

    option<cn::seconds>(u"duration", 'd');
    help(u"duration",
         u"Specify the target duration in seconds of media segments. "
         u"The default is " + UString::Chrono(DEFAULT_OUT_DURATION) + u" per segment for VoD streams "
         u"and " + UString::Chrono(DEFAULT_OUT_LIVE_DURATION) + u" for live streams.");

    option(u"event", 'e');
    help(u"event",
         u"Specify that the output is a event playlist. By default, the output stream is considered as VoD.");

    option(u"fixed-segment-size", 'f', POSITIVE);
    help(u"fixed-segment-size",
         u"Specify the size in bytes of all media segments. "
         u"By default, the segment size is variable and based on the --duration parameter. "
         u"When --fixed-segment-size is specified, the --duration parameter is only "
         u"used as a hint in the playlist file.");

    option(u"intra-close", 'i');
    help(u"intra-close",
         u"Start new segments on the start of an intra-coded image (I-Frame) of the reference video PID. "
         u"By default, a new segment starts on a PES packet boundary on this video PID. "
         u"Note that it is not always possible to guarantee this condition if the video coding format is not "
         u"fully supported, if the start of an intra-image cannot be found in the start of the PES packet "
         u"which is contained in a TS packet or if the TS packet is encrypted.");

    option(u"label-close", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"label-close", u"label1[-label2]",
         u"Close the current segment as soon as possible after a packet with any of the specified labels. "
         u"Labels should have typically been set by a previous plugin in the chain. "
         u"Several --label-close options may be specified.\n\n"
         u"In practice, the current segment is closed and renewed at the start of the next PES packet "
         u"on the video PID. This option is compatible with --duration. "
         u"The current segment is closed on a labelled packed or segment duration, whichever comes first.");

    option(u"live", 'l', POSITIVE);
    help(u"live",
         u"Specify that the output is a live stream. The specified value indicates the "
         u"number of simultaneously available media segments. Obsolete media segment files "
         u"are automatically deleted. By default, the output stream is considered as VoD "
         u"and all created media segments are preserved.");

    option(u"live-extra-segments", 0, UNSIGNED);
    help(u"live-extra-segments",
         u"In a live stream, specify the number of unreferenced segments to keep on disk before deleting them. "
         u"The extra segments were recently referenced in the playlist and can be downloaded by clients after their removal from the playlist. "
         u"The default is " + UString::Decimal(DEFAULT_LIVE_EXTRA_DEPTH) + u" segments.");

    option<cn::seconds>(u"max-extra-duration", 'm');
    help(u"max-extra-duration",
         u"With --intra-close, specify the maximum additional duration in seconds after which "
         u"the segment is closed on the next video PES packet, even if no intra-coded image is found. "
         u"The default is to wait a maximum of an additional " + UString::Chrono(DEFAULT_EXTRA_DURATION) + u" "
         u"for an intra-coded image.");

    option(u"no-bitrate");
    help(u"no-bitrate",
         u"With --playlist, do not specify EXT-X-BITRATE tags for each segment in the playlist. "
         u"This optional tag is present by default.");

    option(u"playlist", 'p', FILENAME);
    help(u"playlist", u"filename",
         u"Specify the name of the playlist file. "
         u"The playlist file is rewritten each time a new segment file is completed or an obsolete one is deleted. "
         u"The playlist and the segment files can be written to distinct directories but, in all cases, "
         u"the URI of the segment files in the playlist are always relative to the playlist location. "
         u"By default, no playlist file is created (media segments only).");

    option(u"slice-only");
    help(u"slice-only",
         u"Disable the insertion of the PAT and PMT at start of each segment. "
         u"Note that this generates a non-standard HLS output.");

    option(u"start-media-sequence", 's', POSITIVE);
    help(u"start-media-sequence",
         u"Initial media sequence number in #EXT-X-MEDIA-SEQUENCE directive in the playlist. "
         u"The default is zero.");
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::hls::OutputPlugin::isRealTime()
{
    return true;
}


//----------------------------------------------------------------------------
// Output command line options method
//----------------------------------------------------------------------------

bool ts::hls::OutputPlugin::getOptions()
{
    getPathValue(_segment_template, u"");
    getPathValue(_playlist_file, u"playlist");
    _intra_close = present(u"intra-close");
    _use_bitrate_tag = !present(u"no-bitrate");
    _align_first_segment = present(u"align-first-segment");
    _slice_only = present(u"slice-only");
    getIntValue(_live_depth, u"live");
    getIntValue(_live_extra_depth, u"live-extra-segments", DEFAULT_LIVE_EXTRA_DEPTH);
    getChronoValue(_target_duration, u"duration", _live_depth == 0 ? DEFAULT_OUT_DURATION : DEFAULT_OUT_LIVE_DURATION);
    getChronoValue(_max_extra_duration, u"max-extra-duration", DEFAULT_EXTRA_DURATION);
    _fixed_segment_size = intValue<PacketCounter>(u"fixed-segment-size") / PKT_SIZE;
    getIntValue(_initial_media_seq, u"start-media-sequence", 0);
    getIntValues(_close_labels, u"label-close");
    getValues(_custom_tags, u"custom-tag");

    if (present(u"event")) {
        _playlist_type = hls::PlayListType::EVENT;
        if (_live_depth > 0) {
            error(u"options --live and --event are incompatible");
            return false;
        }
    }
    else if (_live_depth > 0) {
        _playlist_type = hls::PlayListType::LIVE;
    }
    else {
        _playlist_type = hls::PlayListType::VOD;
    }

    if (_fixed_segment_size > 0 && _close_labels.any()) {
        error(u"options --fixed-segment-size and --label-close are incompatible");
        return false;
    }

    if (_slice_only && _align_first_segment) {
        error(u"options --slice-only and --align-first-segment are incompatible");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::hls::OutputPlugin::start()
{
    // Analyze the segment file name template to isolate segments.
    _name_generator.initCounter(_segment_template);

    // Initialize the demux to get the PAT and PMT.
    _demux.reset();
    _demux.setPIDFilter(NoPID());
    _demux.addPID(PID_PAT);
    _pat_packets.clear();
    _pmt_packets.clear();
    _pmt_pid = PID_NULL;
    _video_pid = PID_NULL;
    _video_stream_type = ST_NULL;
    _pcr_analyzer.reset();
    _previous_bitrate = 0;

    // Fix continuity counters in PAT PID. Will add the PMT PID when found.
    _cc_fixer.reset();
    _cc_fixer.setGenerator(true);
    _cc_fixer.setPIDFilter(NoPID());
    _cc_fixer.addPID(PID_PAT);

    // Initialize the segment and playlist files.
    _live_segment_files.clear();
    _seg_started = false;
    _seg_close_pending = false;
    if (_segment_file.isOpen()) {
        _segment_file.close(*this);
    }
    if (!_playlist_file.empty()) {
        _playlist.reset(_playlist_type, _playlist_file);
        _playlist.setTargetDuration(_target_duration, *this);
        _playlist.setMediaSequence(_initial_media_seq, *this);
    }
    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::hls::OutputPlugin::stop()
{
    // Simply close the current segment (and generate the corresponding playlist).
    return closeCurrentSegment(true);
}


//----------------------------------------------------------------------------
// Create the next segment file (also close the previous one if necessary).
//----------------------------------------------------------------------------

bool ts::hls::OutputPlugin::createNextSegment()
{
    // Close the previous segment file.
    if (!closeCurrentSegment(false)) {
        return false;
    }

    // Generate a new segment file name.
    const UString file_name(_name_generator.newFileName());

    // Create the segment file.
    verbose(u"creating media segment %s", file_name);
    if (!_segment_file.open(file_name, TSFile::WRITE | TSFile::SHARED, *this)) {
        return false;
    }

    // Reset the PCR analysis in each segment to get to bitrate of this segment.
    _pcr_analyzer.reset();

    // Reset the indication to close the segment file.
    _seg_close_pending = false;

    // Add a copy of the PAT and PMT at the beginning of each segment.
    if (!_slice_only) {
        return writePackets(_pat_packets.data(), _pat_packets.size()) && writePackets(_pmt_packets.data(), _pmt_packets.size());
    }

    return true;
}


//----------------------------------------------------------------------------
// Close current segment file.
// Also purge obsolete segment files and regenerate playlist.
//----------------------------------------------------------------------------

bool ts::hls::OutputPlugin::closeCurrentSegment(bool endOfStream)
{
    // If no segment file is open, there is nothing to do.
    if (!_segment_file.isOpen()) {
        return true;
    }

    // Get the segment file name and size (to be inserted in the playlist).
    const UString seg_name(_segment_file.getFileName());
    const PacketCounter seg_packets = _segment_file.writePacketsCount();

    // Close the TS file.
    if (!_segment_file.close(*this)) {
        return false;
    }

    // On live streams, we need to maintain a list of active segments.
    if (_live_depth > 0) {
        _live_segment_files.push_back(seg_name);
    }

    // Create or regenerate the playlist file.
    if (!_playlist_file.empty()) {

        // Set end of stream indicator in the playlist.
        _playlist.setEndList(endOfStream, *this);

        // Declare a new segment.
        hls::MediaSegment seg;
        _playlist.buildURL(seg, seg_name);

        // Estimate duration and bitrate of the segment. We use PCR's from the
        // segment to compute the average bitrate. Then we compute the duration
        // from the bitrate and segment file size. If we cannot get the bitrate
        // of a segment but got one from previous segment, assume that bitrate
        // did not change and reuse previous one.
        if (_pcr_analyzer.bitrateIsValid()) {
            // We have an estimation of the bitrate of the segment file.
            _previous_bitrate = _pcr_analyzer.bitrate188();
        }
        if (_previous_bitrate > 0) {
            // Compute duration based on segment bitrate (or previous one).
            seg.bitrate = _use_bitrate_tag ? _previous_bitrate : 0;
            seg.duration = PacketInterval(_previous_bitrate, seg_packets);
        }
        else {
            // Completely unknown bitrate, we build a fake one based on the target duration.
            seg.duration = cn::duration_cast<cn::milliseconds>(_target_duration);
            seg.bitrate = _use_bitrate_tag ? PacketBitRate(seg_packets, seg.duration) : 0;
        }
        _playlist.addSegment(seg, *this);

        // With live playlists, remove obsolete segments from the playlist.
        while (_live_depth > 0 && _playlist.segmentCount() > _live_depth) {
            _playlist.popFirstSegment();
        }

        // Add custom tags.
        _playlist.clearCustomTags();
        for (const auto& tag : _custom_tags) {
            _playlist.addCustomTag(tag);
        }

        // Use #EXT-X-INDEPENDENT-SEGMENTS if all segments are really independent.
        if (!_slice_only) {
            _playlist.addCustomTag(u"EXT-X-INDEPENDENT-SEGMENTS");
        }

        // Write the playlist file.
        if (!_playlist.saveFile(UString(), *this)) {
            return false;
        }

        // WARNING: suggested improvement:
        //   On Windows, if we overwrite the playlist file while a client is downloading it,
        //   the file is locked by the HTTP server and the replacement will fail. We should
        //   keep a list of failed deletions to retry these deletions later. On Unix systems,
        //   we should not have the problem since the deletion succeeds even if the file
        //   is already open (the file actually disappears when the file is closed).
    }

    // Keep a list of segments we fail to delete (maybe because they are locked by the Web server).
    UStringList failed_delete;

    // On live streams, purge obsolete segment files.
    while (_live_depth > 0 && _live_segment_files.size() > _live_depth + _live_extra_depth) {

        // Remove name of the file to delete from the list of active segment.
        const UString name(_live_segment_files.front());
        _live_segment_files.pop_front();

        // Delete the segment file.
        verbose(u"deleting obsolete segment file %s", name);
        if (!fs::remove(name, &ErrCodeReport(*this, u"error deleting", name)) && fs::exists(name)) {
            // Failed to delete, keep it to retry later.
            failed_delete.push_back(name);
        }
    }

    // Re-insert segments we failed to delete at head of list so that we will retry to delete them next time.
    if (!failed_delete.empty()) {
        _live_segment_files.insert(_live_segment_files.begin(), failed_delete.begin(), failed_delete.end());
    }

    return true;
}


//----------------------------------------------------------------------------
// Implementation of TableHandlerInterface.
//----------------------------------------------------------------------------

void ts::hls::OutputPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    // We need to collect the PAT and the (first) PMT.
    TSPacketVector* packets = nullptr;

    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(duck, table);
            if (pat.isValid()) {
                packets = &_pat_packets;
                // Get the PMT of the first service.
                if (!pat.pmts.empty()) {
                    const uint16_t srv(pat.pmts.begin()->first);
                    _pmt_pid = pat.pmts.begin()->second;
                    _demux.addPID(_pmt_pid);
                    _cc_fixer.addPID(_pmt_pid);
                    verbose(u"using service id %n as reference, PMT PID %n", srv, _pmt_pid);
                }
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(duck, table);
            if (pmt.isValid()) {
                packets = &_pmt_packets;
                _video_pid = pmt.firstVideoPID(duck);
                if (_video_pid == PID_NULL) {
                    warning(u"no video PID found in service %n", pmt.service_id);
                }
                else {
                    _video_stream_type = pmt.streams[_video_pid].stream_type;
                    verbose(u"using video PID %n as reference", _video_pid);
                }
            }
            break;
        }
        default: {
            // Unexpected table.
            break;
        }
    }

    // If we need to packetize the table, do it now.
    if (packets != nullptr) {
        OneShotPacketizer pzer(duck, table.sourcePID());
        pzer.addTable(table);
        pzer.getPackets(*packets);
    }
}


//----------------------------------------------------------------------------
// Write packets into the current segment file, adjust CC in PAT and PMT PID.
//----------------------------------------------------------------------------

bool ts::hls::OutputPlugin::writePackets(const TSPacket* pkt, size_t packetCount)
{
    // Temporary packet buffer if a packet needs to be modified.
    TSPacket tmp;

    // Loop on all packets.
    for (size_t i = 0; i < packetCount; ++i) {

        // Address of the next packet to write.
        const TSPacket* p = pkt + i;

        // If the packet comes from the PAT or PMT, get a copy and fix continuity counter.
        if (!_slice_only) {
            const PID pid = pkt[i].getPID();
            if (pid == PID_PAT) {
                tmp = *p;
                _cc_fixer.feedPacket(tmp);
                p = &tmp;
            }
            else if (_pmt_pid != PID_NULL && pid == _pmt_pid) {
                tmp = *p;
                _cc_fixer.feedPacket(tmp);
                p = &tmp;
            }
        }

        // Write the packet in the segment file.
        if (!_segment_file.writePackets(p, nullptr, 1, *this)) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::hls::OutputPlugin::send(const TSPacket* pkt, const TSPacketMetadata* pktData, size_t packetCount)
{
    const TSPacket* const last_pkt = pkt + packetCount;
    bool ok = true;

    // Process packets one by one.
    while (ok && pkt < last_pkt) {

        // Pass all packets into the demux.
        if (!_slice_only) {
            _demux.feedPacket(*pkt);
        }

        // Analyze PCR's from all packets.
        _pcr_analyzer.feedPacket(*pkt);

        // Check if we can start the generation of output segments.
        if (!_seg_started) {
            if (!_align_first_segment) {
                // Without --align-first-segment, always start immediately.
                _seg_started = true;
            }
            else if (!_pat_packets.empty() && !_pmt_packets.empty() && _video_pid != PID_NULL && pkt->getPID() == _video_pid && pkt->getPUSI()) {
                // With --align-first-segment, need at least a PAT, PMT, PES packet on video PID.
                // When --intra-close is also specified, start on intra image.
                _seg_started = !_intra_close || (pkt->isClear() && PESPacket::FindIntraImage(pkt->getPayload(), pkt->getPayloadSize(), _video_stream_type) != NPOS);
            }
            if (_seg_started) {
                // Create the first segment file.
                ok = createNextSegment();
            }
        }

        // Process output packet only when the generation of segments is started.
        if (ok && _seg_started) {

            // Check if we should close the current segment and create a new one.
            bool renewNow = false;
            bool renewOnPUSI = false;
            if (_fixed_segment_size > 0) {
                // Each segment shall have a fixed size.
                renewNow = _segment_file.writePacketsCount() >= _fixed_segment_size;
            }
            else if (!_seg_close_pending) {
                if (pktData->hasAnyLabel(_close_labels)) {
                    // This packet is a trigger to close the segment as soon as possible.
                    _seg_close_pending = true;
                }
                else if (_pcr_analyzer.bitrateIsValid()) {
                    // The segment file shall be closed when the estimated duration exceeds the target duration.
                    const cn::milliseconds segDuration = PacketInterval(_pcr_analyzer.bitrate188(), _segment_file.writePacketsCount());
                    _seg_close_pending = segDuration >= _target_duration;
                    // With --intra-close, force renew on next PES packet if extra duration is exceeded.
                    renewOnPUSI = segDuration >= _target_duration + _max_extra_duration;
                }
            }

            // We close only when we start a new PES packet or new intra-image on the video PID.
            if (_seg_close_pending) {
                if (_video_pid == PID_NULL) {
                    debug(u"closing segment, no video PID was identified for synchronization");
                    renewNow = true;
                }
                else if (pkt->getPID() == _video_pid && pkt->getPUSI()) {
                    // On a new video PES packet.
                    if (!_intra_close) {
                        debug(u"starting new segment on new PES packet");
                        renewNow = true;
                    }
                    else if (renewOnPUSI) {
                        debug(u"no I-frame found in last %s, starting new segment on new PES packet", _max_extra_duration);
                        renewNow = true;
                    }
                    else if (pkt->isClear() && PESPacket::FindIntraImage(pkt->getPayload(), pkt->getPayloadSize(), _video_stream_type) != NPOS) {
                        debug(u"starting new segment on new I-frame");
                        renewNow = true;
                    }
                }
            }

            // Close current segment and recreate a new one when necessary.
            // Finally write the packet.
            ok = (!renewNow || createNextSegment()) && writePackets(pkt, 1);
        }

        // Process next packet.
        ++pkt;
        ++pktData;
    }
    return ok;
}
