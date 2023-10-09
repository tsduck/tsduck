//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
