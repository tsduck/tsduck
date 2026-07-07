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
    class SocketSubscriptionBase;
    class TCPConnection;

    //!
    //! Interface class for notification of open/close on sockets.
    //! All methods are empty by default. An application may implement the required ones only.
    //!
    class TSCOREDLL SocketHandlerInterface
    {
        TS_INTERFACE(SocketHandlerInterface);
    public:
        //!
        //! Called when a socket is about to be opened.
        //! Never called when the application tries to open a socket which is already open.
        //! @param [in,out] sock Socket for which the handler is invoked.
        //!
        virtual void handleSocketOpenStart(Socket& sock);

        //!
        //! Called after a socket is opened (or failed to open).
        //! @param [in,out] sock Socket for which the handler is invoked.
        //! @param [in] success True if the open was successful, false if the open failed.
        //! In case of error, the socket shall be considered as closed.
        //!
        virtual void handleSocketOpenComplete(Socket& sock, bool success);

        //!
        //! Called when a TCP socket is successfully connected.
        //! @param [in,out] sock Socket for which the handler is invoked.
        //!
        virtual void handleSocketConnected(TCPConnection& sock);

        //!
        //! Called when a TCP socket is disconnected.
        //! @param [in,out] sock Socket for which the handler is invoked.
        //! @param [in] silent Same value as passed to disconnect() or close().
        //!
        virtual void handleSocketDisconnected(TCPConnection& sock, bool silent);

        //!
        //! Called when a socket is about to be closed.
        //! Never called when the application tries to close a socket which is not open.
        //! @param [in,out] sock Socket for which the handler is invoked.
        //! @param [in] silent Same value as passed to close().
        //!
        virtual void handleSocketCloseStart(Socket& sock, bool silent);

        //!
        //! Called after a socket is closed.
        //! @param [in,out] sock Socket for which the handler is invoked.
        //! @param [in] silent Same value as passed to close().
        //! @param [in] success True if the close was successful, false if there was an error during the close.
        //! The socket shall be considered as closed on all cases, regardless of @a success.
        //!
        virtual void handleSocketCloseComplete(Socket& sock, bool silent, bool success);

    private:
        // The class Socket is allowed to call register/deregister.
        friend class SocketSubscriptionBase;
        void registerSocket(SocketSubscriptionBase*);
        void deregisterSocket(SocketSubscriptionBase*);

        bool _destructing = false;                   // True during destructor.
        std::set<SocketSubscriptionBase*> _registered_sockets {};    // List of sockets for which we are registered.
    };
}
