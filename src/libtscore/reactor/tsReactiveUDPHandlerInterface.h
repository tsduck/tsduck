//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  All interface classes which are used as UDP event handlers in a Reactor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsUDPSocket.h"

namespace ts {

    class ReactiveUDPSocket;

    //!
    //! Interface class for UDP operation Reactor handlers.
    //! All methods are empty by default. An application may implement the required ones only.
    //! @ingroup libtscore reactor
    //!
    class TSCOREDLL ReactiveUDPHandlerInterface
    {
        TS_INTERFACE(ReactiveUDPHandlerInterface);
    public:
        //!
        //! Handle the completion of a send operation on a UDP socket.
        //! @param [in,out] sock UDP socket for which the handler is invoked.
        //! @param [in] data Address of the message which was sent. For information only.
        //! @param [in] size Size in bytes of the sent message. For information only.
        //! @param [in] destination Socket address of the destination. For information only.
        //! @param [in] error_code System-specific error code, SYS_SUCCESS on success, SYS_ERROR in case of unknown error,
        //! SYS_CANCELED if the I/O was canceled before completion.
        //! @param [in] user_data The user-data shared pointer which was passed to startSend().
        //!
        virtual void handleUDPSend(ReactiveUDPSocket& sock, const void* data, size_t size, const IPSocketAddress& destination, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle the reception of a UDP datagram.
        //! @param [in,out] sock UDP socket for which the handler is invoked.
        //! @param [in] data Safe pointer to the received data packet.
        //! @param [in] sender Socket address of the sender.
        //! @param [in] destination Socket address of the packet destination. Useful in multicast packets.
        //! @param [in] timestamp Receive timestamp in micro-seconds. If the value is negative, no timestamp is available.
        //! @param [in] timestamp_type Type of receive timestamp.
        //! @param [in] error_code System-specific error code, SYS_SUCCESS on success, SYS_ERROR in case of unknown error.
        //! @param [in] user_data The user-data shared pointer which was passed to startReceive().
        //!
        virtual void handleUDPReceive(ReactiveUDPSocket& sock,
                                      const ByteBlockPtr& data,
                                      const IPSocketAddress& sender,
                                      const IPSocketAddress& destination,
                                      cn::microseconds timestamp,
                                      UDPSocket::TimeStampType timestamp_type,
                                      int error_code,
                                      const ObjectPtr& user_data);

        //!
        //! Handle the completion of closing a UDP socket.
        //! An application closes a ReactiveUDPSocket using startClose() and this handler is called when the closing
        //! of the socket is complete. This is specifically important on operating systems with asynchronous I/O such
        //! as Windows. The application shall not call UDPSocket::close() and immediately consider the socket as done.
        //! If there are pending asynchronous I/O, the associated data buffers are still in use, until the cancelation
        //! of the these I/O are completed, after closing the socket. The application shall therefore wait for the
        //! handleUDPClosed() handler to destroy the data buffers and consider the socket as completely done.
        //! @param [in,out] sock UDP socket for which the handler is invoked.
        //! @param [in] user_data The user-data shared pointer which was passed to startClose().
        //!
        virtual void handleUDPClosed(ReactiveUDPSocket& sock, const ObjectPtr& user_data);
    };
}
