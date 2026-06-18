//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for TCP server Reactor handlers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"

namespace ts {

    class IPSocketAddress;
    class ReactiveTCPConnection;
    class ReactiveTCPServer;

    //!
    //! Interface class for TCP server Reactor handlers.
    //! All methods are empty by default. An application may implement the required ones only.
    //!
    class TSCOREDLL ReactiveTCPServerHandlerInterface
    {
        TS_INTERFACE(ReactiveTCPServerHandlerInterface);
    public:
        //!
        //! Handle the reception of TCP data.
        //! @param [in,out] server TCP server socket for which the handler is invoked.
        //! @param [in,out] sock TCP connection socket of the newly accepted client.
        //! @param [in] addr Socket address of the client.
        //! @param [in] error_code System-specific error code, SYS_SUCCESS on success, SYS_EOF if the peer has disconnected,
        //! SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to startAccept().
        //!
        virtual void handleTCPClientAccepted(ReactiveTCPServer& server, ReactiveTCPConnection& sock, const IPSocketAddress& addr, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle the completion of closing a TCP server socket.
        //! An application closes a ReactiveTCPServer using startClose() and this handler is called when the closing
        //! of the socket is complete. This is specifically important on operating systems with asynchronous I/O such
        //! as Windows. The application shall not call TCPSocket::close() and immediately consider the socket as done.
        //! If there are pending asynchronous I/O, the associated data buffers are still in use, until the cancelation
        //! of the these I/O are completed, after closing the socket. The application shall therefore wait for the
        //! handleTCPClosed() handler to destroy the data buffers and consider the socket as completely done.
        //! @param [in,out] server TCP server socket for which the handler is invoked.
        //! @param [in] user_data The user-data shared pointer which was passed to startClose().
        //!
        virtual void handleTCPServerClosed(ReactiveTCPServer& server, const ObjectPtr& user_data);
    };
}
