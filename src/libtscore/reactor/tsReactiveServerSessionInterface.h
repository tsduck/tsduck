//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for server-side client sessions in ReactiveServer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveTCPConnection.h"

namespace ts {
    //!
    //! This interface shall be implemented by all server-side client sessions used by ReactiveServer.
    //! @ingroup libtscore reactor
    //!
    //! A session object is created each time a new incoming client is accepted. This object shall
    //! contain a ReactiveTCPConnection object (or from a subclass of it). This interface is defined
    //! to access this internal ReactiveTCPConnection. Accessing this ReactiveTCPConnection is required
    //! by ReactiveServer to initialize the incoming connection.
    //!
    //! Each client session is created by an instance of ReactiveServerFactoryInterface.
    //!
    class TSCOREDLL ReactiveServerSessionInterface
    {
        TS_INTERFACE(ReactiveServerSessionInterface);
    public:
        //!
        //! Get the associated ReactiveTCPConnection.
        //! @return A reference to the associated ReactiveTCPConnection object.
        //!
        virtual ReactiveTCPConnection& getConnection() = 0;
    };
}
