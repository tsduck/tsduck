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
//!
//!  @file
//!  HLS output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsSectionDemux.h"
#include "tsTSFile.h"
#include "tsPCRAnalyzer.h"
#include "tsContinuityAnalyzer.h"
#include "tsFileNameGenerator.h"
#include "tshlsPlayList.h"

namespace ts {
    namespace hls {
        //!
        //! HTTP Live Streaming (HLS) output plugin for tsp.
        //! @ingroup plugin
        //!
        //! The output plugin generates playlists and media segments on local files
        //! only. It can also purge obsolete media segments and regenerate live
        //! playlists. To setup a complete HLS server, it is necessary to setup an
        //! external HTTP server such as Apache which simply serves these files.
        //!
        class TSDUCKDLL OutputPlugin: public ts::OutputPlugin, private TableHandlerInterface
        {
            TS_NOBUILD_NOCOPY(OutputPlugin);
        public:
            //!
            //! Constructor.
            //! @param [in] tsp Associated callback to @c tsp executable.
            //!
            OutputPlugin(TSP* tsp);

            // Implementation of plugin API
            virtual bool getOptions() override;
            virtual bool start() override;
            virtual bool stop() override;
            virtual bool isRealTime() override;
            virtual bool send(const TSPacket*, const TSPacketMetadata* pkt_data, size_t) override;

        private:
            // Command line options.
            UString            _segmentTemplate;       // Command line segment file names template.
            UString            _playlistFile;          // Playlist file name.
            bool               _intraClose;            // Try to start segments on intra images.
            bool               _useBitrateTag;         // Specify EXT-X-BITRATE tags for each segment in the playlist.
            bool               _alignFirstSegment;     // Align first segment to the first PAT and PMT.
            bool               _sliceOnly;             // Don't add PAT and PMT to the segments.
            hls::PlayListType  _playlistType;          // Type of playlist.
            size_t             _liveDepth;             // Number of simultaneous segments in live streams.
            size_t             _liveExtraDepth;        // Number of additional segments to keep in live streams.
            Second             _targetDuration;        // Segment target duration in seconds.
            Second             _maxExtraDuration;      // Segment target max extra duration in seconds when intra image is not found.
            PacketCounter      _fixedSegmentSize;      // Optional fixed segment size in packets.
            size_t             _initialMediaSeq;       // Initial media sequence value.
            UStringVector      _customTags;            // Additional custom tags.
            TSPacketLabelSet   _closeLabels;           // Close segment on packets with any of these labels.

            // Working data.
            FileNameGenerator  _nameGenerator;         // Generate the segment file names.
            SectionDemux       _demux;                 // Demux to extract PAT and PMT.
            TSPacketVector     _patPackets;            // TS packets for the PAT at start of each segment file.
            TSPacketVector     _pmtPackets;            // TS packets for the PMT at start of each segment file, after the PAT.
            PID                _pmtPID;                // PID of the PMT of the reference service.
            PID                _videoPID;              // Video PID on which the segmentation is evaluated.
            uint8_t            _videoStreamType;       // Stream type for video PID in PMT.
            bool               _segStarted;            // Generation of output segments has started.
            bool               _segClosePending;       // Close the current segment when possible.
            TSFile             _segmentFile;           // Output segment file.
            UStringList        _liveSegmentFiles;      // List of current segments in a live stream.
            hls::PlayList      _playlist;              // Generated playlist.
            PCRAnalyzer        _pcrAnalyzer;           // PCR analyzer to compute bitrates.
            BitRate            _previousBitrate;       // Bitrate of previous segment.
            ContinuityAnalyzer _ccFixer;               // To fix continuity counters in PAT and PMT PID's.

            // Create the next segment file (also close the previous one if necessary).
            bool createNextSegment();

            // Close current segment file (also purge obsolete segment files and regenerate playlist).
            bool closeCurrentSegment(bool endOfStream);

            // Implementation of TableHandlerInterface.
            virtual void handleTable(SectionDemux&, const BinaryTable&) override;

            // Write packets into the current segment file, adjust CC in PAT and PMT PID.
            bool writePackets(const TSPacket*, size_t);
        };
    }
}
