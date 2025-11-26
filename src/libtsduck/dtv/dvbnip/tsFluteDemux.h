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
#include "tsFluteDemuxArgs.h"
#include "tsFluteHandlerInterface.h"
#include "tsFluteSessionId.h"
#include "tsDuckContext.h"

namespace ts {

    class IPPacket;
    class FluteFDT;

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
        //!
        void reset(const FluteDemuxArgs& args);

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

    private:
        // Description of a file being received.
        class TSDUCKDLL FileContext
        {
        public:
            bool     processed = false;      // The file has been processed, ignored subsequent packets.
            uint32_t instance = 0xFFFFFFFF;  // For FDT only: FDT instance.
            uint64_t transfer_length = 0;    // The expected length of the transport object (same as in FTI header).
            uint64_t current_length = 0;     // The number of currently received bytes.
            UString  name {};                // File name or URN.
            UString  type {};                // File MIME type.

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
        std::map<FluteSessionId, SessionContext> _sessions {};

        // Update the announced length of a file. Return true on success, false if the file should be ignored.
        bool updateFileSize(const FluteSessionId& sid, SessionContext& session, uint64_t toi, FileContext& file, uint64_t file_size);

        // Process a complete file.
        void processCompleteFile(const FluteSessionId& sid, SessionContext& session, uint64_t toi, FileContext& file);
    };
}
