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
        TCPServer():
            TCPSocket()
        {
        }

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
