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
#include "tsFluteHandlerInterface.h"
#include "tsIPSocketAddress.h"
#include "tsDuckContext.h"

namespace ts {

    class IPPacket;
    class IPSocketAddress;

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
        //! The following method feeds the demux with an IP packet.
        //! The packet is ignored if this is not a UDP packet.
        //! @param [in] pkt An IP packet.
        //!
        void feedPacket(const IPPacket& pkt);

        //!
        //! The following method feeds the demux with a UDP packet.
        //! @param [in] source Source socket address.
        //! @param [in] destination Destination socket address.
        //! @param [in] udp Address of UDP payload.
        //! @param [in] udp_size Size in bytes of UDP payload.
        //!
        void feedPacket(const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size);

        //!
        //! Reset the demux.
        //!
        void reset();

        //!
        //! Add a Transport Session Identifier (TSI) to filter.
        //! By default, the demux processes all sessions. If one or more TSI are specified,
        //! only the corresponding sessions are demuxed.
        //! @param [in] tsi The Transport Session Identifier (TSI) to filter.
        //!
        void addTSI(uint64_t tsi) { _tsi_filter.insert(tsi); }

        //!
        //! Remove a Transport Session Identifier (TSI) to filter.
        //! @param [in] tsi The Transport Session Identifier (TSI) to remove.
        //! @see addTSI()
        //!
        void removeTSI(uint64_t tsi) { _tsi_filter.erase(tsi); }

        //!
        //! Set the severity level at which the FLUTE packets are logged.
        //! The default is Severity::Debug.
        //! @param [in] level Severity level at which the FLUTE packets are logged.
        //!
        void setPacketLogLevel(int level) { _packet_log_level = level; }

        //!
        //! Specify if the content of the FLUTE packets is dumped when the packet is logged.
        //! The default is false.
        //! @param [in] on True if the content of the FLUTE packets is dumped.
        //! @see setPacketLogLevel()
        //!
        void logPacketContent(bool on) { _log_packet_content = on; }

    private:
        // A "session" is defined by source address (not port), destination address and port, TSI.
        class TSDUCKDLL SessionId
        {
        public:
            IPAddress       source {};
            IPSocketAddress destination {};
            uint64_t        tsi = 0;

            // Comparison operator for use as index in maps.
            bool operator<(const SessionId& other) const;
        };

        // Description of a file being received.
        class TSDUCKDLL FileContext
        {
        public:
            bool      processed = false;          // The file has been processed, ignored subsequent packets.
            uint32_t  instance = 0xFFFFFFFF;      // For FDT only: FDT instance.
            uint64_t  transfer_length = 0;        // The expected length of the transport object (same as in FTI header).
            uint64_t  current_length = 0;         // The number of currently received bytes.
            UString   name {};                    // File name or URN.
            std::vector<ByteBlockPtr> chunks {};  // Chunks of the file being received. Erased when processed to save storage.

            // Reset the content.
            void clear();
        };

        // Description of a session.
        class TSDUCKDLL SessionContext
        {
        public:
            uint32_t fdt_instance = 0;                 // Current FDT instance.
            std::map<uint64_t, FileContext> files {};  // Files contexts, indexes by TOI (Transport Object Identifier).

            // Process a new FDT.
            void processFDT(uint32_t instance, const UString& xml);
        };

        // FluteDemux private fields.
        DuckContext&           _duck;
        Report&                _report {_duck.report()};
        FluteHandlerInterface* _handler = nullptr;
        int                    _packet_log_level = Severity::Debug;
        bool                   _log_packet_content = false;
        std::set<uint64_t>     _tsi_filter {};
        std::map<SessionId, SessionContext> _sessions {};
    };
}
