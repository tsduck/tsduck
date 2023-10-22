//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract interface for classes which provides PES packets into
//!  a Packetizer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPESPacket.h"

namespace ts {
    //!
    //! Abstract interface for classes which provide PES packets into a Packetizer.
    //! @ingroup mpeg
    //!
    //! This abstract interface must be implemented by classes which provide PES packets into a Packetizer.
    //!
    class TSDUCKDLL PESProviderInterface
    {
        TS_INTERFACE(PESProviderInterface);
    public:
        //!
        //! This hook is invoked when a new PES packet is required.
        //! @param [in] counter The PES counter is an information on the progression
        //! (zero the first time the hook is invoked from the packetizer).
        //! @param [out] pes A smart pointer to the next PES packet to packetize.
        //! If a null pointer is provided, no PES packet is available.
        //!
        virtual void providePESPacket(PacketCounter counter, PESPacketPtr& pes) = 0;
    };
}
