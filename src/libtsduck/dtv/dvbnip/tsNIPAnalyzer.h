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
#include "tsFluteFDT.h"
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
        //!
        void reset(const NIPAnalyzerArgs& args);

        //!
        //! The following method feeds the analyzer with an IP packet.
        //! The packet is ignored if this is not a UDP packet.
        //! @param [in] pkt An IP packet.
        //!
        void feedPacket(const IPPacket& pkt);

        //!
        //! The following method feeds the analyzer with a UDP packet.
        //! @param [in] source Source socket address.
        //! @param [in] destination Destination socket address.
        //! @param [in] udp Address of UDP payload.
        //! @param [in] udp_size Size in bytes of UDP payload.
        //!
        void feedPacket(const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size);

        //!
        //! Add a FLUTE session in the DVB-NIP analyzer.
        //! There is normally no reason to call this from the application.
        //! The analyzer always starts with the DVB-NIP Announcement Channel on reset().
        //! Then, all declared sessions in the DVB-NIP tables are automatically added.
        //! @param [in] session The session id to add.
        //!
        void addSession(const FluteSessionId& session);

    private:
        DuckContext&    _duck;
        Report&         _report {_duck.report()};
        NIPAnalyzerArgs _args {};
        FluteDemux      _flute_demux {_duck, this};
        std::set<FluteSessionId> _session_filter {};

        // Inherited methods.
        virtual void handleFluteFile(FluteDemux&, const FluteFile&) override;
        virtual void handleFluteFDT(FluteDemux&, const FluteFDT&) override;

        // Check if a UDP packet or FLUTE file is part of a filtered session.
        bool isFiltered(const IPAddress& source, const IPSocketAddress& destination) const;
        bool isFiltered(const FluteSessionId& session) const;

        // Save a XML file (if the file name is not empty).
        void saveXML(const FluteFile& file, const fs::path& path, const std::optional<uint32_t> instance = std::optional<uint32_t>());
    };
}
