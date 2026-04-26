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
    //! Interface class for UDP receive operation Reactor handlers.
    //!
    class TSCOREDLL ReactiveUDPReceiveHandlerInterface
    {
        TS_INTERFACE(ReactiveUDPReceiveHandlerInterface);
    public:
        //!
        //! Handle the reception of a UDP datagram.
        //! @param [in,out] sock UDP socket for which the handler is invoked.
        //! @param [in] data Safe pointer to the received data packet.
        //! @param [in] sender Socket address of the sender.
        //! @param [in] destination Socket address of the packet destination. Useful in multicast packets.
        //! @param [in] timestamp Receive timestamp in micro-seconds. If the value is negative, no timestamp is available.
        //! @param [in] timestamp_type Type of receive timestamp.
        //! @param [in] error_code System-specific error code, zero on success, -1 in case of unknown error.
        //!
        virtual void handleUDPReceive(ReactiveUDPSocket& sock,
                                      const ByteBlockPtr& data,
                                      const IPSocketAddress& sender,
                                      const IPSocketAddress& destination,
                                      cn::microseconds timestamp,
                                      UDPSocket::TimeStampType timestamp_type,
                                      int error_code) = 0;
    };

    //!
    //! Interface class for UDP send operation Reactor handlers.
    //!
    class TSCOREDLL ReactiveUDPSendHandlerInterface
    {
        TS_INTERFACE(ReactiveUDPSendHandlerInterface);
    public:
        //!
        //! Handle the completion of a send operation on a UDP socket.
        //! @param [in,out] sock UDP socket for which the handler is invoked.
        //! @param [in] data Address of the message which was sent. For information only.
        //! @param [in] size Size in bytes of the sent message. For information only.
        //! @param [in] destination Socket address of the destination. For information only.
        //! @param [in] error_code System-specific error code, zero on success, -1 in case of unknown error.
        //!
        virtual void handleUDPSend(ReactiveUDPSocket& sock, const void* data, size_t size, const IPSocketAddress& destination, int error_code) = 0;
    };
}
