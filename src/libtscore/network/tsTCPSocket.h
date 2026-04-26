//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  TCP Socket
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSocket.h"
#include "tsIPSocketAddress.h"

namespace ts {
    //!
    //! Base class for TCP/IP sockets.
    //! @ingroup libtscore net
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
    class TSCOREDLL TCPSocket: public Socket
    {
        TS_NOBUILD_NOCOPY(TCPSocket);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //!
        explicit TCPSocket(Report* report, bool non_blocking = false) : Socket(report, non_blocking) {}

        //!
        //! Destructor.
        //!
        virtual ~TCPSocket() override;

        //!
        //! Set the Time To Live (TTL) option.
        //! @param [in] ttl The TTL value, ie. the maximum number of "hops" between routers before an IP packet is dropped.
        //! @return True on success, false on error.
        //!
        bool setTTL(int ttl);

        //!
        //! Remove the linger time option.
        //! @return True on success, false on error.
        //!
        bool setNoLinger();

        //!
        //! Set the linger time option.
        //! @param [in] seconds Number of seconds to wait after shuting down the socket.
        //! @return True on success, false on error.
        //!
        bool setLingerTime(int seconds);

        //!
        //! Set the "keep alive" option.
        //! @param [in] active If true, the socket periodically sends "keep alive"
        //! packets when the connection is idle.
        //! @return True on success, false on error.
        //!
        bool setKeepAlive(bool active);

        //!
        //! Set the "no delay" option.
        //! @param [in] active If true, the socket immediately sends outgoing packets.
        //! By default, a TCP socket waits a small amount of time after a send()
        //! operation to get a chance to group outgoing data from successive send()
        //! operations into one single packet.
        //! @return True on success, false on error.
        //!
        bool setNoDelay(bool active);

        //!
        //! Bind to a local address and port.
        //!
        //! The IP address part of the socket address must one of:
        //! - IPAddress::AnyAddress4. Any local interface may be used to connect to a server
        //!   (client side) or to receive incoming client connections (server side).
        //! - The IP address of an interface of the local system. Outgoing connections
        //!   (client side) will be only allowed through this interface. Incoming client
        //!   connections (server side) will be accepted only when they arrive through
        //!   the selected interface.
        //!
        //! The port number part of the socket address must be one of:
        //! - IPSocketAddress::AnyPort. The socket is bound to an arbitrary unused
        //!   local TCP port. This is the usual configuration for a TCP client.
        //! - A specific port number. This is the usual configuration for a TCP server.
        //!   If this TCP port is already bound by another local TCP socket, the bind
        //!   operation fails, unless the "reuse port" option has already been set.
        //!
        //! @param [in] addr Local socket address to bind to.
        //! @return True on success, false on error.
        //!
        bool bind(const IPSocketAddress& addr);

        // Implementation of Socket interface.
        virtual bool open(IP gen) override;
        virtual bool close(bool silent = false) override;

    protected:
        std::recursive_mutex _mutex {}; //!< Mutex protecting this object.

        //!
        //! This virtual method can be overriden by subclasses to be notified of open.
        //! All subclasses should explicitly invoke their superclass' handlers.
        //!
        virtual void handleOpened();

        //!
        //! This virtual method can be overriden by subclasses to be notified of close.
        //! All subclasses should explicitly invoke their superclass' handlers.
        //!
        virtual void handleClosed();

        // Implementation of Socket interface.
        virtual void declareOpened(SysSocketType sock) override;
    };
}
