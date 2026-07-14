//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
#include "tsStreamType.h"

namespace ts {
    namespace hls {
        //!
        //! HTTP Live Streaming (HLS) output plugin for tsp.
        //! @ingroup libtsduck plugin
        //!
        //! The output plugin generates playlists and media segments on local files
        //! only. It can also purge obsolete media segments and regenerate live
        //! playlists. To setup a complete HLS server, it is necessary to setup an
        //! external HTTP server such as Apache which simply serves these files.
        //!
        class TSDUCKDLL OutputPlugin: public ts::OutputPlugin, private TableHandlerInterface
        {
            TS_PLUGIN_CONSTRUCTORS(OutputPlugin);
        public:
            // Implementation of plugin API
            virtual bool getOptions() override;
            virtual bool start() override;
            virtual bool stop() override;
            virtual bool isRealTime() override;
            virtual bool send(const TSPacket*, const TSPacketMetadata* pkt_data, size_t) override;

        private:
            // Command line options.
            fs::path           _segment_template {};          // Command line segment file names template.
            fs::path           _playlist_file {};             // Playlist file name.
            bool               _intra_close = false;          // Try to start segments on intra images.
            bool               _use_bitrate_tag = false;      // Specify EXT-X-BITRATE tags for each segment in the playlist.
            bool               _align_first_segment = false;  // Align first segment to the first PAT and PMT.
            bool               _slice_only = false;           // Don't add PAT and PMT to the segments.
            hls::PlayListType  _playlist_type = hls::PlayListType::UNKNOWN;
            size_t             _live_depth = 0;               // Number of simultaneous segments in live streams.
            size_t             _live_extra_depth = 0;         // Number of additional segments to keep in live streams.
            cn::seconds        _target_duration {};           // Segment target duration in seconds.
            cn::seconds        _max_extra_duration {};        // Segment target max extra duration in seconds when intra image is not found.
            PacketCounter      _fixed_segment_size = 0;       // Optional fixed segment size in packets.
            size_t             _initial_media_seq = 0;        // Initial media sequence value.
            UStringVector      _custom_tags {};               // Additional custom tags.
            TSPacketLabelSet   _close_labels {};              // Close segment on packets with any of these labels.

            // Working data.
            FileNameGenerator  _name_generator {};            // Generate the segment file names.
            SectionDemux       _demux;                        // Demux to extract PAT and PMT.
            TSPacketVector     _pat_packets {};               // TS packets for the PAT at start of each segment file.
            TSPacketVector     _pmt_packets {};               // TS packets for the PMT at start of each segment file, after the PAT.
            PID                _pmt_pid = PID_NULL;           // PID of the PMT of the reference service.
            PID                _video_pid = PID_NULL;         // Video PID on which the segmentation is evaluated.
            uint8_t            _video_stream_type = ST_NULL;  // Stream type for video PID in PMT.
            bool               _seg_started = false;          // Generation of output segments has started.
            bool               _seg_close_pending = false;    // Close the current segment when possible.
            TSFile             _segment_file {this};          // Output segment file.
            UStringList        _live_segment_files {};        // List of current segments in a live stream.
            hls::PlayList      _playlist {};                  // Generated playlist.
            PCRAnalyzer        _pcr_analyzer {1, 4};          // PCR analyzer to compute bitrates. Minimum required: 1 PID, 4 PCR.
            BitRate            _previous_bitrate = 0;         // Bitrate of previous segment.
            ContinuityAnalyzer _cc_fixer;                     // To fix continuity counters in PAT and PMT PID's.

            static constexpr cn::seconds DEFAULT_OUT_DURATION      = cn::seconds(10); // Default segment target duration for output streams.
            static constexpr cn::seconds DEFAULT_OUT_LIVE_DURATION = cn::seconds(5);  // Default segment target duration for output live streams.
            static constexpr cn::seconds DEFAULT_EXTRA_DURATION    = cn::seconds(2);  // Default segment extra duration when intra image is not found.
            static constexpr size_t      DEFAULT_LIVE_EXTRA_DEPTH  = 1;               // Default additional segments to keep in live streams.

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
