//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  FLUTE analyzer with extraction and reporting.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteDemux.h"
#include "tsmcastFluteAnalyzerArgs.h"

namespace ts::mcast {
    //!
    //! FLUTE analyzer with extraction and reporting.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteAnalyzer : private FluteHandlerInterface
    {
        TS_NOBUILD_NOCOPY(FluteAnalyzer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. A reference is kept in this object.
        //!
        FluteAnalyzer(DuckContext& duck);

        //!
        //! Reset the analysis.
        //! @param [in] args Analysis arguments.
        //! @return True on success, false on error.
        //!
        bool reset(const FluteAnalyzerArgs& args);

        //!
        //! The following method feeds the analyzer with an IP packet.
        //! The packet is ignored if this is not a UDP packet.
        //! @param [in] timestamp Packet time stamp value. This value should be taken from a monotonic clock.
        //! @param [in] pkt An IP packet.
        //!
        template <class Rep, class Period>
        void feedPacket(const cn::duration<Rep,Period>& timestamp, const IPPacket& pkt)
        {
            feedPacket(timestamp, pkt.source(), pkt.destination(), pkt.protocolData(), pkt.protocolDataSize());
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
            if (_args.isDestination(destination)) {
                _demux.feedPacket(timestamp, source, destination, udp, udp_size);
            }
        }

        //!
        //! Print a summary of the FLUTE session.
        //! Print nothing of option @a summary was not specified.
        //! @param [in,out] out Where to print the summary if no output file was specified in FluteAnalyzerArgs.
        //! Ignored when an output file was specified.
        //!
        void printSummary(std::ostream& out = std::cout);

    private:
        // NIPAnalyzer private fields.
        DuckContext&      _duck;
        Report&           _report {_duck.report()};
        FluteAnalyzerArgs _args {};
        FluteDemux        _demux {_duck, this};

        // Implementation of NIPHandlerInterface.
        virtual void handleFluteFile(const FluteFile&) override;

        // Save a carousel file.
        void saveFile(const FluteFile& file);
    };
}
