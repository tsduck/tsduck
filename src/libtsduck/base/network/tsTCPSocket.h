//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  TCP Socket
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSocket.h"
#include "tsIPv4SocketAddress.h"
#include "tsAbortInterface.h"
#include "tsReport.h"
#include "tsNullReport.h"
#include "tsSafePtr.h"
#include "tsNullMutex.h"
#include "tsMutex.h"

namespace ts {
    //!
    //! Base class for TCP/IP sockets.
    //! @ingroup net
    //!
    //! This base class is not supposed to be directly instantiated.
    //! The two concrete subclasses of TCPSocket are:
    //! - TCPServer: A TCP/IP server socket which listens to incoming connections.
    //!   This type is socket is not designed to exchange data.
    //! - TCPConnection: A TCP/IP session between a client and a server. This
    //!   socket can exchange data.
    //!   - A TCP/IP client creates a TCPConnection instance and @e connects to a server.
    //!   - A TCP/IP server creates a TCPServer instance and @e waits for clients. For each
    //!     client session, a TCPConnection instance is created.
    //!
    class TSDUCKDLL TCPSocket: public Socket
    {
        TS_NOCOPY(TCPSocket);
    public:
        //!
        //! Constructor.
        //!
        TCPSocket();

        //!
        //! Destructor.
        //!
        virtual ~TCPSocket() override;

        //!
        //! Set the Time To Live (TTL) option.
        //! @param [in] ttl The TTL value, ie. the maximum number of "hops" between
        //! routers before an IP packet is dropped.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setTTL(int ttl, Report& report = CERR);

        //!
        //! Remove the linger time option.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setNoLinger(Report& report = CERR);

        //!
        //! Set the linger time option.
        //! @param [in] seconds Number of seconds to wait after shuting down the socket.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setLingerTime(int seconds, Report& report = CERR);

        //!
        //! Set the "keep alive" option.
        //! @param [in] active If true, the socket periodically sends "keep alive"
        //! packets when the connection is idle.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setKeepAlive(bool active, Report& report = CERR);

        //!
        //! Set the "no delay" option.
        //! @param [in] active If true, the socket immediately sends outgoing packets.
        //! By default, a TCP socket waits a small amount of time after a send()
        //! operation to get a chance to group outgoing data from successive send()
        //! operations into one single packet.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setNoDelay(bool active, Report& report = CERR);

        //!
        //! Bind to a local address and port.
        //!
        //! The IP address part of the socket address must one of:
        //! - IPv4Address::AnyAddress. Any local interface may be used to connect to a server
        //!   (client side) or to receive incoming client connections (server side).
        //! - The IP address of an interface of the local system. Outgoing connections
        //!   (client side) will be only allowed through this interface. Incoming client
        //!   connections (server side) will be accepted only when they arrive through
        //!   the selected interface.
        //!
        //! The port number part of the socket address must be one of:
        //! - IPv4SocketAddress::AnyPort. The socket is bound to an arbitrary unused
        //!   local TCP port. This is the usual configuration for a TCP client.
        //! - A specific port number. This is the usual configuration for a TCP server.
        //!   If this TCP port is already bound by another local TCP socket, the bind
        //!   operation fails, unless the "reuse port" option has already been set.
        //!
        //! @param [in] addr Local socket address to bind to.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool bind(const IPv4SocketAddress& addr, Report& report = CERR);

        // Implementation of Socket interface.
        virtual bool open(Report& report = CERR) override;
        virtual bool close(Report& report = CERR) override;

    protected:
        Mutex _mutex; //!< Mutex protecting this object.

        //!
        //! This virtual method can be overriden by subclasses to be notified of open.
        //! All subclasses should explicitly invoke their superclass' handlers.
        //! @param [in,out] report Where to report error.
        //!
        virtual void handleOpened(Report& report);

        //!
        //! This virtual method can be overriden by subclasses to be notified of close.
        //! All subclasses should explicitly invoke their superclass' handlers.
        //! @param [in,out] report Where to report error.
        //!
        virtual void handleClosed(Report& report);

        // Implementation of Socket interface.
        virtual void declareOpened(SysSocketType sock, Report& report) override;
    };

    //!
    //! Safe pointer to TCPSocket, single-threaded.
    //!
    typedef SafePtr <TCPSocket, NullMutex> TCPSocketPtr;

    //!
    //! Safe pointer to TCPSocket, multi-threaded.
    //!
    typedef SafePtr <TCPSocket, Mutex> TCPSocketPtrMT;
}
