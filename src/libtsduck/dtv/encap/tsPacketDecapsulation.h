//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard (PES mode by lars18th)
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
    //! @ingroup libtsduck mpeg
    //! @see PacketEncapsulation
    //!
    class TSDUCKDLL PacketDecapsulation
    {
        TS_NOBUILD_NOCOPY(PacketDecapsulation);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to log error or debug messages.
        //! @param [in] pid The PID containing encapsulated packets. When PID_NULL, no decapsulation is done.
        //!
        PacketDecapsulation(Report& report, PID pid = PID_NULL);

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
        const UString& lastError() const { return _last_error; }

        //!
        //! Check if a previous error is pending.
        //! @return True if a previous error is pending.
        //! @see resetError()
        //!
        bool hasError() const { return !_last_error.empty(); }

        //!
        //! Reset the last error.
        //!
        void resetError() { _last_error.clear(); }

        //!
        //! Get the input PID.
        //! @return The input PID.
        //!
        PID inputPID() const { return _input_pid; }

    private:
        [[maybe_unused]] Report& _report;
        PacketCounter _packet_count = 0;           // Number of processed packets.
        PID           _input_pid = PID_NULL;       // Input PID.
        bool          _synchronized = false;       // Input PID fully synchronized.
        uint8_t       _cc_input = 0;               // Continuity counter in input PID.
        size_t        _next_index {1};             // Current size of _next_packet (not full yet), 1 points after sync byte.
        TSPacket      _next_packet {{SYNC_BYTE}};  // Next packet, partially decapsulated, sync byte is implicit.
        UString       _last_error {};              // Last error message.

        // Loose synchronization, return false.
        bool lostSync(const UString& error);
        bool lostSync(TSPacket& pkt, const UString& error);
    };
}
