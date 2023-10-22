//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Packetization of MPEG sections into Transport Stream packets in one shot
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCyclingPacketizer.h"
#include "tsTSPacket.h"

namespace ts {
    //!
    //! Packetization of MPEG sections into Transport Stream packets in one shot.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL OneShotPacketizer: public CyclingPacketizer
    {
        TS_NOBUILD_NOCOPY(OneShotPacketizer);
    public:
        //!
        //! Default constructor.
        //! @param [in] duck TSDuck execution context. The reference is kept inside the packetizer.
        //! @param [in] pid PID for generated TS packets.
        //! @param [in] do_stuffing TS packet stuffing at end of section.
        //! @param [in] bitrate Output bitrate, zero if undefined.
        //! Useful only when using specific repetition rates for sections
        //!
        OneShotPacketizer(const DuckContext& duck, PID pid = PID_NULL, bool do_stuffing = false, const BitRate& bitrate = 0);

        //!
        //! Virtual destructor.
        //!
        virtual ~OneShotPacketizer() override;

        //!
        //! Set the stuffing policy.
        //! @param [in] do_stuffing TS packet stuffing at end of section.
        //!
        void setStuffingPolicy(bool do_stuffing)
        {
            CyclingPacketizer::setStuffingPolicy(do_stuffing ? StuffingPolicy::ALWAYS : StuffingPolicy::AT_END);
        }

        //!
        //! Get a complete cycle as one list of TS packets.
        //! @param [out] packets Returned list of TS packets containing a complete cycle.
        //!
        void getPackets(TSPacketVector& packets);

    private:
        // Hide these methods
        void setStuffingPolicy(StuffingPolicy) = delete;
        virtual bool getNextPacket(TSPacket&) override;
    };
}
