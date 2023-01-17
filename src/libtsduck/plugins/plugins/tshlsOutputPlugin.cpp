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

#include "tshlsOutputPlugin.h"
#include "tsPluginRepository.h"
#include "tsOneShotPacketizer.h"
#include "tsFileUtils.h"
#include "tsPESPacket.h"
#include "tsPAT.h"
#include "tsPMT.h"

TS_REGISTER_OUTPUT_PLUGIN(u"hls", ts::hls::OutputPlugin);

#define DEFAULT_OUT_DURATION      10  // Default segment target duration for output streams.
#define DEFAULT_OUT_LIVE_DURATION  5  // Default segment target duration for output live streams.
#define DEFAULT_EXTRA_DURATION     2  // Default segment extra duration when intra image is not found.
#define DEFAULT_LIVE_EXTRA_DEPTH   1  // Default additional segments to keep in live streams.


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::hls::OutputPlugin::OutputPlugin(TSP* tsp_) :
    ts::OutputPlugin(tsp_, u"Generate HTTP Live Streaming (HLS) media", u"[options] filename"),
    _segmentTemplate(),
    _playlistFile(),
    _intraClose(false),
    _useBitrateTag(false),
    _alignFirstSegment(false),
    _sliceOnly(false),
    _playlistType(hls::PlayListType::UNKNOWN),
    _liveDepth(0),
    _liveExtraDepth(0),
    _targetDuration(0),
    _maxExtraDuration(0),
    _fixedSegmentSize(0),
    _initialMediaSeq(0),
    _customTags(),
    _closeLabels(),
    _nameGenerator(),
    _demux(duck, this),
    _patPackets(),
    _pmtPackets(),
    _pmtPID(PID_NULL),
    _videoPID(PID_NULL),
    _videoStreamType(ST_NULL),
    _segStarted(false),
    _segClosePending(false),
    _segmentFile(),
    _liveSegmentFiles(),
    _playlist(),
    _pcrAnalyzer(1, 4),  // Minimum required: 1 PID, 4 PCR
    _previousBitrate(0),
    _ccFixer(NoPID, tsp)
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

    option(u"duration", 'd', POSITIVE);
    help(u"duration",
         u"Specify the target duration in seconds of media segments. "
         u"The default is " TS_STRINGIFY(DEFAULT_OUT_DURATION) u" seconds per segment for VoD streams "
         u"and " TS_STRINGIFY(DEFAULT_OUT_LIVE_DURATION) u" seconds for live streams.");

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
         u"The default is " TS_STRINGIFY(DEFAULT_LIVE_EXTRA_DEPTH) u" segments.");

    option(u"max-extra-duration", 'm', POSITIVE);
    help(u"max-extra-duration",
         u"With --intra-close, specify the maximum additional duration in seconds after which "
         u"the segment is closed on the next video PES packet, even if no intra-coded image is found. "
         u"The default is to wait a maximum of " TS_STRINGIFY(DEFAULT_EXTRA_DURATION) u" additional seconds "
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
    getValue(_segmentTemplate, u"");
    getValue(_playlistFile, u"playlist");
    _intraClose = present(u"intra-close");
    _useBitrateTag = !present(u"no-bitrate");
    _alignFirstSegment = present(u"align-first-segment");
    _sliceOnly = present(u"slice-only");
    getIntValue(_liveDepth, u"live");
    getIntValue(_liveExtraDepth, u"live-extra-segments", DEFAULT_LIVE_EXTRA_DEPTH);
    getIntValue(_targetDuration, u"duration", _liveDepth == 0 ? DEFAULT_OUT_DURATION : DEFAULT_OUT_LIVE_DURATION);
    getIntValue(_maxExtraDuration, u"max-extra-duration", DEFAULT_EXTRA_DURATION);
    _fixedSegmentSize = intValue<PacketCounter>(u"fixed-segment-size") / PKT_SIZE;
    getIntValue(_initialMediaSeq, u"start-media-sequence", 0);
    getIntValues(_closeLabels, u"label-close");
    getValues(_customTags, u"custom-tag");

    if (present(u"event")) {
        _playlistType = hls::PlayListType::EVENT;
        if (_liveDepth > 0) {
            tsp->error(u"options --live and --event are incompatible");
            return false;
        }
    }
    else if (_liveDepth > 0) {
        _playlistType = hls::PlayListType::LIVE;
    }
    else {
        _playlistType = hls::PlayListType::VOD;
    }

    if (_fixedSegmentSize > 0 && _closeLabels.any()) {
        tsp->error(u"options --fixed-segment-size and --label-close are incompatible");
        return false;
    }

    if (_sliceOnly && _alignFirstSegment) {
        tsp->error(u"options --slice-only and --align-first-segment are incompatible");
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
    _nameGenerator.initCounter(_segmentTemplate);

    // Initialize the demux to get the PAT and PMT.
    _demux.reset();
    _demux.setPIDFilter(NoPID);
    _demux.addPID(PID_PAT);
    _patPackets.clear();
    _pmtPackets.clear();
    _pmtPID = PID_NULL;
    _videoPID = PID_NULL;
    _videoStreamType = ST_NULL;
    _pcrAnalyzer.reset();
    _previousBitrate = 0;

    // Fix continuity counters in PAT PID. Will add the PMT PID when found.
    _ccFixer.reset();
    _ccFixer.setGenerator(true);
    _ccFixer.setPIDFilter(NoPID);
    _ccFixer.addPID(PID_PAT);

    // Initialize the segment and playlist files.
    _liveSegmentFiles.clear();
    _segStarted = false;
    _segClosePending = false;
    if (_segmentFile.isOpen()) {
        _segmentFile.close(*tsp);
    }
    if (!_playlistFile.empty()) {
        _playlist.reset(_playlistType, _playlistFile);
        _playlist.setTargetDuration(_targetDuration, *tsp);
        _playlist.setMediaSequence(_initialMediaSeq, *tsp);
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
    const UString fileName(_nameGenerator.newFileName());

    // Create the segment file.
    tsp->verbose(u"creating media segment %s", {fileName});
    if (!_segmentFile.open(fileName, TSFile::WRITE | TSFile::SHARED, *tsp)) {
        return false;
    }

    // Reset the PCR analysis in each segment to get to bitrate of this segment.
    _pcrAnalyzer.reset();

    // Reset the indication to close the segment file.
    _segClosePending = false;

    // Add a copy of the PAT and PMT at the beginning of each segment.
    if (!_sliceOnly) {
        return writePackets(_patPackets.data(), _patPackets.size()) && writePackets(_pmtPackets.data(), _pmtPackets.size());
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
    if (!_segmentFile.isOpen()) {
        return true;
    }

    // Get the segment file name and size (to be inserted in the playlist).
    const UString segName(_segmentFile.getFileName());
    const PacketCounter segPackets = _segmentFile.writePacketsCount();

    // Close the TS file.
    if (!_segmentFile.close(*tsp)) {
        return false;
    }

    // On live streams, we need to maintain a list of active segments.
    if (_liveDepth > 0) {
        _liveSegmentFiles.push_back(segName);
    }

    // Create or regenerate the playlist file.
    if (!_playlistFile.empty()) {

        // Set end of stream indicator in the playlist.
        _playlist.setEndList(endOfStream, *tsp);

        // Declare a new segment.
        hls::MediaSegment seg;
        _playlist.buildURL(seg, segName);

        // Estimate duration and bitrate of the segment. We use PCR's from the
        // segment to compute the average bitrate. Then we compute the duration
        // from the bitrate and segment file size. If we cannot get the bitrate
        // of a segment but got one from previous segment, assume that bitrate
        // did not change and reuse previous one.
        if (_pcrAnalyzer.bitrateIsValid()) {
            // We have an estimation of the bitrate of the segment file.
            _previousBitrate = _pcrAnalyzer.bitrate188();
        }
        if (_previousBitrate > 0) {
            // Compute duration based on segment bitrate (or previous one).
            seg.bitrate = _useBitrateTag ? _previousBitrate : 0;
            seg.duration = PacketInterval(_previousBitrate, segPackets);
        }
        else {
            // Completely unknown bitrate, we build a fake one based on the target duration.
            seg.duration = _targetDuration * MilliSecPerSec;
            seg.bitrate = _useBitrateTag ? PacketBitRate(segPackets, seg.duration) : 0;
        }
        _playlist.addSegment(seg, *tsp);

        // With live playlists, remove obsolete segments from the playlist.
        while (_liveDepth > 0 && _playlist.segmentCount() > _liveDepth) {
            _playlist.popFirstSegment();
        }

        // Add custom tags.
        _playlist.clearCustomTags();
        for (const auto& tag : _customTags) {
            _playlist.addCustomTag(tag);
        }

        // Use #EXT-X-INDEPENDENT-SEGMENTS if all segments are really independent.
        if (!_sliceOnly) {
            _playlist.addCustomTag(u"EXT-X-INDEPENDENT-SEGMENTS");
        }

        // Write the playlist file.
        if (!_playlist.saveFile(UString(), *tsp)) {
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
    UStringList failedDelete;

    // On live streams, purge obsolete segment files.
    while (_liveDepth > 0 && _liveSegmentFiles.size() > _liveDepth + _liveExtraDepth) {

        // Remove name of the file to delete from the list of active segment.
        const UString name(_liveSegmentFiles.front());
        _liveSegmentFiles.pop_front();

        // Delete the segment file.
        tsp->verbose(u"deleting obsolete segment file %s", {name});
        if (!DeleteFile(name, *tsp) && FileExists(name)) {
            // Failed to delete, keep it to retry later.
            failedDelete.push_back(name);
        }
    }

    // Re-insert segments we failed to delete at head of list so that we will retry to delete them next time.
    if (!failedDelete.empty()) {
        _liveSegmentFiles.insert(_liveSegmentFiles.begin(), failedDelete.begin(), failedDelete.end());
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
                packets = &_patPackets;
                // Get the PMT of the first service.
                if (!pat.pmts.empty()) {
                    const uint16_t srv(pat.pmts.begin()->first);
                    _pmtPID = pat.pmts.begin()->second;
                    _demux.addPID(_pmtPID);
                    _ccFixer.addPID(_pmtPID);
                    tsp->verbose(u"using service id 0x%X (%d) as reference, PMT PID 0x%X (%d)", {srv, srv, _pmtPID, _pmtPID});
                }
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(duck, table);
            if (pmt.isValid()) {
                packets = &_pmtPackets;
                _videoPID = pmt.firstVideoPID(duck);
                if (_videoPID == PID_NULL) {
                    tsp->warning(u"no video PID found in service 0x%X (%d)", {pmt.service_id, pmt.service_id});
                }
                else {
                    _videoStreamType = pmt.streams[_videoPID].stream_type;
                    tsp->verbose(u"using video PID 0x%X (%d) as reference", {_videoPID, _videoPID});
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
        if (!_sliceOnly) {
            const PID pid = pkt[i].getPID();
            if (pid == PID_PAT) {
                tmp = *p;
                _ccFixer.feedPacket(tmp);
                p = &tmp;
            }
            else if (_pmtPID != PID_NULL && pid == _pmtPID) {
                tmp = *p;
                _ccFixer.feedPacket(tmp);
                p = &tmp;
            }
        }

        // Write the packet in the segment file.
        if (!_segmentFile.writePackets(p, nullptr, 1, *tsp)) {
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
    const TSPacket* const lastPkt = pkt + packetCount;
    bool ok = true;

    // Process packets one by one.
    while (ok && pkt < lastPkt) {

        // Pass all packets into the demux.
        if (!_sliceOnly) {
            _demux.feedPacket(*pkt);
        }

        // Analyze PCR's from all packets.
        _pcrAnalyzer.feedPacket(*pkt);

        // Check if we can start the generation of output segments.
        if (!_segStarted) {
            if (!_alignFirstSegment) {
                // Without --align-first-segment, always start immediately.
                _segStarted = true;
            }
            else if (!_patPackets.empty() && !_pmtPackets.empty() && _videoPID != PID_NULL && pkt->getPID() == _videoPID && pkt->getPUSI()) {
                // With --align-first-segment, need at least a PAT, PMT, PES packet on video PID.
                // When --intra-close is also specified, start on intra image.
                _segStarted = !_intraClose || (pkt->isClear() && PESPacket::FindIntraImage(pkt->getPayload(), pkt->getPayloadSize(), _videoStreamType) != NPOS);
            }
            if (_segStarted) {
                // Create the first segment file.
                ok = createNextSegment();
            }
        }

        // Process output packet only when the generation of segments is started.
        if (ok && _segStarted) {

            // Check if we should close the current segment and create a new one.
            bool renewNow = false;
            bool renewOnPUSI = false;
            if (_fixedSegmentSize > 0) {
                // Each segment shall have a fixed size.
                renewNow = _segmentFile.writePacketsCount() >= _fixedSegmentSize;
            }
            else if (!_segClosePending) {
                if (pktData->hasAnyLabel(_closeLabels)) {
                    // This packet is a trigger to close the segment as soon as possible.
                    _segClosePending = true;
                }
                else if (_pcrAnalyzer.bitrateIsValid()) {
                    // The segment file shall be closed when the estimated duration exceeds the target duration.
                    const MilliSecond segDuration = PacketInterval(_pcrAnalyzer.bitrate188(), _segmentFile.writePacketsCount());
                    _segClosePending = segDuration >= _targetDuration * MilliSecPerSec;
                    // With --intra-close, force renew on next PES packet if extra duration is exceeded.
                    renewOnPUSI = segDuration >= (_targetDuration + _maxExtraDuration) * MilliSecPerSec;
                }
            }

            // We close only when we start a new PES packet or new intra-image on the video PID.
            if (_segClosePending) {
                if (_videoPID == PID_NULL) {
                    tsp->debug(u"closing segment, no video PID was identified for synchronization");
                    renewNow = true;
                }
                else if (pkt->getPID() == _videoPID && pkt->getPUSI()) {
                    // On a new video PES packet.
                    if (!_intraClose) {
                        tsp->debug(u"starting new segment on new PES packet");
                        renewNow = true;
                    }
                    else if (renewOnPUSI) {
                        tsp->debug(u"no I-frame found in last %d seconds, starting new segment on new PES packet", {_maxExtraDuration});
                        renewNow = true;
                    }
                    else if (pkt->isClear() && PESPacket::FindIntraImage(pkt->getPayload(), pkt->getPayloadSize(), _videoStreamType) != NPOS) {
                        tsp->debug(u"starting new segment on new I-frame");
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
