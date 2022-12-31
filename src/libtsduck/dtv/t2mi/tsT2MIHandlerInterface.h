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
//!  T2-MI demux handler interface.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"

namespace ts {

    class T2MIDemux;
    class T2MIDescriptor;
    class T2MIPacket;
    class PMT;

    //!
    //! T2-MI demux handler interface.
    //! @ingroup mpeg
    //!
    //! This abstract interface must be implemented by classes which need to be
    //! notified of PID's and packets using a T2MIDemux.
    //!
    class TSDUCKDLL T2MIHandlerInterface
    {
    public:
        //!
        //! This hook is invoked when a new PID carrying T2-MI is available.
        //! @param [in,out] demux A reference to the T2-MI demux.
        //! @param [in] pmt The PMT of the service describing this PID.
        //! @param [in] pid The PID carrying T2-MI encapsulation.
        //! @param [in] desc The T2MI_descriptor for this PID.
        //!
        virtual void handleT2MINewPID(T2MIDemux& demux, const PMT& pmt, PID pid, const T2MIDescriptor& desc) = 0;

        //!
        //! This hook is invoked when a new T2-MI packet is available.
        //! @param [in,out] demux A reference to the T2-MI demux.
        //! @param [in] pkt The T2-MI packet.
        //!
        virtual void handleT2MIPacket(T2MIDemux& demux, const T2MIPacket& pkt) = 0;

        //!
        //! This hook is invoked when a new TS packet is extracted.
        //! @param [in,out] demux A reference to the T2-MI demux.
        //! @param [in] t2mi The T2-MI packet from which @a ts was extracted.
        //! @param [in] ts The extracted TS packet.
        //!
        virtual void handleTSPacket(T2MIDemux& demux, const T2MIPacket& t2mi, const TSPacket& ts) = 0;

        //!
        //! Virtual destructor.
        //!
        virtual ~T2MIHandlerInterface();
    };
}
