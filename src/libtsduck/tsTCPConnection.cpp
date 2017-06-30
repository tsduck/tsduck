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
//  TCP connected socket, for data communication
//
//----------------------------------------------------------------------------

#include "tsTCPConnection.h"
#include "tsGuard.h"
#include "tsMemoryUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// This method is used by specific subclass to declare that the socket
// has just become connected.
//----------------------------------------------------------------------------

void ts::TCPConnection::declareConnected (ReportInterface& report)
{
    {
        Guard lock (_mutex);
        if (_is_connected) {
            report.fatal ("implementation error: TCP socket already connected");
            throw ImplementationError ("TCP socket already connected");
            return;
        }
        _is_connected = true;
    }
    handleConnected (report);
}


//----------------------------------------------------------------------------
// Declare that the socket has just become disconnected.
//----------------------------------------------------------------------------

void ts::TCPConnection::declareDisconnected (ReportInterface& report)
{
    {
        Guard lock (_mutex);
        if (_is_connected) {
            _is_connected = false;
        }
        else {
            return;
        }
    }
    handleDisconnected (report);
}


//----------------------------------------------------------------------------
// Invoked when socket is closed()
//----------------------------------------------------------------------------

void ts::TCPConnection::handleClosed (ReportInterface& report)
{
    declareDisconnected (report);
    SuperClass::handleClosed (report);
}


//----------------------------------------------------------------------------
// Get connected peer.
//----------------------------------------------------------------------------

bool ts::TCPConnection::getPeer (SocketAddress& peer, ReportInterface& report) const
{
    ::sockaddr sock_addr;
    TS_SOCKET_SOCKLEN_T len = sizeof(sock_addr);
    TS_ZERO (sock_addr);
    if (::getpeername (getSocket(), &sock_addr, &len) != 0) {
        report.error ("error getting socket peer: " + SocketErrorCodeMessage ());
        return false;
    }
    peer = SocketAddress (sock_addr);
    return true;
}

std::string ts::TCPConnection::peerName() const
{
    SocketAddress peer;
    return getPeer (peer, NULLREP) ? std::string (peer) : "";
}


//----------------------------------------------------------------------------
// Send data.
//----------------------------------------------------------------------------

bool ts::TCPConnection::send(const void* buffer, size_t size, ReportInterface& report)
{
    const char* data = reinterpret_cast <const char*> (buffer);
    size_t remain = size;

    while (remain > 0) {
        TS_SOCKET_SSIZE_T gone = ::send(getSocket(), TS_SENDBUF_T(data), int(remain), 0);
        if (gone > 0) {
            assert (size_t (gone) <= remain);
            data += gone;
            remain -= gone;
        }
#if !defined(__windows)
        else if (errno == EINTR) {
            // Ignore signal, retry
            report.debug("send() interrupted by signal, retrying");
        }
#endif
        else {
            report.error("error sending data to socket: " + SocketErrorCodeMessage());
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

bool ts::TCPConnection::receive (void* data,             // Buffers address
                                   size_t max_size,        // Buffer size
                                   size_t& ret_size,       // Received message size
                                   const AbortInterface* abort,
                                   ReportInterface& report)
{
    // Clear returned values
    ret_size = 0;

    // Loop on unsollicited interrupts
    for (;;) {
        TS_SOCKET_SSIZE_T got = ::recv(getSocket(), TS_RECVBUF_T(data), int(max_size), 0);
        const SocketErrorCode err_code = LastSocketErrorCode();
        if (got > 0) {
            // Received some data
            assert(size_t(got) <= max_size);
            ret_size = size_t(got);
            return true;
        }
        else if (got == 0 || err_code == TS_SOCKET_ERR_RESET) {
            // End of connection (graceful or aborted). Do not report an error.
            declareDisconnected(report);
            return false;
        }
#if !defined(__windows)
        else if (err_code == EINTR) {
            // Ignore signal, retry
            report.debug ("recv() interrupted by signal, retrying");
        }
#endif
        else {
            Guard lock (_mutex);
            if (isOpen()) {
                report.error ("error receiving data from socket: " + SocketErrorCodeMessage (err_code));
            }
            return false;
        }
    }
}



//----------------------------------------------------------------------------
// Receive data until buffer is full.
//----------------------------------------------------------------------------

bool ts::TCPConnection::receive (void* buffer, size_t size, const AbortInterface* abort, ReportInterface& report)
{
    char* data = reinterpret_cast <char*> (buffer);
    size_t remain = size;

    while (remain > 0) {
        size_t got;
        if (!receive (data, remain, got, abort, report)) {
            return false;
        }
        assert (got <= remain);
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

bool ts::TCPConnection::connect (const SocketAddress& addr, ReportInterface& report)
{
    // Loop on unsollicited interrupts
    for (;;) {
        ::sockaddr sock_addr;
        addr.copy (sock_addr);
        report.debug ("connecting to " + std::string (addr));
        if (::connect (getSocket(), &sock_addr, sizeof(sock_addr)) == 0) {
            declareConnected (report);
            return true;
        }
#if !defined (__windows)
        else if (errno == EINTR) {
            // Ignore signal, retry
            report.debug ("connect() interrupted by signal, retrying");
        }
#endif
        else {
            report.error ("error connecting socket: " + SocketErrorCodeMessage ());
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Shutdown the socket.
//----------------------------------------------------------------------------

bool ts::TCPConnection::shutdownSocket(int how, ReportInterface& report)
{
    if (::shutdown(getSocket(), how) != 0) {
        const SocketErrorCode err_code = LastSocketErrorCode();
        Guard lock(_mutex);
        // Do not report "not connected" errors since they are normal when the peer disconnects first.
        if (isOpen() && err_code != TS_SOCKET_ERR_NOTCONN) {
            report.error("error shutting down socket: " + SocketErrorCodeMessage(err_code));
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Close the write direction of the connection.
//----------------------------------------------------------------------------

bool ts::TCPConnection::closeWriter(ReportInterface& report)
{
    report.debug ("closing socket writer");
    return shutdownSocket(TS_SOCKET_SHUT_WR, report);
}


//----------------------------------------------------------------------------
// Disconnect from remote partner.
//----------------------------------------------------------------------------

bool ts::TCPConnection::disconnect(ReportInterface& report)
{
    declareDisconnected(report);
    report.debug("disconnecting socket");
    return shutdownSocket(TS_SOCKET_SHUT_RDWR, report);
}
