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
//!  Base class for TCP and UDP sockets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPv4SocketAddress.h"
#include "tsIPUtils.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Base class for TCP and UDP sockets.
    //! @ingroup net
    //!
    class TSDUCKDLL Socket
    {
        TS_NOCOPY(Socket);
    public:
        //!
        //! Constructor.
        //!
        Socket();

        //!
        //! Destructor.
        //!
        virtual ~Socket();

        //!
        //! Open the socket.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        virtual bool open(Report& report = CERR) = 0;

        //!
        //! Close the socket.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        virtual bool close(Report& report = CERR);

        //!
        //! Check if socket is open.
        //! @return True if socket is open.
        //!
        bool isOpen() const { return _sock != SYS_SOCKET_INVALID; }

        //!
        //! Set the send buffer size.
        //! @param [in] size Send buffer size in bytes.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setSendBufferSize(size_t size, Report& report = CERR);

        //!
        //! Set the receive buffer size.
        //! @param [in] size Receive buffer size in bytes.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setReceiveBufferSize(size_t size, Report& report = CERR);

        //!
        //! Set the receive timeout.
        //! @param [in] timeout Receive timeout in milliseconds.
        //! If negative, receive timeout is not used.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setReceiveTimeout(MilliSecond timeout, Report& report = CERR);

        //!
        //! Set the "reuse port" option.
        //! @param [in] reuse_port If true, the socket is allowed to reuse a local
        //! UDP port which is already bound.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool reusePort(bool reuse_port, Report& report = CERR);

        //!
        //! Get local socket address
        //! @param [out] addr Local socket address of the connection.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool getLocalAddress(IPv4SocketAddress& addr, Report& report = CERR);

        //!
        //! Get the underlying socket device handle (use with care).
        //!
        //! This method is reserved for low-level operations and should
        //! not be used by normal applications.
        //!
        //! @return The underlying socket system device handle or file descriptor.
        //! Return SYS_SOCKET_INVALID if the socket is not open.
        //!
        SysSocketType getSocket() const { return _sock; }

    protected:
        //!
        //! Create the socket.
        //! @param [in] domain Socket domain: PF_INET
        //! @param [in] type Socket type: SOCK_STREAM, SOCK_DGRAM
        //! @param [in] protocol Socket protocol: IPPROTO_TCP, IPPROTO_UDP
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool createSocket(int domain, int type, int protocol, Report& report);

        //!
        //! Set an open socket descriptor from a subclass.
        //! This method is used by a server to declare that a client socket has just become opened.
        //! @param [in] sock New socket descriptor.
        //! @param [in,out] report Where to report error.
        //!
        virtual void declareOpened(SysSocketType sock, Report& report);

    private:
        volatile SysSocketType _sock;
    };
}
