//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for TCP and UDP sockets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNonBlockingDevice.h"
#include "tsSocketHandlerInterface.h"
#include "tsIPSocketAddress.h"
#include "tsIPUtils.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Base class for TCP and UDP sockets.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL Socket : public NonBlockingDevice
    {
        TS_NOCOPY(Socket);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //!
        explicit Socket(Report* report = nullptr, bool non_blocking = false);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //!
        explicit Socket(ReporterBase* delegate, bool non_blocking = false);

        //!
        //! Destructor.
        //!
        virtual ~Socket() override;

        //!
        //! Open the socket.
        //! @param [in] gen IP generation, IPv4 or IPv6. If set to IP::Any, open an IPv6 socket (IPv4 connections allowed).
        //! @return True on success, false on error.
        //!
        virtual bool open(IP gen) = 0;

        //!
        //! Close the socket.
        //! If overridden by a subclass, the superclass must be called at the end of the overridden close().
        //! @param [in] silent If true, do not report errors through the logger. This is typically useful when the socket
        //! is in some error condition and closing it is necessary although it may generate additional meaningless errors.
        //! @return True on success, false on error.
        //!
        virtual bool close(bool silent = false);

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
        //! @return True on success, false on error.
        //!
        bool setSendBufferSize(size_t size);

        //!
        //! Set the receive buffer size.
        //! @param [in] size Receive buffer size in bytes.
        //! @return True on success, false on error.
        //!
        bool setReceiveBufferSize(size_t size);

        //!
        //! Set the receive timeout.
        //! @param [in] timeout Receive timeout in milliseconds.
        //! If negative or zero, receive timeout is not used, reception waits forever.
        //! @return True on success, false on error.
        //!
        bool setReceiveTimeout(cn::milliseconds timeout);

        //!
        //! Set the "reuse port" option.
        //! @param [in] reuse_port If true, the socket is allowed to reuse a local
        //! UDP port which is already bound.
        //! @return True on success, false on error.
        //!
        bool reusePort(bool reuse_port);

        //!
        //! Get local socket address
        //! @param [out] addr Local socket address of the connection.
        //! @return True on success, false on error.
        //!
        bool getLocalAddress(IPSocketAddress& addr) const;

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

        //!
        //! Add a subscriber to open/close events.
        //! @param [in] handler The object to call on open() and close().
        //!
        void addSubscription(SocketHandlerInterface* handler);

        //!
        //! Remove a subscriber to open/close events.
        //! @param [in] handler The object to no longer call on open() and close().
        //!
        void cancelSubscription(SocketHandlerInterface* handler);

    protected:
        //!
        //! Create the socket.
        //! @param [in] gen IP generation.
        //! @param [in] type Socket type: SOCK_STREAM, SOCK_DGRAM
        //! @param [in] protocol Socket protocol: IPPROTO_TCP, IPPROTO_UDP
        //! @return True on success, false on error.
        //! @see open(ge, Report&)
        //!
        bool createSocket(IP gen, int type, int protocol);

        //!
        //! Set an open socket descriptor from a subclass.
        //! This method is used by a server to declare that a client socket has just become opened.
        //! @param [in] sock New socket descriptor.
        //!
        virtual void declareOpened(SysSocketType sock);

        //!
        //! Convert an IP address to make it compatible with the socket IP generation.
        //! @param addr [in,out] The address to convert.
        //! @return True on success, false on error.
        //!
        bool convert(IPAddress& addr) const;

        // Overloaded methods.
        virtual bool allowSetNonBlocking() const override;

    private:
        volatile SysSocketType            _sock = SYS_SOCKET_INVALID;
        IP                                _gen = IP::v4;    // Current generation of the IP address. Never IP::Any.
        std::set<SocketHandlerInterface*> _subscribers {};  // Subscribers to open/close notifications.
    };
}
