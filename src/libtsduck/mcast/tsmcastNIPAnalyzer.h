//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB-NIP analyzer with extraction and reporting.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastNIPDemux.h"
#include "tsmcastNIPAnalyzerArgs.h"
#include "tsmcastNIPActualCarrierInformation.h"

namespace ts::mcast {
    //!
    //! DVB-NIP analyzer with extraction and reporting.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NIPAnalyzer : private NIPHandlerInterface
    {
        TS_NOBUILD_NOCOPY(NIPAnalyzer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. A reference is kept in this object.
        //!
        NIPAnalyzer(DuckContext& duck);

        //!
        //! Reset the analysis.
        //! @param [in] args Analysis arguments.
        //! @return True on success, false on error.
        //!
        bool reset(const NIPAnalyzerArgs& args);

        //!
        //! The following method feeds the analyzer with an IP packet.
        //! The packet is ignored if this is not a UDP packet.
        //! @param [in] timestamp Packet time stamp value. This value should be taken from a monotonic clock.
        //! @param [in] pkt An IP packet.
        //!
        template <class Rep, class Period>
        void feedPacket(const cn::duration<Rep,Period>& timestamp, const IPPacket& pkt)
        {
            _demux.feedPacket(timestamp, pkt);
        }

        //!
        //! The following method feeds the analyzer with a UDP packet.
        //! @param [in] timestamp Packet time stamp value. This value should be taken from a monotonic clock.
        //! @param [in] source Source socket address.
        //! @param [in] destination Destination socket address.
        //! @param [in] udp Address of UDP payload.
        //! @param [in] udp_size Size in bytes of UDP payload.
        //!
        template <class Rep, class Period>
        void feedPacket(const cn::duration<Rep,Period>& timestamp, const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size)
        {
            _demux.feedPacket(timestamp, source, destination, udp, udp_size);
        }

        //!
        //! Print a summary of the DVB-NIP session.
        //! Print nothing of option @a summary was not specified.
        //! @param [in,out] out Where to print the summary if no output file was specified in NIPAnalyzerArgs.
        //! Ignored when an output file was specified.
        //!
        void printSummary(std::ostream& out = std::cout);

    private:
        // Description of a file.
        class TSDUCKDLL FileContext
        {
        public:
            bool     complete = false;  // The file has been received in this object.
            uint64_t size = 0;          // File size in bytes.
            uint64_t received = 0;      // Received size in bytes.
            uint64_t toi = 0;           // Transport object identifier.
            UString  type {};           // File type.
        };

        // Description of a session.
        class TSDUCKDLL SessionContext
        {
        public:
            std::map<UString, FileContext> files {};  // Description of files, indexed by name.
        };

        // NIPAnalyzer private fields.
        DuckContext&    _duck;
        Report&         _report {_duck.report()};
        NIPAnalyzerArgs _args {};
        NIPDemux        _demux {_duck, this};
        std::map<FluteSessionId, SessionContext> _sessions {};
        std::set<NIPActualCarrierInformation>    _nacis {};

        // Implementation of NIPHandlerInterface.
        virtual void handleFluteFile(const FluteFile&) override;
        virtual void handleFluteNACI(const NIPActualCarrierInformation&) override;
        virtual void handleFluteStatus(const FluteSessionId&, const UString&, const UString&, uint64_t, uint64_t, uint64_t) override;

        // Save a XML file (if the file name is not empty).
        void saveXML(const FluteFile& file, const fs::path& path);

        // Save a carousel file.
        void saveFile(const FluteFile& file, const fs::path& root_dir, const UString& path);
    };
}
