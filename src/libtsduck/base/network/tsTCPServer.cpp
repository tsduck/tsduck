//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#include "tsGuardMutex.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TCPServer::listen(int backlog, Report& report)
{
    report.debug(u"server listen, backlog is %d", {backlog});
    if (::listen(getSocket(), backlog) != 0) {
        report.error(u"error starting TCP server: %s", {SysSocketErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Wait for a client
//----------------------------------------------------------------------------

bool ts::TCPServer::accept (TCPConnection& client, IPv4SocketAddress& client_address, Report& report)
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
    SysSocketLengthType len = sizeof(sock_addr);
    TS_ZERO(sock_addr);
    SysSocketType client_sock = ::accept(getSocket(), &sock_addr, &len);

    if (client_sock == SYS_SOCKET_INVALID) {
        GuardMutex lock(_mutex);
        if (isOpen()) {
            report.error(u"error accepting TCP client: %s", {SysSocketErrorCodeMessage()});
        }
        return false;
    }

    client_address = IPv4SocketAddress(sock_addr);
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
    if (::shutdown(getSocket(), SYS_SOCKET_SHUT_RDWR) != 0) {
        const SysSocketErrorCode err_code = LastSysSocketErrorCode();
        if (err_code != SYS_SOCKET_ERR_NOTCONN) {
            report.error(u"error shutdowning server socket: %s", {SysSocketErrorCodeMessage()});
        }
    }

    // Then invoke superclass
    return SuperClass::close(report);
}
