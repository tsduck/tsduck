//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Packetization of PES data into Transport Stream packets in one shot.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPESStreamPacketizer.h"
#include "tsTSPacket.h"

namespace ts {
    //!
    //! Packetization of PES data into Transport Stream packets in one shot.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL PESOneShotPacketizer: public PESStreamPacketizer
    {
        TS_NOBUILD_NOCOPY(PESOneShotPacketizer);
    public:
        //!
        //! Constructor.
        //! @param [in] duck TSDuck execution context. The reference is kept inside the packetizer.
        //! @param [in] pid PID for generated TS packets.
        //!
        PESOneShotPacketizer(const DuckContext& duck, PID pid = PID_NULL);

        //!
        //! Destructor
        //!
        virtual ~PESOneShotPacketizer() override;

        //!
        //! Get all enqueued PES packets as one list of TS packets.
        //! @param [out] packets Returned list of TS packets containing all TS packets.
        //!
        void getPackets(TSPacketVector& packets);

    private:
        // Hide these methods
        virtual bool getNextPacket(TSPacket&) override;
    };
}
