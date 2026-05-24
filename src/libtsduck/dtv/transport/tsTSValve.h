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
    public:
        //!
        //! Constructor.
        //!
        TSValve() = default;

        //!
        //! Reset the transmission.
        //! @param [in] args Valve parameters.
        //! @param [in] initial Initial state, before stating the valve. This is typically TSP_DROP, meaning no packet is passed before starting the processing.
        //! @param [in] first First transmission status.
        //!
        void reset(const TSValveArgs& args, PacketProcessStatus initial, PacketProcessStatus first);

        //!
        //! Change the transmission state.
        //! @param [in] new_status New transmission status.
        //!
        void change(PacketProcessStatus new_status);

        //!
        //! Process one TS packet.
        //! @param [in] pkt Current packet in the transport stream.
        //! @return What to do with this packet.
        //!
        PacketProcessStatus processPacket(const TSPacket& pkt);

    private:
        std::map<PID,PacketProcessStatus> _pid_status {};
    };
}
