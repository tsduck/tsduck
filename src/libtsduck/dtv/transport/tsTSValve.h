//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream "valve": pass or drop TS packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsTSPacket.h"
#include "tsTSValveArgs.h"

namespace ts {

    class Args;

    //!
    //! Transport stream "valve": pass or drop TS packets with transition period management.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TSValve
    {
    private:
        TSValve() = delete;
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report debug messages.
        //!
        TSValve(Report& report) : _report(report) {}

        //!
        //! Reset the transmission.
        //! @param [in] args Valve parameters.
        //! @param [in] initial_status Initial status, before stating the valve. This is typically TSP_DROP, meaning no packet is passed before starting the processing.
        //! @param [in] first_status First transmission status.
        //!
        void reset(const TSValveArgs& args, PacketProcessStatus initial_status, PacketProcessStatus first_status);

        //!
        //! Change the transmission status.
        //! @param [in] new_status New transmission status.
        //!
        void change(PacketProcessStatus new_status);

        //!
        //! Get current transmission status.
        //! @return Current transmission status.
        //!
        PacketProcessStatus current() const { return _current_status; }

        //!
        //! Process one TS packet.
        //! @param [in] pkt Current packet in the transport stream.
        //! @return What to do with this packet.
        //!
        PacketProcessStatus processPacket(const TSPacket& pkt);

    private:
        // Description of one PID.
        class TSDUCKDLL PIDContext
        {
        public:
            PIDContext() = default;
            PacketCounter last_packet = 0;   // Index in TS of last packet with that PID.
            bool transitioning = false;      // Waiting for a unit boundary.
            bool new_pid = true;             // Detect insertion of a new entry in the map.
        };

        // Meaningful status are OK, DROP, NULL TSP_END does not mean anything here.
        static PacketProcessStatus NormalizeStatus(PacketProcessStatus);

        Report&                  _report;
        PacketCounter            _total_packets = 0;        // Packet counter in TS since reset().
        PacketCounter            _next_silent_check = 0;    // Index in TS of next time we need to check for silent PID's.
        size_t                   _transitioning_pids = 0;   // Number of PIDs waiting for a unit boundary.
        PacketProcessStatus      _previous_status = TSP_OK;
        PacketProcessStatus      _current_status = TSP_OK;
        TSValveArgs              _args {};
        std::map<PID,PIDContext> _pid_contexts {};
        const Names&             _names {PacketProcessingStatusNames()};
    };
}
