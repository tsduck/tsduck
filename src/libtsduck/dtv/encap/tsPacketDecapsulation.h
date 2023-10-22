//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard (PES mode by lars18th)
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        PID      _pidInput = PID_NULL;       // Input PID.
        bool     _synchronized = false;      // Input PID fully synchronized.
        uint8_t  _ccInput = 0;               // Continuity counter in input PID.
        size_t   _nextIndex {1};             // Current size of _nextPacket (not full yet), 1 points after sync byte.
        TSPacket _nextPacket {{SYNC_BYTE}};  // Next packet, partially decapsulated, sync byte is implicit.
        UString  _lastError {};              // Last error message.

        // Loose synchronization, return false.
        bool lostSync(const UString& error);
        bool lostSync(TSPacket& pkt, const UString& error);
    };
}
