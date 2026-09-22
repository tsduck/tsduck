//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB-NIP (Native IP) live service extraction plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractSingleMPEPlugin.h"
#include "tsmcastNIPDemux.h"
#include "tshlsPlayList.h"

namespace ts::mcast {
    //!
    //! DVB-NIP (Native IP) live service extraction plugin.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL NIPExtractPlugin: public AbstractSingleMPEPlugin, private NIPHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(NIPExtractPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;
        virtual void handleSingleMPEPacket(PCR timestamp, TimeSource source, const MPEPacket& mpe) override;

    private:
        // Command line options.
        bool     _use_first_service = false;  // Extract first service.
        UString  _service_name {};            // DVB-NIP service name.
        uint32_t _lcn = 0;                    // DVB-NIP service channel number (if name is empty).

        // Plugin private fields.
        NIPDemux       _demux {duck, this};
        UString        _playlist_url {};       // Playlist of the service.
        hls::PlayList  _playlist {};           // Playlist content.
        FluteSessionId _service_session {};    // Session id of the service content.
        size_t         _output_next = 0;       // Byte index of next TS packet to output in _output.front().
        std::list<ByteBlockPtr> _output {};    // List of contents of segment files to output.

        // Initial playlist acquisition: Before locating the service, we do not know the name of its HLS playlist.
        // When the playlist is a media playlist, it is regularly updated and new versions (with a new FLUTE TOI)
        // are received. On the other hand, when the playlist is a master playlist, it is never updated (it is
        // received with the same TOI all the time). The application is notified only once, the first time it is
        // received. It this reception occurs before locating the service, we don't know yet that this playlist
        // will be needed later, and we won't receive another copy. Therefore, before locating the service, we
        // build a cache of all received playlist. Once the service is located, we clear it and no longer use it.
        // This cache is indexed by file name.
        std::map<UString, FluteFile> _initial_playlist_cache {};

        // Segment caching ahead of playlist: When a playlist is received, all segments are supposed to be availble
        // on the receiver. Therefore, a segment is always sent _before_ the first playlist which references it.
        // When we receive a file which is a segment of the service, we don't know yet that this is a segment of the
        // service because we have not yet received a playlist which references it. On the other hand, we don't want
        // to cache all received files (too large). So, we try to "guess" if a received file may be future segment of
        // the service. All these files are cached here, in order of reception.
        std::list<FluteFile> _ahead_segment_cache {};

        // However, because we usually use the last segment of a service (this is live), the playlist becomes
        // empty quite often and we lose the capability to compare a file name with path and extension of segments.
        // Therefore, we save the last one here.
        UString _last_segment_path {};
        UString _last_segment_ext {};

        // This method "guesses" if a file is maybe a future segment (see previous comment).
        bool maybeFutureSegment(const UString& name);

        // Process an update of the playlist of the service.
        void processPlayList(const FluteFile& file);

        // Check if a file is a known segment in the playlist. Enqueue its contents is yes.
        // Return true if the file is a known segment and has been enqueued for output.
        bool processSegment(const FluteFile& file);

        // Implementation of NIPHandlerInterface.
        virtual void handleNewService(const NIPService& service) override;
        virtual void handleFluteFile(const FluteFile& file) override;

        // Check if a file can be a HLS playlist.
        static bool IsValidPlayListName(const UString& file_name, const UString& file_type);
    };
}
