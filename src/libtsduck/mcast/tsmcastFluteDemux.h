//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  This class extract files from FLUTE streams in UDP datagrams.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteDemuxArgs.h"
#include "tsmcastFluteHandlerInterface.h"
#include "tsmcastFluteSessionId.h"
#include "tsmcastFluteFDT.h"
#include "tsIPPacket.h"
#include "tsDuckContext.h"

namespace ts::mcast {
    //!
    //! This class extract files from FLUTE streams in UDP datagrams.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteDemux
    {
        TS_NOBUILD_NOCOPY(FluteDemux);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] handler The object to invoke when FLUTE information is found.
        //!
        explicit FluteDemux(DuckContext& duck, FluteHandlerInterface* handler = nullptr);

        //!
        //! Destructor.
        //!
        ~FluteDemux();

        //!
        //! Replace the FLUTE handler.
        //! @param [in] h The new handler.
        //!
        void setHandler(FluteHandlerInterface* h) { _handler = h; }

        //!
        //! Reset the demux.
        //! @param [in] args Demux arguments.
        //! @return True on success, false on error.
        //!
        bool reset(const FluteDemuxArgs& args);

        //!
        //! The following method feeds the demux with an IP packet.
        //! The packet is ignored if this is not a UDP packet.
        //! @param [in] timestamp Packet time stamp value. This value should be taken from a monotonic clock.
        //! @param [in] pkt An IP packet.
        //!
        template <class Rep, class Period>
        void feedPacket(const cn::duration<Rep,Period>& timestamp, const IPPacket& pkt);

        //!
        //! The following method feeds the demux with a UDP packet.
        //! @param [in] timestamp Packet time stamp value. This value should be taken from a monotonic clock.
        //! @param [in] source Source socket address.
        //! @param [in] destination Destination socket address.
        //! @param [in] udp Address of UDP payload.
        //! @param [in] udp_size Size in bytes of UDP payload.
        //!
        template <class Rep, class Period>
        void feedPacket(const cn::duration<Rep,Period>& timestamp, const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size)
        {
            feedPacketImpl(cn::duration_cast<cn::microseconds>(timestamp), source, destination,udp, udp_size);
        }

        //!
        //! Get the current status of all file transfers.
        //! The current handler is invoked on method @a handleFluteStatus() for each file,
        //! either completely or partially transfered.
        //!
        void getFilesStatus();

    private:
        // Description of a file being received.
        class TSDUCKDLL FileContext
        {
        public:
            bool             processed = false;      // The file has been processed, ignored subsequent packets.
            uint32_t         instance = 0xFFFFFFFF;  // For FDT only: FDT instance.
            uint64_t         transfer_length = 0;    // The expected length of the transport object (same as in FTI header).
            uint64_t         current_length = 0;     // The number of currently received bytes.
            UString          name {};                // File name or URN.
            UString          type {};                // File MIME type.
            cn::microseconds last_time {};           // Timestamp of last received data for the file.

            // Chunks of the file being received.
            // First level of index: Source Block Number (SBN).
            // Second level of index: Encoding Symbol ID in source block.
            // Erased when the file is processed to save storage.
            std::vector<std::vector<ByteBlockPtr>> chunks {};

            // Reset the content.
            void clear();
        };

        // Description of a session.
        class TSDUCKDLL SessionContext
        {
        public:
            std::optional<uint32_t>         fdt_instance {};  // Current FDT instance.
            std::map<uint64_t, FileContext> files {};         // Files contexts, indexes by TOI (Transport Object Identifier).
        };

        // FluteDemux private fields.
        DuckContext&           _duck;
        Report&                _report {_duck.report()};
        FluteHandlerInterface* _handler = nullptr;
        FluteDemuxArgs         _args {};
        uint64_t               _packet_count = 0;               // Number of IP packets.
        cn::microseconds       _next_gc_timestamp {};           // Timestamps of next garbage collection.
        std::map<FluteSessionId, SessionContext> _sessions {};  // Session contexts, indexed by session id.

        // Avoid accumulation of old file descriptions which clutter the memory:
        // 1) When a file is completely received and passed to the application (processed = true), we deallocate the data chunks
        //    because they are no longer necessary. However, we keep the FileContext for a while in case packets are repeated for
        //    that file. In that case, we need to remember that this file was processed and avoid recreate a "new file".
        // 2) When parts of a large file are lost, the file will never be complete and will never be passed to the application.
        //    Therefore, its FileContext will stay allocated forever.
        // To avoid an ever-growing number of FileContext (with or without data chunk), we do some periodic garbage collection (gc).
        // The gc is executed every '_gc_interval'. All files for which no packet has been received in the last '_file_max_lifetime'
        // is purged. These values are currenty hard-coded (with default from environment variables) but may become parameters in
        // the future.
        static const cn::microseconds _gc_interval;
        static const cn::microseconds _file_max_lifetime;

        // Execute the garbage collector.
        void garbageCollector(const cn::microseconds& current_timestamp);

        // Feed the analyzer with a UDP packet (non template version).
        void feedPacketImpl(const cn::microseconds& timestamp, const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size);

        // Update the announced length of a file. Return true on success, false if the file should be ignored.
        bool updateFileSize(const FluteSessionId& sid, SessionContext& session, uint64_t toi, FileContext& file, uint64_t file_size);

        // Process a complete file.
        void processCompleteFile(const FluteSessionId& sid, SessionContext& session, uint64_t toi, FileContext& file);

        // Process a File Delivery Table (FDT).
        void processFDT(SessionContext& session, const FluteFDT& fdt);
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Feed the analyzer with a UDP packet (IPPacket version).
template <class Rep, class Period>
void ts::mcast::FluteDemux::feedPacket(const cn::duration<Rep,Period>& timestamp, const IPPacket& pkt)
{
    if (pkt.isUDP()) {
        feedPacketImpl(cn::duration_cast<cn::microseconds>(timestamp), pkt.source(), pkt.destination(), pkt.protocolData(), pkt.protocolDataSize());
    }
}

#endif // DOXYGEN
