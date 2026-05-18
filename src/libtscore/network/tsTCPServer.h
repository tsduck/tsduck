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
        TS_NOCOPY(TCPServer);
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
        explicit TCPServer(Report* report = nullptr, bool non_blocking = false, Object* owner = nullptr) : TCPSocket(report, non_blocking, owner) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TCPServer(ReporterBase* delegate, bool non_blocking = false, Object* owner = nullptr) : TCPSocket(delegate, non_blocking, owner) {}

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
        //! @param [out] client This object receives the new connection. Upon successful return from accept(),
        //! the TCPConnection object is a properly connected TCP session. Once the connection is completed, the
        //! TCPConnection objects on the client side and the server side are symmetric and can be used the same way.
        //! In case of asynchronous I/O, the object must remain valid until the accept() completes or is canceled.
        //! @param [out] client_address This object receives the socket address of the client. If the server wants to filter
        //! client connections based on their IP address, it may use @a addr for that.
        //! @param [in,out] iosb Address of an IOSB structure. If non-null, the socket must be in non-blocking mode.
        //! When null, the socket must be in blocking mode (the default). See the description of IOSB.
        //! Important: The parameter @a iosb should not be used by applications. It should be used only by
        //! "reactive classes", which work in combination with a Reactor.
        //! @return True on success, false on error. In case of non-blocking mode, if the accept is successfully started
        //! but still pending (no available client), iosb->pending is set to true and the method returns true.
        //! @see listen()
        //!
        virtual bool accept(TCPConnection& client, IPSocketAddress& client_address, IOSB* iosb = nullptr);

        //!
        //! Update the status of an asynchronous accept().
        //! This method shall be used with asynchronous I/O only. It returns an error when the system
        //! uses non-blocking I/O instead of asynchronous I/O.
        //! @param [in,out] client The socket which received the new connection.
        //! @param [out] client_address This object receives the socket address of the client.
        //! @param [in,out] iosb Address of the IOSB structure which was used when connect() was called.
        //! @return True on success, false on error.
        //!
        bool setAcceptStatus(TCPConnection& client, IPSocketAddress& client_address, IOSB* iosb);

    protected:
        // Inherited methods.
        virtual bool closeImplementation(bool silent) override;

    private:
#if defined(TS_WINDOWS)
        // For Windows asynchronous I/O, we need to keep parameter in one single structure which lives during the I/O.
        class TSCOREDLL AsyncBuffers: public Object
        {
            TS_NOCOPY(AsyncBuffers);
        public:
            // The remote and local socket addresses are returned into a buffer. The Microsoft documentation says that
            // "the number of bytes reserved for the address information must be at least 16 bytes more than the maximum
            // address length for the transport protocol in use".
            static constexpr size_t ADDR_BUFLEN = sizeof(::sockaddr_storage) + 16;

            // Buffer for remote and local socket addresses.
            uint8_t buf[2 * ADDR_BUFLEN] {};

            // Constructor and destructor.
            AsyncBuffers() = default;
            virtual ~AsyncBuffers() override;
        };
#endif
    };
}
