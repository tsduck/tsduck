//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_INTERFACE(T2MIHandlerInterface);
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
    };
}
