//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  An efficient TSDuck-specific TS packets encapsulation in a PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"
#include "tsTSPacket.h"

namespace ts {
    //!
    //! An efficient TSDuck-specific TS packets encapsulation in a PID.
    //! @ingroup mpeg
    //!
    //! An instance of this class encapsulates several PID's from the
    //! input transport stream into one single output PID. Functionally,
    //! this is a subset of the features of T2-MI but much more lightweight
    //! and significantly faster to process.
    //!
    //! Encapsulation format
    //! --------------------
    //! In the output elementary stream (ES), all input TS packets are
    //! contiguous, without encapsulation. The initial 0x47 synchronization
    //! byte is removed. Only the remaining 187 bytes are encapsulated.
    //!
    //! In the output PID, the packetization is similar to sections, with
    //! 187-bytes packets instead of sections. The Payload Unit Start
    //! Indicator (PUSI) bit is set in the header of TS packets containing
    //! the start of an encapsulated packet. When the PUSI bit is set, the
    //! first byte of the payload is a "pointer field" to the beginning of
    //! the first encapsulated packet.
    //!
    //! Due to the overhead of the TS header, the number of output packets
    //! is slightly larger than the input packets. The input streams must
    //! contain a few null packets to absorb the extra output packets. For
    //! this reason, null packets (PID 0x1FFF) are never encapsulated.
    //!
    class TSDUCKDLL PacketEncapsulation
    {
    public:
        //!
        //! Constructor.
        //! @param [in] pidOutput The output PID. When PID_NULL, no encapsulation is done.
        //! @param [in] pidInput The initial set of PID's to encapsulate.
        //! @param [in] pcrReference The PID with PCR's to use as reference to add PCR's in
        //! the encapsulating PID. When PID_NULL, do not add PCR.
        //!
        PacketEncapsulation(PID pidOutput = PID_NULL, const PIDSet& pidInput = NoPID, PID pcrReference = PID_NULL);

        //!
        //! Reset the encapsulation.
        //! @param [in] pidOutput The new output PID. When PID_NULL, no encapsulation is done.
        //! @param [in] pidInput The new set of PID's to encapsulate.
        //! @param [in] pcrReference The PID with PCR's to use as reference to add PCR's in
        //! the encapsulating PID. When PID_NULL, do not add PCR.
        //!
        void reset(PID pidOutput = PID_NULL, const PIDSet& pidInput = NoPID, PID pcrReference = PID_NULL);

        //!
        //! Process a TS packet from the input stream.
        //! @param [in,out] pkt A TS packet. If the packet belongs to one of the input
        //! PID's, it is replaced by an encapsulating packet in the output PID. Some
        //! null packets are also replaced to absorb the encapsulation overhead.
        //! @return True on success, false on error (PID conflict, output overflow).
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
        //! Get the output PID.
        //! @return The output PID.
        //!
        PID outputPID() const { return _pidOutput; }

        //!
        //! Change the output PID.
        //! @param [in] pid The new output PID.
        //!
        void setOutputPID(PID pid) { _pidOutput = pid; }

        //!
        //! Get the current set of input PID's.
        //! @return A constant reference to the set of input PID's.
        //!
        const PIDSet& inputPIDs() const { return _pidInput; }

        //!
        //! Get the current number of input PID's being encapsulated.
        //! @return The current number of PID's being encapsulated.
        //!
        size_t pidCount() const { return _pidInput.count(); }

        //!
        //! Replace the set of input PID's.
        //! @param [in] pidInput The new set of PID's to encapsulate.
        //!
        void setInputPIDs(const PIDSet& pidInput) { _pidInput = pidInput; }

        //!
        //! Add one PID to encapsulate.
        //! @param [in] pid The new PID to encapsulate.
        //!
        void addInputPID(PID pid) { _pidInput.set(pid); }

        //!
        //! Remove one PID to encapsulate.
        //! @param [in] pid The PID to no longer encapsulate.
        //!
        void removeInputPID(PID pid) { _pidInput.reset(pid); }

        //!
        //! Get the reference PID for PCR's.
        //! @return The reference PID for PCR's. PID_NULL if there is none.
        //!
        PID referencePCR() const { return _pcrReference; }

        //!
        //! Change the reference PID for PCR's.
        //! @param [in] pid The new reference PID for PCR's. PID_NULL if there is none.
        //!
        void setReferencePCR(PID pid);

    private:
        PID     _pidOutput;     // Output PID.
        PIDSet  _pidInput;      // Input PID's to encapsulate.
        PID     _pcrReference;  // Insert PCR's based on this reference PID.
        UString _lastError;     // Last error message.

        // Inaccessible operations.
        PacketEncapsulation(const PacketEncapsulation&) = delete;
        PacketEncapsulation& operator=(const PacketEncapsulation&) = delete;
    };
}
