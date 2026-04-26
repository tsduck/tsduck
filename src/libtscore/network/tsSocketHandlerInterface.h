//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for notification of open/close on sockets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class Socket;

    //!
    //! Interface class for notification of open/close on sockets.
    //! Interface class for UDP receive operation Reactor handlers.
    //!
    class TSCOREDLL SocketHandlerInterface
    {
        TS_INTERFACE(SocketHandlerInterface);
    public:
        //!
        //! Called at the beginning of a socket open session.
        //! @param [in,out] sock Socket for which the handler is invoked.
        //!
        virtual void handleSocketOpen(Socket& sock) = 0;

        //!
        //! Called at the end of a socket close session.
        //! @param [in,out] sock Socket for which the handler is invoked.
        //!
        virtual void handleSocketClose(Socket& sock) = 0;

    private:
        // The class Socket is allowed to call register/deregister.
        friend class Socket;
        void registerSocket(Socket*);
        void deregisterSocket(Socket*);

        bool _destructing = false;                   // True during destructor.
        std::set<Socket*> _registered_sockets {};    // List of sockets for which we are registered.
    };
}
