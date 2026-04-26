//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  TCP connected socket, for data communication.
//!  Can be used as TCP client (using connect() method).
//!  Can be used by TCP server to receive a client connection.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTCPSocket.h"
#include "tsAbortInterface.h"

namespace ts {
    //!
    //! Base class for a TCP/IP session.
    //! @ingroup libtscore net
    //!
    //! This class can be used directly by applications or can be derived to create specific
    //! sub-classes which handle application protocols.
    //!
    //! This class is used in two contexts:
    //! - A TCP/IP client creates a TCPConnection instance and @e connects to a server.
    //! - A TCP/IP server creates a TCPServer instance and @e waits for clients. For each
    //!   client session, a TCPConnection instance is created.
    //!
    //! For a detailed scenario of the server side, see the class TCPServer.
    //!
    //! The following lists the typical client-side scenario in the correct order.
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
    //! - [*] connect()
    //! - send() / receive()
    //! - closeWriter()
    //! - disconnect()
    //! - close()
    //!
    //! Invoking close() is optional since the destructor of the class will properly
    //! close the socket if not already done. Invoking disconnect() is also optional
    //! but is highly recommended. Closing a socket without prior disconnect is
    //! considered as a session abort by the remote peer. The peer may thus consider
    //! that something went wrong may take unexpected corrective or rollback actions.
    //!
    class TSCOREDLL TCPConnection: public TCPSocket
    {
        TS_NOCOPY(TCPConnection);
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
        explicit TCPConnection(Report* report = nullptr, bool non_blocking = false) : TCPSocket(report, non_blocking) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //!
        explicit TCPConnection(ReporterBase* delegate, bool non_blocking = false) : TCPSocket(delegate, non_blocking) {}

        //!
        //! Connect to a remote address and port.
        //!
        //! Use this method when acting as TCP client.
        //! Do not use on server side: the TCPConnection object is passed
        //! to TCPServer::accept() which establishes the connection.
        //!
        //! @param [in] addr IP address and port of the server to connect.
        //! @return True on success, false on error.
        //!
        virtual bool connect(const IPSocketAddress& addr);

        //!
        //! Check if the socket is connected.
        //! @return True if the socket was successfully connected to the peer.
        //!
        bool isConnected() const { return isOpen() && _is_connected; }

        //!
        //! Get the connected remote peer.
        //! @param [out] addr IP address and port of the remote socket.
        //! @return True on success, false on error.
        //!
        bool getPeer(IPSocketAddress& addr);

        //!
        //! Get the connected remote peer as a string.
        //! @return A string representation of the IP address and port of the remote socket.
        //!
        UString peerName();

        //!
        //! Close the write direction of the connection.
        //!
        //! The application shall call this routine after sending the last
        //! message but may still want to receive messages, waiting for the
        //! peer to voluntary disconnect.
        //!
        //! @param [in] silent If true, do not report errors through the logger. This is typically useful when the socket
        //! is in some error condition and closing it is necessary although it may generate additional meaningless errors.
        //! @return True on success, false on error.
        //!
        virtual bool closeWriter(bool silent = false);

        //!
        //! Disconnect from remote partner.
        //! @param [in] silent If true, do not report errors through the logger. This is typically useful when the socket
        //! is in some error condition and closing it is necessary although it may generate additional meaningless errors.
        //! @return True on success, false on error.
        //!
        virtual bool disconnect(bool silent = false);

        //!
        //! Send data.
        //! @param [in] data Address of the data to send.
        //! @param [in] size Size in bytes of the data to send.
        //! @return True on success, false on error.
        //!
        virtual bool send(const void* data, size_t size);

        //!
        //! Receive data.
        //!
        //! This version of receiveMessage() returns when "some" data are received into
        //! the user buffer. The actual received data may be shorter than the
        //! user buffer size.
        //!
        //! The version is typically useful when the application cannot predict
        //! how much data will be received and must respond even if the user
        //! buffer is not full.
        //!
        //! @param [out] buffer Address of the buffer for the received data.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received data.
        //! Will never be larger than @a max_size.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //!
        virtual bool receive(void* buffer, size_t max_size, size_t& ret_size, const AbortInterface* abort = nullptr);

        //!
        //! Receive data until buffer is full.
        //!
        //! This version of receiveMessage() returns only when sufficient data are
        //! received to completely fill the user buffer. The size of the actual
        //! received data is identical to the user buffer size.
        //!
        //! The version is typically useful when the application knows that
        //! a certain amount of data is expected and must wait for them.
        //!
        //! This base implementation uses the variable-length version of receiveMessage().
        //! Therefore, a subclass may only override the variable-length version.
        //!
        //! @param [out] buffer Address of the buffer for the received data.
        //! @param [in] size Size in bytes of the buffer.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted
        //! (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //!
        virtual bool receive(void* buffer, size_t size, const AbortInterface* abort = nullptr);

    protected:
        //!
        //! This virtual method can be overriden by subclasses to be notified of connection.
        //! All subclasses should explicitly invoke their superclass' handlers.
        //!
        virtual void handleConnected();

        //!
        //! This virtual method can be overriden by subclasses to be notified of disconnection.
        //! All subclasses should explicitly invoke their superclass' handlers.
        //!
        virtual void handleDisconnected();

        // Overriden methods
        virtual void handleClosed() override;

    private:
        volatile bool _is_connected = false;

        // Declare that the socket has just become connected / disconnected.
        void declareConnected();
        void declareDisconnected();
        friend class TCPServer;

        // Shutdown the socket.
        bool shutdownSocket(int how, bool silent);
    };
}
