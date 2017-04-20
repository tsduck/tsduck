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
//!  TCP Socket
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSocketAddress.h"
#include "tsAbortInterface.h"
#include "tsReportInterface.h"
#include "tsNullReport.h"
#include "tsException.h"
#include "tsSafePtr.h"
#include "tsNullMutex.h"
#include "tsMutex.h"

namespace ts {

    class TSDUCKDLL TCPSocket
    {
    public:
        // Constructor & destructor
        TCPSocket(): _sock (TS_SOCKET_T_INVALID) {}
        virtual ~TCPSocket() {close (NULLREP);}

        // Open and close the socket
        bool open (ReportInterface&);
        bool close (ReportInterface&);

        // Check if socket is open
        bool isOpen() const {return _sock != TS_SOCKET_T_INVALID;}

        // Set various socket options
        bool setSendBufferSize (size_t bytes, ReportInterface&);
        bool setReceiveBufferSize (size_t bytes, ReportInterface&);
        bool reusePort (bool active, ReportInterface&);
        bool setTTL (int ttl, ReportInterface&);
        bool setNoLinger (ReportInterface&);
        bool setLingerTime (int seconds, ReportInterface&);
        bool setKeepAlive (bool active, ReportInterface&);
        bool setNoDelay (bool active, ReportInterface&);

        // Bind to a local address and port.
        bool bind (const SocketAddress&, ReportInterface&);

        // Get local socket address
        bool getLocalAddress (SocketAddress&, ReportInterface&);

        // Get socket device (use with care).
        // Return TS_SOCKET_T_INVALID if invalid.
        TS_SOCKET_T getSocket() const {return _sock;}

        // Exceptions
        tsDeclareException (ImplementationError);

    protected:
        Mutex _mutex;

        // These virtual methods can be overriden by subclasses to be notified
        // of open and close. All subclasses should explicitely
        // invoke their superclass' handlers.
        virtual void handleOpened (ReportInterface&) {}
        virtual void handleClosed (ReportInterface&) {}

    private:
        TS_SOCKET_T _sock;

        // This method is used by a server to declare that the socket has just become opened.
        void declareOpened (TS_SOCKET_T, ReportInterface&) throw (ImplementationError);
        friend class TCPServer;

        // Unreachable operations
        TCPSocket (const TCPSocket&);
        TCPSocket& operator= (const TCPSocket&);
    };

    // Safe pointers
    typedef SafePtr <TCPSocket, NullMutex> TCPSocketPtr; // single-threaded
    typedef SafePtr <TCPSocket, Mutex> TCPSocketPtrMT;   // multi-threaded
}
