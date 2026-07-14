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
        TS_NOBUILD_NOCOPY(TCPConnection);
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
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TCPConnection(Report* report, bool non_blocking = false, Object* owner = nullptr) : TCPSocket(report, non_blocking, owner) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TCPConnection(ReporterBase* delegate, bool non_blocking = false, Object* owner = nullptr) : TCPSocket(delegate, non_blocking, owner) {}

        //!
        //! Connect to a remote address and port.
        //!
        //! Use this method when acting as TCP client.
        //! Do not use on server side: the TCPConnection object is passed
        //! to TCPServer::accept() which establishes the connection.
        //!
        //! @param [in] addr IP address and port of the server to connect to.
        //! @param [in,out] iosb Address of an IOSB structure. If non-null, the socket must be in non-blocking mode.
        //! When null, the socket must be in blocking mode (the default). See the description of IOSB.
        //! Important: The parameter @a iosb should not be used by applications. It should be used only by
        //! "reactive classes", which work in combination with a Reactor.
        //! @return True on success, false on error. In case of non-blocking mode, if the connection is successfully started
        //! but still pending, iosb->pending is set to true and the method returns true.
        //!
        virtual bool connect(const IPSocketAddress& addr, IOSB* iosb = nullptr);

        //!
        //! Update the status of an asynchronous connect().
        //! - With non-blocking I/O, @a error_code shall be the error from the write-ready notification.
        //! - With asynchronous I/O, @a iosb is used to complete the socket state.
        //! @param [in,out] iosb Address of the IOSB structure which was used when connect() was called.
        //! @param [in] error_code Error code in the context of the connection.
        //! @return True on success, false on error.
        //!
        bool setConnectStatus(IOSB* iosb, int error_code);

        //!
        //! Check if the socket is connected.
        //! @return True if the socket was successfully connected to the peer.
        //!
        bool isConnected() const { return isOpen() && _is_connected; }

        //!
        //! Check if the socket is the server-side end of a connection.
        //! @return True if the socket is the server-side end of a connection, where the connection is the result of an
        //! accept(). False if the socket is the client-side end of a connection, where the connection used connect().
        //!
        bool isServerSide() const { return _is_server_side; }

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
        //! @param [in,out] iosb Address of an IOSB structure. If non-null, the socket must be in non-blocking mode.
        //! When null, the socket must be in blocking mode (the default). See the description of IOSB.
        //! Important: The parameter @a iosb should not be used by applications. It should be used only by
        //! "reactive classes", which work in combination with a Reactor.
        //! @return True on success, false on error. In case of non-blocking mode, if the I/O is successfully started
        //! but still pending, iosb->pending is set to true and the method returns true.
        //!
        virtual bool send(const void* data, size_t size, IOSB* iosb = nullptr);

        //!
        //! Receive data.
        //!
        //! This version of receiveMessage() returns when "some" data are received into the user buffer.
        //! The actual received data may be shorter than the user buffer size.
        //!
        //! The version is typically useful when the application cannot predict how much data will be
        //! received and must respond even if the user buffer is not full.
        //!
        //! @param [out] buffer Address of the buffer for the received data.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received data. Will never be larger than @a max_size.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted (in case of user-interrupt, return, otherwise retry).
        //! @param [in,out] iosb Address of an IOSB structure. If non-null, the socket must be in non-blocking mode.
        //! When null, the socket must be in blocking mode (the default). See the description of IOSB.
        //! Important: The parameter @a iosb should not be used by applications. It should be used only by
        //! "reactive classes", which work in combination with a Reactor.
        //! @return True on success, false on error. In case of non-blocking mode, if the I/O is successfully started
        //! but still pending, iosb->pending is set to true and the method returns true.
        //!
        virtual bool receive(void* buffer, size_t max_size, size_t& ret_size, const AbortInterface* abort = nullptr, IOSB* iosb = nullptr);

        //!
        //! Receive data until buffer is full.
        //!
        //! This version of receive() returns only when sufficient data are received to completely fill the user buffer.
        //! The size of the actual received data is identical to the user buffer size. If some data, but not all, were
        //! received before the connection was closed, these data are ignored and the method returns false. The version
        //! is typically useful when the application knows that a certain amount of data is expected and must wait for them.
        //!
        //! This method is only allowed when the socket is in blocking-mode (the default) because this method is blocking
        //! by definition. Therefore, there is no @a iosb parameter.
        //!
        //! This base implementation uses the variable-length version of receive(). Therefore, a subclass may only override
        //! the variable-length version and not this one.
        //!
        //! @param [out] buffer Address of the buffer for the received data.
        //! @param [in] size Size in bytes of the buffer.
        //! @param [in] abort If non-zero, invoked when I/O is interrupted (in case of user-interrupt, return, otherwise retry).
        //! @return True on success, false on error.
        //!
        virtual bool receive(void* buffer, size_t size, const AbortInterface* abort = nullptr);

    private:
        volatile bool _is_connected = false;
        volatile bool _is_server_side = false;

        // Implementation of Socket interface.
        virtual void declareOpened(SysSocketType sock) override;
        virtual bool closeImplementation(bool silent) override;

        // Declare that the socket has just become connected / disconnected.
        void declareConnected();
        void declareDisconnected(bool silent);
        friend class TCPServer;

        // Shutdown the socket.
        bool shutdownSocket(int how, bool silent);

#if defined(TS_WINDOWS)
        // For Windows asynchronous I/O, we need to keep parameter in one single structure which lives during the I/O.
        class TSCOREDLL AsyncBuffers: public Object
        {
            TS_NOBUILD_NOCOPY(AsyncBuffers);
        public:
            SocketOp type;                    // Operation type, for debug purpose.
            ::WSABUF buf {};                  // Pointer to user's buffer (send/receive).
            ::DWORD flags = 0;                // WSARecv flags.
            ::sockaddr_storage peer_sock {};  // Peer socket (connect).
            int peer_sock_len = 0;            // Actual length of peer_socket.

            // Constructor and destructor.
            AsyncBuffers(SocketOp op) : type(op) {}
            virtual ~AsyncBuffers() override;
        };
#endif
    };
}
