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

    private:
        DuckContext&           _duck;
        Report&                _report {_duck.report()};
        FluteHandlerInterface* _handler = nullptr;
    };
}
