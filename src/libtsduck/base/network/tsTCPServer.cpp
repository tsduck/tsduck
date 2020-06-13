//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsTCPServer.h"
#include "tsIPUtils.h"
#include "tsGuard.h"
#include "tsMemory.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TCPServer::listen(int backlog, Report& report)
{
    report.debug(u"server listen, backlog is %d", {backlog});
    if (::listen(getSocket(), backlog) != 0) {
        report.error(u"error starting TCP server: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Wait for a client
//----------------------------------------------------------------------------

bool ts::TCPServer::accept (TCPConnection& client, SocketAddress& client_address, Report& report)
{
    if (client.isConnected()) {
        report.error(u"invalid client in accept(): already connected");
        return false;
    }

    if (client.isOpen()) {
        report.error(u"invalid client in accept(): already open");
        return false;
    }

    report.debug(u"server accepting clients");
    ::sockaddr sock_addr;
    TS_SOCKET_SOCKLEN_T len = sizeof(sock_addr);
    TS_ZERO(sock_addr);
    TS_SOCKET_T client_sock = ::accept(getSocket(), &sock_addr, &len);

    if (client_sock == TS_SOCKET_T_INVALID) {
        Guard lock(_mutex);
        if (isOpen()) {
            report.error(u"error accepting TCP client: %s", {SocketErrorCodeMessage()});
        }
        return false;
    }

    client_address = SocketAddress(sock_addr);
    report.debug(u"received connection from %s", {client_address});

    client.declareOpened(client_sock, report);
    client.declareConnected(report);
    return true;
}


//----------------------------------------------------------------------------
// Inherited and overridden
//----------------------------------------------------------------------------

bool ts::TCPServer::close(Report& report)
{
    // Shutdown server socket.
    // Do not report "not connected" errors since they are normal when the client disconnects first.
    if (::shutdown(getSocket(), TS_SOCKET_SHUT_RDWR) != 0) {
        const SocketErrorCode err_code = LastSocketErrorCode();
        if (err_code != TS_SOCKET_ERR_NOTCONN) {
            report.error(u"error shutdowning server socket: %s", {SocketErrorCodeMessage()});
        }
    }

    // Then invoke superclass
    return SuperClass::close(report);
}
