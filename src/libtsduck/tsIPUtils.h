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
//!  Utilities for IP networking
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsCerrReport.h"
#include "tsSysUtils.h"

namespace ts {

    //!
    //! Initialize the IP libraries in the current process.
    //!
    //! On some systems (UNIX), there is no need to initialize IP.
    //! On other systems (Windows), using IP and socket without initialization fails.
    //! This method is a portable way to ensure that IP is properly initialized.
    //! It shall be called at least once before using IP in the application.
    //!
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool IPInitialize (ReportInterface& = CERR);

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
#if defined(TS_WINDOWS)
        return ::WSAGetLastError();
#elif defined(TS_UNIX)
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

    //! @cond nodoxygen
    class IPAddress;
    //! @endcond

    //!
    //! Vector of IP addresses.
    //!
    typedef std::vector<IPAddress> IPAddressVector;

    //!
    //! Get the list of all local IPv4 addresses in the system.
    //!
    //! @param [out] addresses A vector of IpAddress which receives the list
    //! of all local IPv4 addresses in the system, except @link ts::IPAddress::LocalHost @endlink.
    //! @param [in] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool GetLocalIPAddresses(IPAddressVector& addresses, ReportInterface& report = CERR);

    //!
    //! Check if a local system interface has a specified IP address.
    //!
    //! @param [in] address The IP address to check.
    //! @return True is @a address is the address of a local system interface, false otherwise.
    //!
    TSDUCKDLL bool IsLocalIPAddress(const IPAddress& address);

    //------------------------------------------------------------------------
    // Socket programming portability macros.
    // Most socket types and functions have identical API in UNIX and Windows.
    // However, there are some slight incompatibilities which are solved by
    // using the following macros.
    //------------------------------------------------------------------------

#if defined(DOXYGEN)

    //!
    //! Data type for socket descriptors as returned by the socket() system call.
    //!
    #define TS_SOCKET_T platform_specific

    //!
    //! Value of type TS_SOCKET_T which is returned by the socket() system call in case of failure.
    //! Example:
    //! @code
    //! TS_SOCKET_T sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //! if (sock == TS_SOCKET_T_INVALID) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
    #define TS_SOCKET_T_INVALID platform_specific

    //!
    //! Integer data type which receives the length of a struct sockaddr.
    //! Example:
    //! @code
    //! struct sockaddr sock_addr;
    //! TS_SOCKET_SOCKLEN_T len = sizeof(sock_addr);
    //! if (getsockname(sock, &sock_addr, &len) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
    #define TS_SOCKET_SOCKLEN_T platform_specific

    //!
    //! Integer data type for a "signed size" returned from send() or recv() system calls.
    //! Example:
    //! @code
    //! TS_SOCKET_SSIZE_T got = recv(sock, TS_RECVBUF_T(&data), max_size, 0);
    //! @endcode
    //!
    #define TS_SOCKET_SSIZE_T platform_specific

    //!
    //! Integer data type for the Time To Live (TTL) socket option.
    //! Example:
    //! @code
    //! TS_SOCKET_TTL_T ttl = 10;
    //! if (setsockopt(sock, IPPROTO_IP, IP_TTL, TS_SOCKOPT_T(&ttl), sizeof(ttl)) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
    #define TS_SOCKET_TTL_T platform_specific

    //!
    //! Integer data type for the multicast Time To Live (TTL) socket option.
    //! Example:
    //! @code
    //! TS_SOCKET_MC_TTL_T mttl = 1;
    //! if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, TS_SOCKOPT_T(&mttl), sizeof(mttl)) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
    #define TS_SOCKET_MC_TTL_T platform_specific

    //!
    //! Type conversion macro for the field l_linger in the struct linger socket option.
    //! All systems do not use the same type size and this may generate some warnings.
    //! Example:
    //! @code
    //! struct linger lin;
    //! lin.l_linger = TS_SOCKET_L_LINGER_T(seconds);
    //! @endcode
    //!
    #define TS_SOCKET_L_LINGER_T(x) platform_specific

    //!
    //! Integer data type for the IP_PKTINFO socket option.
    //! Example:
    //! @code
    //! TS_SOCKET_PKTINFO_T state = 1;
    //! if (setsockopt(sock, IPPROTO_IP, IP_PKTINFO, TS_SOCKOPT_T(&state), sizeof(state)) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
    #define TS_SOCKET_PKTINFO_T platform_specific

    //!
    //! Type conversion macro for the address of a socket option value.
    //! The "standard" parameter type is @c void* but some systems use other exotic values.
    //! Example:
    //! @code
    //! TS_SOCKET_TTL_T ttl = 10;
    //! if (setsockopt(sock, IPPROTO_IP, IP_TTL, TS_SOCKOPT_T(&ttl), sizeof(ttl)) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
    #define TS_SOCKOPT_T(x) platform_specific

    //!
    //! Type conversion macro for the address of the data buffer for a recv() system call.
    //! The "standard" parameter type is @c void* but some systems use other exotic values.
    //! Example:
    //! @code
    //! TS_SOCKET_SSIZE_T got = recv(sock, TS_RECVBUF_T(&data), max_size, 0);
    //! @endcode
    //!
    #define TS_RECVBUF_T(x) platform_specific

    //!
    //! Type conversion macro for the address of the data buffer for a send() system call.
    //! The "standard" parameter type is @c void* but some systems use other exotic values.
    //! Example:
    //! @code
    //! TS_SOCKET_SSIZE_T gone = send(sock, TS_SENDBUF_T(&data), size, 0);
    //! @endcode
    //!
    #define TS_SENDBUF_T(x) platform_specific

    //!
    //! Name of the ioctl() system call which applies to socket devices.
    //! The "standard" name is @c ioctl but some systems use other exotic names.
    //! Note that the ioctl() system call is rarely used on sockets.
    //! Most options are accessed through getsockopt() and setsockopt().
    //!
    #define TS_SOCKET_IOCTL platform_specific

    //!
    //! Name of the close() system call which applies to socket devices.
    //! The "standard" name is @c close but some systems use other exotic names.
    //! Example:
    //! @code
    //! TS_SOCKET_CLOSE(sock);
    //! @endcode
    //!
    #define TS_SOCKET_CLOSE platform_specific

    //!
    //! Name of the option for the shutdown() system call which means "close on both directions".
    //! Example:
    //! @code
    //! shutdown(sock, TS_SOCKET_SHUT_RDWR);
    //! @endcode
    //!
    #define TS_SOCKET_SHUT_RDWR platform_specific

    //!
    //! Name of the option for the shutdown() system call which means "close on receive side".
    //! Example:
    //! @code
    //! shutdown(sock, TS_SOCKET_SHUT_RD);
    //! @endcode
    //!
    #define TS_SOCKET_SHUT_RD platform_specific

    //!
    //! Name of the option for the shutdown() system call which means "close on send side".
    //! Example:
    //! @code
    //! shutdown(sock, TS_SOCKET_SHUT_WR);
    //! @endcode
    //!
    #define TS_SOCKET_SHUT_WR platform_specific

    //!
    //! System error code value meaning "connection reset by peer".
    //!
    #define TS_SOCKET_ERR_RESET platform_specific

    //!
    //! System error code value meaning "peer socket not connected".
    //!
    #define TS_SOCKET_ERR_NOTCONN platform_specific

#elif defined (TS_WINDOWS)

    #define TS_SOCKET_T             ::SOCKET
    #define TS_SOCKET_T_INVALID     INVALID_SOCKET
    #define TS_SOCKET_SOCKLEN_T     int
    #define TS_SOCKET_SSIZE_T       int
    #define TS_SOCKET_TTL_T         ::DWORD
    #define TS_SOCKET_MC_TTL_T      ::DWORD
    #define TS_SOCKET_L_LINGER_T(x) (static_cast<u_short>(x))
    #define TS_SOCKET_PKTINFO_T     ::DWORD
    #define TS_SOCKOPT_T(x)         (reinterpret_cast<const char*>(x))
    #define TS_RECVBUF_T(x)         (reinterpret_cast<char*>(x))
    #define TS_SENDBUF_T(x)         (reinterpret_cast<const char*>(x))
    #define TS_SOCKET_IOCTL         ::ioctlsocket
    #define TS_SOCKET_CLOSE         ::closesocket
    #define TS_SOCKET_SHUT_RDWR     SD_BOTH
    #define TS_SOCKET_SHUT_RD       SD_RECEIVE
    #define TS_SOCKET_SHUT_WR       SD_SEND
    #define TS_SOCKET_ERR_RESET     WSAECONNRESET
    #define TS_SOCKET_ERR_NOTCONN   WSAENOTCONN

#elif defined(TS_UNIX)

    #define TS_SOCKET_T             int
    #define TS_SOCKET_T_INVALID     (-1)
    #define TS_SOCKET_SOCKLEN_T     ::socklen_t
    #define TS_SOCKET_SSIZE_T       ::ssize_t
    #define TS_SOCKET_TTL_T         int
    #define TS_SOCKET_MC_TTL_T      unsigned char
    #define TS_SOCKET_L_LINGER_T(x) (static_cast<int>(x))
    #define TS_SOCKET_PKTINFO_T     int
    #define TS_SOCKOPT_T(x)         (x)
    #define TS_RECVBUF_T(x)         (x)
    #define TS_SENDBUF_T(x)         (x)
    #define TS_SOCKET_IOCTL         ::ioctl
    #define TS_SOCKET_CLOSE         ::close
    #define TS_SOCKET_SHUT_RDWR     SHUT_RDWR
    #define TS_SOCKET_SHUT_RD       SHUT_RD
    #define TS_SOCKET_SHUT_WR       SHUT_WR
    #define TS_SOCKET_ERR_RESET     EPIPE
    #define TS_SOCKET_ERR_NOTCONN   ENOTCONN

#else
    #error "check socket compatibility macros on this platform"
#endif
}
