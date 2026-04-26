//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
    //! @ingroup libtscore net
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
    class TSCOREDLL TCPServer: public TCPSocket
    {
        TS_NOBUILD_NOCOPY(TCPServer);
    public:
        //!
        //! Reference to the superclass.
        //!
        using SuperClass = TCPSocket;

        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //!
        explicit TCPServer(Report* report, bool non_blocking = false) : TCPSocket(report, non_blocking) {}

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
        //! @return True on success, false on error.
        //!
        virtual bool listen(int backlog);

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
        //! @return True on success, false on error.
        //! @see listen()
        //!
        virtual bool accept(TCPConnection& client, IPSocketAddress& addr);

        // Inherited methods.
        virtual bool close(bool silent = false) override;
    };
}
