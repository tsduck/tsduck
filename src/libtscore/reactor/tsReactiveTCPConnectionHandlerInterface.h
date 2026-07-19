//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for TCP connection Reactor handlers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsObject.h"

namespace ts {

    class ReactiveTCPServer;
    class ReactiveTCPConnection;
    class ReactiveInputControl;

    //!
    //! Interface class for TCP connection Reactor handlers.
    //! All methods are empty by default. An application may implement the required ones only.
    //! @ingroup libtscore reactor
    //!
    class TSCOREDLL ReactiveTCPConnectionHandlerInterface
    {
        TS_INTERFACE(ReactiveTCPConnectionHandlerInterface);
    public:
        //!
        //! Handle the completion of a connection of a TCP socket.
        //! @param [in,out] sock TCP socket for which the handler is invoked.
        //! @param [in] error_code System-specific error code, SYS_SUCCESS on success, SYS_ERROR in case of unknown error,
        //! SYS_CANCELED if the connection was canceled before completion.
        //! @param [in] user_data The user-data shared pointer which was passed to startConnect().
        //!
        virtual void handleTCPConnected(ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle the completion of a client connection of a TCP socket in the context of a server.
        //! @param [in,out] server TCP server in the context of which @a sock is starting a new client session.
        //! @param [in,out] sock TCP socket for which the handler is invoked.
        //! @param [in] error_code System-specific error code, SYS_SUCCESS on success, SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to whenAccepted().
        //!
        virtual void handleTCPAccepted(ReactiveTCPServer& server, ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle the completion of a send operation on a TCP socket.
        //! @param [in,out] sock TCP socket for which the handler is invoked.
        //! @param [in] position The number of sent bytes since the socket was connected, at the end of the
        //! send operation for which the hanlder is called. This is informative only.
        //! @param [in] error_code System-specific error code, SYS_SUCCESS on success, SYS_EOF if the handler is called as a completion
        //! of startCloseWriter(), SYS_ERROR in case of unknown error, SYS_CANCELED if the I/O was canceled before completion.
        //! @param [in] user_data The user-data shared pointer which was passed to startSend().
        //!
        virtual void handleTCPSend(ReactiveTCPConnection& sock, size_t position, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle the reception of TCP data.
        //! @param [in,out] sock TCP socket for which the handler is invoked.
        //! @param [in] data Received data.
        //! @param [in,out] control Input control data that the handler may update according to the usage of @a data.
        //! @param [in] error_code System-specific error code, SYS_SUCCESS on success, SYS_EOF if the peer has disconnected,
        //! SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to startReceive().
        //!
        virtual void handleTCPReceive(ReactiveTCPConnection& sock, const ByteBlock& data, ReactiveInputControl& control, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle the completion of closing a TCP socket.
        //! An application closes a ReactiveTCPConnection using startClose() and this handler is called when the closing
        //! of the socket is complete. This is specifically important on operating systems with asynchronous I/O such
        //! as Windows. The application shall not call TCPSocket::close() and immediately consider the socket as done.
        //! If there are pending asynchronous I/O, the associated data buffers are still in use, until the cancelation
        //! of the these I/O are completed, after closing the socket. The application shall therefore wait for the
        //! handleTCPClosed() handler to destroy the data buffers and consider the socket as completely done.
        //! @param [in,out] sock TCP socket for which the handler is invoked.
        //! @param [in] user_data The user-data shared pointer which was passed to startClose().
        //!
        virtual void handleTCPClosed(ReactiveTCPConnection& sock, const ObjectPtr& user_data);
    };
}
