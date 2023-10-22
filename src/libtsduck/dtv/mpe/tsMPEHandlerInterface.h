//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_INTERFACE(MPEHandlerInterface);
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
    };
}
