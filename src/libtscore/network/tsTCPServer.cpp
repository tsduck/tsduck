//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTCPServer.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"
#include "tsInitZero.h"


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TCPServer::listen(int backlog)
{
    report().debug(u"server listen, backlog is %d", backlog);
    if (::listen(getSocket(), backlog) != 0) {
        report().error(u"error starting TCP server: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Wait for a client
//----------------------------------------------------------------------------

bool ts::TCPServer::accept(TCPConnection& client, IPSocketAddress& client_address)
{
    if (client.isConnected()) {
        report().error(u"invalid client in accept(): already connected");
        return false;
    }

    if (client.isOpen()) {
        report().error(u"invalid client in accept(): already open");
        return false;
    }

    report().debug(u"server accepting clients");
    InitZero<::sockaddr_storage> sock_addr;
    SysSocketLengthType len = sizeof(sock_addr.data);
    SysSocketType client_sock = ::accept(getSocket(), reinterpret_cast<::sockaddr*>(&sock_addr.data), &len);

    if (client_sock == SYS_SOCKET_INVALID) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (isOpen()) {
            report().error(u"error accepting TCP client: %s", SysErrorCodeMessage());
        }
        return false;
    }

    client_address = IPSocketAddress(sock_addr.data);
    report().debug(u"received connection from %s", client_address);

    client.declareOpened(client_sock);
    client.declareConnected();
    return true;
}


//----------------------------------------------------------------------------
// Close the server resources.
//----------------------------------------------------------------------------

bool ts::TCPServer::close(bool silent)
{
    // Shutdown server socket.
    // Do not report "not connected" errors since they are normal when the client disconnects first.
    if (::shutdown(getSocket(), SYS_SOCKET_SHUT_RDWR) != 0) {
        const int errcode = LastSysErrorCode();
        if (errcode != SYS_SOCKET_ERR_NOTCONN) {
            report().log(SilentLevel(silent), u"error shutdowning server socket: %s", SysErrorCodeMessage(errcode));
        }
    }

    // Then invoke superclass
    return SuperClass::close(silent);
}
