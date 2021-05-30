//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  Perform various transformations on an EIT PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsTSPacket.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Generate and inserts EIT sections based on an EPG content.
    //! @ingroup mpeg
    //!
    //! The object is continuously invoked for all packets in a TS.
    //! Packets from the EIT PID or the stuffing PID are replaced.
    //!
    class TSDUCKDLL EITGenerator : private SectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(EITGenerator);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside this object.
        //! @param [in] pid The PID containing EIT's to insert.
        //!
        explicit EITGenerator(DuckContext& duck, PID pid = PID_EIT);

        //!
        //! Reset the EIT generator to default state.
        //!
        void reset();

        //!
        //! Process one packet from the stream.
        //! @param [in,out] pkt A TS packet from the stream. If the packet belongs
        //! to the EIT PID or the null PID, it may be updated with new content.
        //!
        void processPacket(TSPacket& pkt);

    private:
        DuckContext&          _duck;
        const PID             _eit_pid;
        SectionDemux          _demux;
        CyclingPacketizer     _packetizer;
        std::list<SectionPtr> _sections;

        // Implementation of SectionHandlerInterface.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;
    };
}
