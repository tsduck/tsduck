//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB-NIP analyzer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNIPAnalyzerArgs.h"
#include "tsDuckContext.h"
#include "tsFluteDemux.h"
#include "tsNIPActualCarrierInformation.h"
#include "tsIPSocketAddress.h"
#include "tsIPPacket.h"

namespace ts {
    //!
    //! DVB-NIP analyzer.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NIPAnalyzer : private FluteHandlerInterface
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
        void feedPacket(const cn::duration<Rep,Period>& timestamp, const IPPacket& pkt);

        //!
        //! The following method feeds the analyzer with a UDP packet.
        //! @param [in] timestamp Packet time stamp value. This value should be taken from a monotonic clock.
        //! @param [in] source Source socket address.
        //! @param [in] destination Destination socket address.
        //! @param [in] udp Address of UDP payload.
        //! @param [in] udp_size Size in bytes of UDP payload.
        //!
        template <class Rep, class Period>
        void feedPacket(const cn::duration<Rep,Period>& timestamp, const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size);

        //!
        //! Add a FLUTE session in the DVB-NIP analyzer.
        //! There is normally no reason to call this from the application.
        //! The analyzer always starts with the DVB-NIP Announcement Channel on reset().
        //! Then, all declared sessions in the DVB-NIP tables are automatically added.
        //! @param [in] session The session id to add.
        //!
        void addSession(const FluteSessionId& session);

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
        FluteDemux      _flute_demux {_duck, this};
        std::set<FluteSessionId>                 _session_filter {};
        std::map<FluteSessionId, SessionContext> _sessions {};
        std::set<NIPActualCarrierInformation>    _nacis {};

        // Inherited methods.
        virtual void handleFluteFile(FluteDemux&, const FluteFile&) override;
        virtual void handleFluteNACI(FluteDemux&, const NIPActualCarrierInformation&) override;
        virtual void handleFluteStatus(FluteDemux&, const FluteSessionId&, const UString&, const UString&, uint64_t, uint64_t, uint64_t) override;

        // Check if a UDP packet or FLUTE file is part of a filtered session.
        bool isFiltered(const IPAddress& source, const IPSocketAddress& destination) const;
        bool isFiltered(const FluteSessionId& session) const;

        // Save a XML file (if the file name is not empty).
        void saveXML(const FluteFile& file, const fs::path& path);

        // Save a carousel file.
        void saveFile(const FluteFile& file, const fs::path& root_dir, const UString& path);
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Feed the analyzer with a UDP packet (IPPacket version).
template <class Rep, class Period>
void ts::NIPAnalyzer::feedPacket(const cn::duration<Rep,Period>& timestamp, const IPPacket& pkt)
{
    if (pkt.isUDP()) {
        feedPacket(cn::duration_cast<cn::microseconds>(timestamp), pkt.source(), pkt.destination(), pkt.protocolData(), pkt.protocolDataSize());
    }
}

// Feed the analyzer with a UDP packet (IPPacket version).
template <class Rep, class Period>
void ts::NIPAnalyzer::feedPacket(const cn::duration<Rep,Period>& timestamp, const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size)
{
    // Feed the FLUTE demux with possibly filtered packets. The TSI is not yet accessible, only the addresses.
    if (isFiltered(source, destination)) {
        _flute_demux.feedPacket(timestamp, source, destination, udp, udp_size);
    }
}

#endif // DOXYGEN
