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

    private:
        DuckContext&    _duck;
        Report&         _report {_duck.report()};
        NIPAnalyzerArgs _args {};
        FluteDemux      _flute_demux {_duck, this};

        // Inherited methods.
        virtual void handleFluteFile(FluteDemux&, const FluteFile&) override;
        virtual void handleFluteFDT(FluteDemux&, const FluteFDT&) override;
    };
}
