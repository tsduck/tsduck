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
//!  MPE demux handler interface.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"

namespace ts {

    class MPEDemux;
    class MPEPacket;
    class PMT;

    //!
    //! MPE (Multi-Protocol Encapsulation) demux handler interface.
    //! @ingroup mpeg
    //!
    //! This abstract interface must be implemented by classes which need to be
    //! notified of PID's and packets using a MPEDemux.
    //!
    class TSDUCKDLL MPEHandlerInterface
    {
    public:
        //!
        //! This hook is invoked when a new PID carrying MPE is available.
        //! @param [in,out] demux A reference to the MPE demux.
        //! @param [in] pmt The PMT of the service describing this PID.
        //! @param [in] pid The PID carrying MPE sections.
        //!
        virtual void handleMPENewPID(MPEDemux& demux, const PMT& pmt, PID pid) = 0;

        //!
        //! This hook is invoked when a new MPE packet is available.
        //! @param [in,out] demux A reference to the MPE demux.
        //! @param [in] mpe The MPE packet.
        //!
        virtual void handleMPEPacket(MPEDemux& demux, const MPEPacket& mpe) = 0;

        //!
        //! Virtual destructor.
        //!
        virtual ~MPEHandlerInterface();
    };
}
