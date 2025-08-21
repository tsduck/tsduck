//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for TCP and UDP sockets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPSocketAddress.h"
#include "tsIPUtils.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Base class for TCP and UDP sockets.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL Socket
    {
        TS_NOCOPY(Socket);
    public:
        //!
        //! Constructor.
        //!
        Socket() = default;

        //!
        //! Destructor.
        //!
        virtual ~Socket();

        //!
        //! Open the socket.
        //! @param [in] gen IP generation, IPv4 or IPv6. If set to IP::Any, open an IPv6 socket (IPv4 connections allowed).
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        virtual bool open(IP gen, Report& report = CERR) = 0;

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
        //! Get the IP generation with which the socket was open.
        //! @return The IP generation used to open the socket. Never IP::Any.
        //!
        IP generation() const { return _gen; }

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
        //! If negative or zero, receive timeout is not used, reception waits forever.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setReceiveTimeout(cn::milliseconds timeout, Report& report = CERR);

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
        bool getLocalAddress(IPSocketAddress& addr, Report& report = CERR);

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
        //! @param [in] gen IP generation.
        //! @param [in] type Socket type: SOCK_STREAM, SOCK_DGRAM
        //! @param [in] protocol Socket protocol: IPPROTO_TCP, IPPROTO_UDP
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //! @see open(ge, Report&)
        //!
        bool createSocket(IP gen, int type, int protocol, Report& report);

        //!
        //! Set an open socket descriptor from a subclass.
        //! This method is used by a server to declare that a client socket has just become opened.
        //! @param [in] sock New socket descriptor.
        //! @param [in,out] report Where to report error.
        //!
        virtual void declareOpened(SysSocketType sock, Report& report);

        //!
        //! Convert an IP address to make it compatible with the socket IP generation.
        //! @param addr [in,out] The address to convert.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool convert(IPAddress& addr, Report& report) const;

    private:
        volatile SysSocketType _sock = SYS_SOCKET_INVALID;
        IP _gen = IP::v4;   // Current generation of the IP address. Never IP::Any.
    };
}
