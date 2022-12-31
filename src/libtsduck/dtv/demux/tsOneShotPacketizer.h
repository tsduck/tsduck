//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
