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
//
//  TCP Server
//
//----------------------------------------------------------------------------

#include "tsTCPServer.h"
#include "tsDecimal.h"
#include "tsGuard.h"
#include "tsMemoryUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TCPServer::listen (int backlog, ReportInterface& report)
{
    report.debug ("server listen, backlog is " + Decimal (backlog));
    if (::listen (getSocket(), backlog) != 0) {
        report.error ("error starting TCP server: " + SocketErrorCodeMessage ());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Wait for a client
//----------------------------------------------------------------------------

bool ts::TCPServer::accept (TCPConnection& client, SocketAddress& client_address, ReportInterface& report)
{
    if (client.isConnected()) {
        report.error ("invalid client in accept(): already connected");
        return false;
    }

    if (client.isOpen()) {
        report.error ("invalid client in accept(): already open");
        return false;
    }

    report.debug ("server accepting clients");
    ::sockaddr sock_addr;
    TS_SOCKET_SOCKLEN_T len = sizeof(sock_addr);
    TS_ZERO (sock_addr);
    TS_SOCKET_T client_sock = ::accept (getSocket(), &sock_addr, &len);

    if (client_sock == TS_SOCKET_T_INVALID) {
        Guard lock (_mutex);
        if (isOpen()) {
            report.error ("error accepting TCP client: " + SocketErrorCodeMessage ());
        }
        return false;
    }

    client_address = SocketAddress (sock_addr);
    report.debug ("received connection from " + std::string (client_address));

    client.declareOpened (client_sock, report);
    client.declareConnected (report);
    return true;
}


//----------------------------------------------------------------------------
// Inherited and overridden
//----------------------------------------------------------------------------

bool ts::TCPServer::close (ReportInterface& report)
{
    // Shutdown server socket.
    // Do not report "not connected" errors since they are normal when the client disconnects first.
    if (::shutdown(getSocket(), TS_SOCKET_SHUT_RDWR) != 0) {
        const SocketErrorCode err_code = LastSocketErrorCode();
        if (err_code != TS_SOCKET_ERR_NOTCONN) {
            report.error("error shutdowning server socket: " + SocketErrorCodeMessage(err_code));
        }
    }

    // Then invoke superclass
    return SuperClass::close (report);
}
