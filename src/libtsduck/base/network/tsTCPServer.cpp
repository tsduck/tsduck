//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTCPServer.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TCPServer::listen(int backlog, Report& report)
{
    report.debug(u"server listen, backlog is %d", {backlog});
    if (::listen(getSocket(), backlog) != 0) {
        report.error(u"error starting TCP server: %s", {SysErrorCodeMessage()});
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
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (isOpen()) {
            report.error(u"error accepting TCP client: %s", {SysErrorCodeMessage()});
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
        const int errcode = LastSysErrorCode();
        if (errcode != SYS_SOCKET_ERR_NOTCONN) {
            report.error(u"error shutdowning server socket: %s", {SysErrorCodeMessage(errcode)});
        }
    }

    // Then invoke superclass
    return SuperClass::close(report);
}
