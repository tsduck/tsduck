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
//  Utilities for IP networking
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsCerrReport.h"
#include "tsSysUtils.h"

namespace ts {

    // Initialize IP usage. Shall be called once at least.
    // Return true on success, false on error.
    TSDUCKDLL bool IPInitialize (ReportInterface& = CERR);

    // Most socket types and functions have identical API in UNIX and Windows,
    // except the following:
#if defined (__windows)
#define TS_SOCKET_T           ::SOCKET
#define TS_SOCKET_T_INVALID   INVALID_SOCKET
#define TS_SOCKET_SOCKLEN_T   int
#define TS_SOCKET_SSIZE_T     int
#define TS_SOCKET_TTL_T       ::DWORD
#define TS_SOCKET_MC_TTL_T    ::DWORD
#define TS_SOCKOPT_T(x)       (reinterpret_cast<const char*> (x))
#define TS_RECVBUF_T(x)       (reinterpret_cast<char*> (x))
#define TS_SENDBUF_T(x)       (reinterpret_cast<const char*> (x))
#define TS_SOCKET_IOCTL       ::ioctlsocket
#define TS_SOCKET_CLOSE       ::closesocket
#define TS_SOCKET_SHUT_RDWR   SD_BOTH
#define TS_SOCKET_SHUT_RD     SD_RECEIVE
#define TS_SOCKET_SHUT_WR     SD_SEND
#define TS_SOCKET_ERR_RESET   WSAECONNRESET
#define TS_SOCKET_ERR_NOTCONN WSAENOTCONN
#else
#define TS_SOCKET_T           int
#define TS_SOCKET_T_INVALID   (-1)
#define TS_SOCKET_SOCKLEN_T   ::socklen_t
#define TS_SOCKET_SSIZE_T     ::ssize_t
#define TS_SOCKET_TTL_T       int
#define TS_SOCKET_MC_TTL_T    unsigned char
#define TS_SOCKOPT_T(x)       (x)
#define TS_RECVBUF_T(x)       (x)
#define TS_SENDBUF_T(x)       (x)
#define TS_SOCKET_IOCTL       ::ioctl
#define TS_SOCKET_CLOSE       ::close
#define TS_SOCKET_SHUT_RDWR   SHUT_RDWR
#define TS_SOCKET_SHUT_RD     SHUT_RD
#define TS_SOCKET_SHUT_WR     SHUT_WR
#define TS_SOCKET_ERR_RESET   EPIPE
#define TS_SOCKET_ERR_NOTCONN ENOTCONN
#endif

    //!
    //! Type for socket error code.
    //!
    typedef int SocketErrorCode;

    //!
    //! Get the error code of the last socket system call.
    //!
    //! The validity of the returned value may depends on specific conditions.
    //!
    //! @return The error code of the last socket system call.
    //!
    TSDUCKDLL inline SocketErrorCode LastSocketErrorCode()
    {
#if defined(__windows)
        return ::WSAGetLastError();
#elif defined(__unix)
        return errno;
#else
        #error "Unsupported operating system"
#endif
    }

    //!
    //! Format a socket error code into a string.
    //!
    //! @param [in] code An error code from the operating system.
    //! Typically a result from LastSocketErrorCode().
    //! @return A string describing the error.
    //!
    TSDUCKDLL inline std::string SocketErrorCodeMessage(SocketErrorCode code = LastSocketErrorCode())
    {
        // Note for windows: although error codes types are different for system and
        // winsock, it appears that the system error message also applies to winsock.
        return ErrorCodeMessage(code);
    }

    // IPAddress
    class IPAddress;
    typedef std::vector<IPAddress> IPAddressVector;

    // This method returns the list of all local IPv4 addresses in the system,
    // except LocalHost. Return true on success, false on error.
    TSDUCKDLL bool GetLocalIPAddresses(IPAddressVector&, ReportInterface& = CERR);

    // Check if a local system interface has a specified IP address.
    TSDUCKDLL bool IsLocalIPAddress(const IPAddress& address);
}
