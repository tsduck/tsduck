//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

TS_REGISTER_OUTPUT_PLUGIN(u"hls", ts::hls::OutputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::hls::OutputPlugin::REFERENCE = 0;

#define DEFAULT_OUT_DURATION      10  // Default segment target duration for output streams.
#define DEFAULT_OUT_LIVE_DURATION  5  // Default segment target duration for output live streams.
#define DEFAULT_OUT_NUM_WIDTH      6  // Default size of number field in output segment files.


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::hls::OutputPlugin::OutputPlugin(TSP* tsp_) :
    ts::OutputPlugin(tsp_, u"Generate HTTP Live Streaming (HLS) media", u"[options] filename"),
    _segmentTemplate(),
    _segmentTemplateHead(),
    _segmentTemplateTail(),
    _segmentNumWidth(DEFAULT_OUT_NUM_WIDTH),
    _segmentNextFile(0),
    _playlistFile(),
    _fixedSegmentSize(0),
    _targetDuration(0),
    _liveDepth(0),
    _initialMediaSeq(0),
    _demux(duck, this),
    _patPackets(),
    _pmtPackets(),
    _videoPID(PID_NULL),
    _pmtPID(PID_NULL),
    _segClosePending(false),
    _segmentFile(),
    _liveSegmentFiles(),
    _playlist(),
    _pcrAnalyzer(1, 4),  // Minimum required: 1 PID, 4 PCR
    _previousBitrate(0),
    _ccFixer(NoPID, tsp),
    _close_labels()
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
         u"The default is " TS_STRINGIFY(DEFAULT_OUT_DURATION) u" seconds per segment for VoD streams "
         u"and " TS_STRINGIFY(DEFAULT_OUT_LIVE_DURATION) u" seconds for live streams.");

    option(u"fixed-segment-size", 'f', POSITIVE);
    help(u"fixed-segment-size",
         u"Specify the size in bytes of all media segments. "
         u"By default, the segment size is variable and based on the --duration parameter. "
         u"When --fixed-segment-size is specified, the --duration parameter is only "
         u"used as a hint in the playlist file.");

    option(u"label-close", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"label-close", u"label1[-label2]",
         u"Close the current segment as soon as possible after a packet with any of the specified labels. "
         u"Labels should have typically been set by a previous plugin in the chain. "
         u"Several --label-close options may be specified.\n\n"
         u"In practice, the current segment is closed and renewed at the start of the next PES packet "
         u"on the video PID. This option is compatible with --duration. "
         u"The current segment is closed on a labelled packed or segment duration, "
         u"whichever comes first.");

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
    _liveDepth = intValue<size_t>(u"live");
    _targetDuration = intValue<Second>(u"duration", _liveDepth == 0 ? DEFAULT_OUT_DURATION : DEFAULT_OUT_LIVE_DURATION);
    _fixedSegmentSize = intValue<PacketCounter>(u"fixed-segment-size") / PKT_SIZE;
    _initialMediaSeq = intValue<size_t>(u"start-media-sequence", 0);
    getIntValues(_close_labels, u"label-close");

    if (_fixedSegmentSize > 0 && _close_labels.any()) {
        tsp->error(u"options --fixed-segment-size and --label-close are incompatible");
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
    _segmentTemplateHead = PathPrefix(_segmentTemplate);
    _segmentTemplateTail = PathSuffix(_segmentTemplate);

    // Compute number of existing digits at end of template head.
    const size_t len = _segmentTemplateHead.length();
    _segmentNumWidth = 0;
    while (_segmentNumWidth < len && IsDigit(_segmentTemplateHead[len - 1 - _segmentNumWidth])) {
        _segmentNumWidth++;
    }
    if (_segmentNumWidth == 0) {
        // No pre-existing integer field at end of file name. Use defaults.
        _segmentNumWidth = DEFAULT_OUT_NUM_WIDTH;
        _segmentNextFile = 0;
    }
    else {
        // Use existing integer field as initial value.
        _segmentTemplateHead.substr(len - _segmentNumWidth).toInteger(_segmentNextFile);
        _segmentTemplateHead.erase(len - _segmentNumWidth);
    }

    // Initialize the demux to get the PAT and PMT.
    _demux.reset();
    _demux.setPIDFilter(NoPID);
    _demux.addPID(PID_PAT);
    _patPackets.clear();
    _pmtPackets.clear();
    _pmtPID = PID_NULL;
    _videoPID = PID_NULL;
    _pcrAnalyzer.reset();
    _previousBitrate = 0;

    // Fix continuity counters in PAT PID. Will add the PMT PID when found.
    _ccFixer.reset();
    _ccFixer.setGenerator(true);
    _ccFixer.setPIDFilter(NoPID);
    _ccFixer.addPID(PID_PAT);

    // Initialize the segment and playlist files.
    _liveSegmentFiles.clear();
    _segClosePending = false;
    if (_segmentFile.isOpen()) {
        _segmentFile.close(*tsp);
    }
    if (!_playlistFile.empty()) {
        _playlist.reset(hls::MEDIA_PLAYLIST, _playlistFile);
        _playlist.setTargetDuration(_targetDuration, *tsp);
        _playlist.setPlaylistType(_liveDepth == 0 ? u"VOD" : u"EVENT", *tsp);
        _playlist.setMediaSequence(_initialMediaSeq, *tsp);
    }

    // Create the first segment file.
    return createNextSegment();
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::hls::OutputPlugin::stop()
{
    // Simply close the current segmetn (and generate the corresponding playlist).
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
    const UString fileName(UString::Format(u"%s%0*d%s", {_segmentTemplateHead, _segmentNumWidth, _segmentNextFile, _segmentTemplateTail}));

    // Create the segment file.
    tsp->verbose(u"creating media segment %s", {fileName});
    if (!_segmentFile.open(fileName, TSFile::WRITE | TSFile::SHARED, *tsp)) {
        return false;
    }

    // Increment index for next segment name.
    _segmentNextFile++;

    // Reset the PCR analysis in each segment to get to bitrate of this segment.
    _pcrAnalyzer.reset();

    // Reset the indication to close the segment file.
    _segClosePending = false;

    // Add a copy of the PAT and PMT at the beginning of each segment.
    return writePackets(_patPackets.data(), _patPackets.size()) && writePackets(_pmtPackets.data(), _pmtPackets.size());
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
        seg.uri = segName;

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
            seg.bitrate = _previousBitrate;
            seg.duration = PacketInterval(seg.bitrate, segPackets);
        }
        else {
            // Completely unknown bitrate, we build a fake one based on the target duration.
            seg.duration = _targetDuration * MilliSecPerSec;
            seg.bitrate = PacketBitRate(segPackets, seg.duration);
        }
        _playlist.addSegment(seg, *tsp);

        // With live playlists, remove obsolete segments from the playlist.
        while (_liveDepth > 0 && _playlist.segmentCount() > _liveDepth) {
            _playlist.popFirstSegment(seg);
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

    // On live streams, purge obsolete segment files.
    while (_liveDepth > 0 && _liveSegmentFiles.size() > _liveDepth) {

        // Remove name of the file to delete from the list of active segment.
        const UString name(_liveSegmentFiles.front());
        _liveSegmentFiles.pop_front();

        // Delete the segment file.
        tsp->verbose(u"deleting obsolete segment file %s", {name});
        if (DeleteFile(name) != SYS_SUCCESS) {
            tsp->verbose(u"error deleting obsolete segment file %s", {name});
        }

        // WARNING: several improvements are possible here.
        // - It could be better to delay the purge of obsolete segments. Clients may have loaded
        //   the previous playlist just before we modified it and could try to download the
        //   obsolete segment.
        // - On Windows, if we try to delete the file while a client is downloading it, the
        //   segment file is locked by the HTTP server and the deletion will fail. We should
        //   keep a list of failed deletions to retry these deletions later. On Unix systems,
        //   we should not have the problem since the deletion succeeds even if the file
        //   is already open (the file actually disappears when the file is closed).
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
                _videoPID = pmt.firstVideoPID();
                if (_videoPID == PID_NULL) {
                    tsp->warning(u"no video PID found in service 0x%X (%d)", {pmt.service_id, pmt.service_id});
                }
                else {
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

bool ts::hls::OutputPlugin::send(const TSPacket* pkt, const TSPacketMetadata* pkt_data, size_t packetCount)
{
    bool ok = true;

    // Process packets one by one.
    for (size_t i = 0; ok && i < packetCount; ++i) {

        // Pass all packets into the demux.
        _demux.feedPacket(pkt[i]);

        // Analyze PCR's from all packets.
        _pcrAnalyzer.feedPacket(pkt[i]);

        // Check if we should close the current segment and create a new one.
        bool renew = false;
        if (_fixedSegmentSize > 0) {
            // Each segment shall have a fixed size.
            renew = _segmentFile.writePacketsCount() >= _fixedSegmentSize;
        }
        else if (!_segClosePending) {
            if (pkt_data[i].hasAnyLabel(_close_labels)) {
                // This packet is a trigger to close the segment as soon as possible.
                _segClosePending = true;
            }
            else if (_pcrAnalyzer.bitrateIsValid()) {
                // The segment file shall be closed when the estimated duration exceeds the target duration.
                _segClosePending = PacketInterval(_pcrAnalyzer.bitrate188(), _segmentFile.writePacketsCount()) >= _targetDuration * MilliSecPerSec;
            }
        }

        // We do close only when we start a new PES packet on the video PID.
        renew = renew || (_segClosePending && (_videoPID == PID_NULL || (pkt[i].getPID() == _videoPID && pkt[i].getPUSI())));

        // Close current segment and recreate a new one when necessary.
        // Finally write the packet.
        ok = (!renew || createNextSegment()) && writePackets(pkt + i, 1);
    }
    return ok;
}
