//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  TCP Server
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTCPConnection.h"

namespace ts {
    //!
    //! Implementation of a TCP/IP server.
    //! @ingroup net
    //!
    //! The following lists the typical server-side scenario in the correct order.
    //! Many steps such as setting socket options are optional. The symbol [*] means mandatory.
    //! Depending on the platform, some options settings are sensitive to the order.
    //! The following order has proven to work on most platforms.
    //!
    //! - [*] open()
    //! - reusePort()
    //! - setSendBufferSize()
    //! - setReceiveBufferSize()
    //! - setLingerTime() / setNoLinger()
    //! - setKeepAlive()
    //! - setNoDelay()
    //! - setTTL()
    //! - [*] bind()
    //! - [*] listen()
    //! - [*] accept()
    //! - close()
    //!
    //! Invoking close() is optional since the destructor of the class will properly
    //! close the socket if not already done.
    //!
    class TSDUCKDLL TCPServer: public TCPSocket
    {
        TS_NOCOPY(TCPServer);
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef TCPSocket SuperClass;

        //!
        //! Constructor
        //!
        TCPServer() = default;

        //!
        //! Start the server.
        //!
        //! Here, @e starting the server means starting to listen to incoming
        //! client connections. Internally to the kernel, the incoming connections
        //! are queued up to @a backlog. When the method accept() is invoked and
        //! some incoming connections are already queued in the kernel, the oldest
        //! one is immediately accepted. Otherwise, accept() blocks until a new
        //! incoming connection arrives.
        //!
        //! @param [in] backlog Maximum number of incoming connections which allowed
        //! to queue in the kernel until the next call to accept(). Note that this
        //! value is a minimum queue size. But the kernel may accept more. There is
        //! no guarantee that additional incoming connections will be rejected if more
        //! than @a backlog are already queueing.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool listen(int backlog, Report& report = CERR);

        //!
        //! Wait for an incoming client connection.
        //!
        //! @param [out] client This object receives the new connection. Upon successful
        //! return from accept(), the TCPConnection object is a properly connected TCP
        //! session. Once the connection is completed, the TCPConnection objects on the
        //! client side and the server side are symmetric and can be used the same way.
        //! @param [out] addr This object receives the socket address of the client.
        //! If the server wants to filter client connections based on their IP address,
        //! it may use @a addr for that.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //! @see listen()
        //!
        bool accept(TCPConnection& client, IPv4SocketAddress& addr, Report& report = CERR);

        // Inherited and overridden
        virtual bool close(Report& report = CERR) override;
    };
}
