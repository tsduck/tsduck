//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generic datagram handler interface.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class Report;

    //!
    //! Generic datagram handler interface.
    //! @ingroup mpeg
    //!
    //! This abstract interface must be implemented by classes which send datagrams of TS packets.
    //!
    class TSDUCKDLL TSDatagramOutputHandlerInterface
    {
        TS_INTERFACE(TSDatagramOutputHandlerInterface);
    public:
        //!
        //! Send a datagram message.
        //! Must be implemented by classes which are in charge of sending datagrams.
        //! @param [in] address Address of datagram.
        //! @param [in] size Size in bytes of datagram.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        virtual bool sendDatagram(const void* address, size_t size, Report& report) = 0;
    };
}
