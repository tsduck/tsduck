//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface to create server-side client sessions used by ReactiveServer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveServerSessionInterface.h"

namespace ts {
    //!
    //! This interface creates server-side client sessions used by ReactiveServer.
    //!
    //! Each time a new incoming client is accepted by a ReactiveServer, the server uses
    //! an instance of ReactiveServerFactoryInterface to create the object which drives
    //! the session.
    //!
    //! Each new instance of the ReactiveServerSessionInterface must be allocated using the
    //! "new" operator. The ownership of the allocated object is transfered to the ReactiveServer.
    //! The object will be automatically deleted by the ReactiveServer when the session is closed.
    //! The method handleTCPClosed() on the closed session is the last time the ReactiveServerSessionInterface
    //! object can be used. All additional cleanup must be done in its virtual destructor.
    //!
    class TSCOREDLL ReactiveServerFactoryInterface
    {
        TS_INTERFACE(ReactiveServerFactoryInterface);
    public:
        //!
        //! Create a new ReactiveServerSessionInterface.
        //! @return Pointer to a new instance of ReactiveServerSessionInterface. The ownership
        //! of the allocated object is transfered to the server. The object will be automatically
        //! deleted by the ReactiveServer.
        //!
        virtual ReactiveServerSessionInterface* newClientSession() = 0;
    };
}
