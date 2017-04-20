//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  TCP connected socket, for data communication.
//!  Can be used as TCP client (using connect() method).
//!  Can be used by TCP server to receive a client connection.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTCPSocket.h"

namespace ts {

    class TSDUCKDLL TCPConnection: public TCPSocket
    {
    public:
        typedef TCPSocket SuperClass;

        // Constructor
        TCPConnection(): TCPSocket(), _is_connected (false) {}

        // Client-side scenario. [*] means mandatory.
        //
        //   open() [*]
        //   reusePort()
        //   setSendBufferSize()
        //   setReceiveBufferSize()
        //   setLingerTime() / setNoLinger()
        //   setKeepAlive()
        //   setNoDelay()
        //   setTTL()
        //   bind() [*]
        //   connect() [*]
        //   send() / receive()
        //   disconnect()
        //   close()
        //
        // Unless specified otherwise, all methods return true on success
        // and false on error.

        // Connect to a remote address and port.
        // Use this method when acting as TCP client.
        // Do not use on server side: the TCPConnection object is passed
        // to TCPServer::accept() which establishes the connection.
        bool connect (const SocketAddress&, ReportInterface&);

        // Check if socket is connected.
        bool isConnected() const {return isOpen() && _is_connected;}

        // Get connected peer.
        bool getPeer (SocketAddress&, ReportInterface&) const;
        std::string peerName() const;

        // Close the write direction of the connection.
        // The application shall call this routine after sending the last
        // message but may still want to receive messages, waiting for the
        // peer to voluntary disconnect.
        bool closeWriter (ReportInterface&);

        // Disconnect from remote partner.
        bool disconnect (ReportInterface&);

        // Send data.
        bool send (const void* data, size_t size, ReportInterface&);

        // Receive data.
        // If abort interface is non-zero, invoke it when I/O is interrupted
        // (in case of user-interrupt, return, otherwise retry).
        bool receive (void* buffer,           // Buffers address
                      size_t max_size,        // Buffer size
                      size_t& ret_size,       // Received message size
                      const AbortInterface*,
                      ReportInterface&);

        // Receive data until buffer is full.
        bool receive (void* buffer,       // Buffers address
                      size_t size,        // Buffer size
                      const AbortInterface*,
                      ReportInterface&);

    protected:
        // These virtual methods can be overriden by subclasses to be notified
        // of connection and disconnection. All subclasses should explicitely
        // invoke their superclass' handlers.
        virtual void handleConnected (ReportInterface&) {}
        virtual void handleDisconnected (ReportInterface&) {}

        // Overriden methods
        virtual void handleClosed (ReportInterface&);

    private:
        bool _is_connected;

        // Declare that the socket has just become connected / disconnected.
        void declareConnected(ReportInterface&) throw(ImplementationError);
        void declareDisconnected(ReportInterface&);
        friend class TCPServer;

        // Shutdown the socket.
        bool shutdownSocket(int how, ReportInterface& report);
        
        // Unreachable operations
        TCPConnection(const TCPConnection&) = delete;
        TCPConnection& operator=(const TCPConnection&) = delete;
    };

    // Safe pointers
    typedef SafePtr<TCPConnection,NullMutex> TCPConnectionPtr; // single-threaded
    typedef SafePtr<TCPConnection,Mutex> TCPConnectionPtrMT;   // multi-threaded
}
