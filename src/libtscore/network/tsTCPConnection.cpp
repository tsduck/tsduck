//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTCPConnection.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"
#include "tsMemory.h"
#include "tsNullReport.h"
#include "tsException.h"


//----------------------------------------------------------------------------
// Default implementations of handlers.
//----------------------------------------------------------------------------

void ts::TCPConnection::handleConnected(Report& report)
{
}

void ts::TCPConnection::handleDisconnected(Report& report)
{
}


//----------------------------------------------------------------------------
// This method is used by specific subclass to declare that the socket
// has just become connected.
//----------------------------------------------------------------------------

void ts::TCPConnection::declareConnected(Report& report)
{
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (_is_connected) {
            report.fatal(u"implementation error: TCP socket already connected");
            throw ImplementationError(u"TCP socket already connected");
        }
        _is_connected = true;
    }
    handleConnected(report);
}


//----------------------------------------------------------------------------
// Declare that the socket has just become disconnected.
//----------------------------------------------------------------------------

void ts::TCPConnection::declareDisconnected(Report& report)
{
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (_is_connected) {
            _is_connected = false;
        }
        else {
            return;
        }
    }
    handleDisconnected(report);
}


//----------------------------------------------------------------------------
// Invoked when socket is closed()
//----------------------------------------------------------------------------

void ts::TCPConnection::handleClosed(Report& report)
{
    declareDisconnected(report);
    SuperClass::handleClosed(report);
}


//----------------------------------------------------------------------------
// Get connected peer.
//----------------------------------------------------------------------------

bool ts::TCPConnection::getPeer(IPSocketAddress& peer, Report& report) const
{
    ::sockaddr_storage sock_addr;
    SysSocketLengthType len = sizeof(sock_addr);
    TS_ZERO(sock_addr);
    if (::getpeername(getSocket(), reinterpret_cast<::sockaddr*>(&sock_addr), &len) != 0) {
        report.error(u"error getting socket peer: %s", SysErrorCodeMessage());
        return false;
    }
    peer = IPSocketAddress(sock_addr);
    return true;
}

ts::UString ts::TCPConnection::peerName() const
{
    IPSocketAddress peer;
    return getPeer(peer, NULLREP) ? peer.toString() : u"";
}


//----------------------------------------------------------------------------
// Send data.
//----------------------------------------------------------------------------

bool ts::TCPConnection::send(const void* buffer, size_t size, Report& report)
{
    const char* data = reinterpret_cast<const char*>(buffer);
    size_t remain = size;

    while (remain > 0) {
        SysSocketSignedSizeType gone = ::send(getSocket(), SysSendBufferPointer(data), int(remain), 0);
        if (gone > 0) {
            assert(size_t(gone) <= remain);
            data += gone;
            remain -= gone;
        }
#if !defined(TS_WINDOWS)
        else if (errno == EINTR) {
            // Ignore signal, retry
            report.debug(u"send() interrupted by signal, retrying");
        }
#endif
        else {
            report.error(u"error sending data to socket: %s", SysErrorCodeMessage());
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Receive data.
// If abort interface is non-zero, invoke it when I/O is interrupted
// (in case of user-interrupt, return, otherwise retry).
//----------------------------------------------------------------------------

bool ts::TCPConnection::receive(void* data,
                                size_t max_size,
                                size_t& ret_size,
                                const AbortInterface* abort,
                                Report& report)
{
    // Clear returned values
    ret_size = 0;

    // Loop on unsollicited interrupts
    for (;;) {
        SysSocketSignedSizeType got = ::recv(getSocket(), SysRecvBufferPointer(data), int(max_size), 0);
        const int errcode = LastSysErrorCode();
        if (got > 0) {
            // Received some data
            assert(size_t(got) <= max_size);
            ret_size = size_t(got);
            return true;
        }
        else if (got == 0 || errcode == SYS_SOCKET_ERR_RESET) {
            // End of connection (graceful or aborted). Do not report an error.
            declareDisconnected(report);
            return false;
        }
        else if (abort != nullptr && abort->aborting()) {
            // User-interrupt, end of processing but no error message
            return false;
        }
#if defined(TS_UNIX)
        else if (errcode == EINTR) {
            // Ignore signal, retry.
            report.debug(u"recv() interrupted by signal, retrying");
        }
#endif
        else {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            if (isOpen()) {
                // Report the error only if the error does not result from a close in another thread.
                report.error(u"error receiving data from socket: %s", SysErrorCodeMessage(errcode));
            }
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Receive data until buffer is full.
//----------------------------------------------------------------------------

bool ts::TCPConnection::receive(void* buffer, size_t size, const AbortInterface* abort, Report& report)
{
    char* data = reinterpret_cast<char*>(buffer);
    size_t remain = size;

    while (remain > 0) {
        size_t got;
        if (!receive(data, remain, got, abort, report)) {
            return false;
        }
        assert(got <= remain);
        data += got;
        remain -= got;
    }

    return true;
}


//----------------------------------------------------------------------------
// Connect to a remote address and port.
// Use this method when acting as TCP client.
// Do not use on server side: the TCPConnection object is passed
// to TCPServer::accept() which establishes the connection.
//----------------------------------------------------------------------------

bool ts::TCPConnection::connect(const IPSocketAddress& addr, Report& report)
{
    IPSocketAddress addr2(addr);
    if (!convert(addr2, report)) {
        return false;
    }

    // Loop on unsollicited interrupts
    for (;;) {
        ::sockaddr_storage sock_addr;
        const size_t sock_size = addr2.get(sock_addr);
        report.debug(u"connecting to %s", addr2);
        if (::connect(getSocket(), reinterpret_cast<const ::sockaddr*>(&sock_addr), socklen_t(sock_size)) == 0) {
            declareConnected(report);
            return true;
        }
#if !defined(TS_WINDOWS)
        else if (errno == EINTR) {
            // Ignore signal, retry
            report.debug(u"connect() interrupted by signal, retrying");
        }
#endif
        else {
            report.error(u"error connecting socket: %s", SysErrorCodeMessage());
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Shutdown the socket.
//----------------------------------------------------------------------------

bool ts::TCPConnection::shutdownSocket(int how, Report& report)
{
    if (::shutdown(getSocket(), how) != 0) {
        const int errcode = LastSysErrorCode();
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        // Do not report "not connected" errors since they are normal when the peer disconnects first.
        if (isOpen() && errcode != SYS_SOCKET_ERR_NOTCONN) {
            report.error(u"error shutting down socket: %s", SysErrorCodeMessage(errcode));
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Close the write direction of the connection.
//----------------------------------------------------------------------------

bool ts::TCPConnection::closeWriter(Report& report)
{
    report.debug(u"closing socket writer");
    return shutdownSocket(SYS_SOCKET_SHUT_WR, report);
}


//----------------------------------------------------------------------------
// Disconnect from remote partner.
//----------------------------------------------------------------------------

bool ts::TCPConnection::disconnect(Report& report)
{
    declareDisconnected(report);
    report.debug(u"disconnecting socket");
    return shutdownSocket(SYS_SOCKET_SHUT_RDWR, report);
}
