//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard (PES mode by lars18th)
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
//!  An efficient TSDuck-specific TS packets decapsulation from a PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"

namespace ts {
    //!
    //! An efficient TSDuck-specific TS packets decapsulation from a PID.
    //! This class extract packets which were encapsulated using PacketEncapsulation.
    //! @ingroup mpeg
    //! @see PacketEncapsulation
    //!
    class TSDUCKDLL PacketDecapsulation
    {
        TS_NOCOPY(PacketDecapsulation);
    public:
        //!
        //! Constructor.
        //! @param [in] pid The PID containing encapsulated packets. When PID_NULL, no decapsulation is done.
        //!
        PacketDecapsulation(PID pid = PID_NULL);

        //!
        //! Reset the decapsulation.
        //! @param [in] pid The PID containing encapsulated packets. When PID_NULL, no decapsulation is done.
        //!
        void reset(PID pid = PID_NULL);

        //!
        //! Process a TS packet from the input stream.
        //! @param [in,out] pkt A TS packet. If the packet belongs to the encapsulated
        //! PID, it is replaced by an decapsulating packet.
        //! @return True on success, false on error (PID conflict).
        //! In case of error, use lastError().
        //!
        bool processPacket(TSPacket& pkt);

        //!
        //! Get the last error message.
        //! @return The last error message.
        //!
        const UString& lastError() const { return _lastError; }

        //!
        //! Check if a previous error is pending.
        //! @return True if a previous error is pending.
        //! @see resetError()
        //!
        bool hasError() const { return !_lastError.empty(); }

        //!
        //! Reset the last error.
        //!
        void resetError() { _lastError.clear(); }

        //!
        //! Get the input PID.
        //! @return The input PID.
        //!
        PID inputPID() const { return _pidInput; }

    private:
        PID      _pidInput;      // Input PID.
        bool     _synchronized;  // Input PID fully synchronized.
        uint8_t  _ccInput;       // Continuity counter in input PID.
        size_t   _nextIndex;     // Current size of _nextIndex (not full yet).
        TSPacket _nextPacket;    // Next packet, partially decapsulated.
        UString  _lastError;     // Last error message.

        // Loose synchronization, return false.
        bool lostSync(const UString& error);
        bool lostSync(TSPacket& pkt, const UString& error);
    };
}
