//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup net
//!  Utilities for IP networking
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCerrReport.h"
#include "tsIPv4Address.h"
#include "tsIPv4AddressMask.h"

namespace ts {
    //!
    //! Initialize the IP libraries in the current process.
    //! On some systems (UNIX), there is no need to initialize IP.
    //! On other systems (Windows), using IP and socket without initialization fails.
    //! This method is a portable way to ensure that IP is properly initialized.
    //! It shall be called at least once before using IP in the application.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool IPInitialize(Report& = CERR);

    //------------------------------------------------------------------------
    // Portable definitions for system socket interface.
    // Most socket types and functions have identical API in UNIX and Windows.
    // However, there are some slight incompatibilities which are solved by
    // using the following definitions.
    //------------------------------------------------------------------------

    //!
    //! Data type for socket descriptors as returned by the socket() system call.
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSocketType;
#elif defined(TS_WINDOWS)
    typedef ::SOCKET SysSocketType;
#elif defined(TS_UNIX)
    typedef int SysSocketType;
#endif

    //!
    //! Value of type SysSocketType which is returned by the socket() system call in case of failure.
    //! Example:
    //! @code
    //! SysSocketType sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //! if (sock == SYS_SOCKET_INVALID) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
#if defined(DOXYGEN)
    constexpr SysSocketType SYS_SOCKET_INVALID = platform_specific;
#elif defined(TS_WINDOWS)
    constexpr SysSocketType SYS_SOCKET_INVALID = INVALID_SOCKET;
#elif defined(TS_UNIX)
    constexpr SysSocketType SYS_SOCKET_INVALID = -1;
#endif

    //!
    //! System error code value meaning "connection reset by peer".
    //!
#if defined(DOXYGEN)
    constexpr int SYS_SOCKET_ERR_RESET = platform_specific;
#elif defined(TS_WINDOWS)
    constexpr int SYS_SOCKET_ERR_RESET = WSAECONNRESET;
#elif defined(TS_UNIX)
    constexpr int SYS_SOCKET_ERR_RESET = EPIPE;
#endif

    //!
    //! System error code value meaning "peer socket not connected".
    //!
#if defined(DOXYGEN)
    constexpr int SYS_SOCKET_ERR_NOTCONN = platform_specific;
#elif defined(TS_WINDOWS)
    constexpr int SYS_SOCKET_ERR_NOTCONN = WSAENOTCONN;
#elif defined(TS_UNIX)
    constexpr int SYS_SOCKET_ERR_NOTCONN = ENOTCONN;
#endif

    //!
    //! Integer data type which receives the length of a struct sockaddr.
    //! Example:
    //! @code
    //! struct sockaddr sock_addr;
    //! SysSocketLengthType len = sizeof(sock_addr);
    //! if (getsockname(sock, &sock_addr, &len) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSocketLengthType;
#elif defined(TS_WINDOWS)
    typedef int SysSocketLengthType;
#elif defined(TS_UNIX)
    typedef ::socklen_t SysSocketLengthType;
#endif

    //!
    //! Integer data type for a "signed size" returned from send() or recv() system calls.
    //! Example:
    //! @code
    //! SysSocketSignedSizeType got = recv(sock, SysRecvBufferPointer(&data), max_size, 0);
    //! @endcode
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSocketSignedSizeType;
#elif defined(TS_WINDOWS)
    typedef int SysSocketSignedSizeType;
#elif defined(TS_UNIX)
    typedef ::ssize_t SysSocketSignedSizeType;
#endif

    //!
    //! Integer data type for the Time To Live (TTL) socket option.
    //! Example:
    //! @code
    //! SysSocketTTLType ttl = 10;
    //! if (setsockopt(sock, IPPROTO_IP, IP_TTL, SysSockOptPointer(&ttl), sizeof(ttl)) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSocketTTLType;
#elif defined(TS_WINDOWS)
    typedef ::DWORD SysSocketTTLType;
#elif defined(TS_UNIX)
    typedef int SysSocketTTLType;
#endif

    //!
    //! Integer data type for the multicast Time To Live (TTL) socket option.
    //! Example:
    //! @code
    //! SysSocketMulticastTTLType mttl = 1;
    //! if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, SysSockOptPointer(&mttl), sizeof(mttl)) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSocketMulticastTTLType;
#elif defined(TS_WINDOWS)
    typedef ::DWORD SysSocketMulticastTTLType;
#elif defined(TS_UNIX)
    typedef unsigned char SysSocketMulticastTTLType;
#endif

    //!
    //! Integer data type for the Type Of Service (TOS) socket option.
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSocketTOSType;
#elif defined(TS_WINDOWS)
    typedef ::DWORD SysSocketTOSType;
#elif defined(TS_UNIX)
    typedef int SysSocketTOSType;
#endif

    //!
    //! Integer data type for the multicast loop socket option.
    //! Example:
    //! @code
    //! SysSocketMulticastLoopType mloop = 1;
    //! if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, SysSockOptPointer(&mloop), sizeof(mloop)) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSocketMulticastLoopType;
#elif defined(TS_WINDOWS)
    typedef ::DWORD SysSocketMulticastLoopType;
#elif defined(TS_UNIX)
    typedef unsigned char SysSocketMulticastLoopType;
#endif

    //!
    //! Integer data type for the IP_PKTINFO socket option.
    //! Example:
    //! @code
    //! SysSocketPktInfoType state = 1;
    //! if (setsockopt(sock, IPPROTO_IP, IP_PKTINFO, SysSockOptPointer(&state), sizeof(state)) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSocketPktInfoType;
#elif defined(TS_WINDOWS)
    typedef ::DWORD SysSocketPktInfoType;
#elif defined(TS_UNIX)
    typedef int SysSocketPktInfoType;
#endif

    //!
    //! Integer data type for the field l_linger in the struct linger socket option.
    //! All systems do not use the same type size and this may generate some warnings.
    //! Example:
    //! @code
    //! struct linger lin;
    //! lin.l_linger = SysSocketLingerType(seconds);
    //! @endcode
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSocketLingerType;
#elif defined(TS_WINDOWS)
    typedef u_short SysSocketLingerType;
#elif defined(TS_UNIX)
    typedef int SysSocketLingerType;
#endif

    //!
    //! Pointer type for the address of a socket option value.
    //! The "standard" parameter type is @c void* but some systems use other exotic values.
    //! Example:
    //! @code
    //! SysSocketTTLType ttl = 10;
    //! if (setsockopt(sock, IPPROTO_IP, IP_TTL, SysSockOptPointer(&ttl), sizeof(ttl)) != 0) {
    //!     ... error processing ...
    //! }
    //! @endcode
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSockOptPointer;
#elif defined(TS_WINDOWS)
    typedef const char* SysSockOptPointer;
#elif defined(TS_UNIX)
    typedef void* SysSockOptPointer;
#endif

    //!
    //! Pointer type for the address of the data buffer for a recv() system call.
    //! The "standard" parameter type is @c void* but some systems use other exotic values.
    //! Example:
    //! @code
    //! SysSocketSignedSizeType got = recv(sock, SysRecvBufferPointer(&data), max_size, 0);
    //! @endcode
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysRecvBufferPointer;
#elif defined(TS_WINDOWS)
    typedef char* SysRecvBufferPointer;
#elif defined(TS_UNIX)
    typedef void* SysRecvBufferPointer;
#endif

    //!
    //! Pointer type for the address of the data buffer for a send() system call.
    //! The "standard" parameter type is @c void* but some systems use other exotic values.
    //! Example:
    //! @code
    //! SysSocketSignedSizeType gone = send(sock, SysSendBufferPointer(&data), SysSendSizeType(size), 0);
    //! @endcode
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSendBufferPointer;
#elif defined(TS_WINDOWS)
    typedef const char* SysSendBufferPointer;
#elif defined(TS_UNIX)
    typedef void* SysSendBufferPointer;
#endif

    //!
    //! Integer type for the size of the data buffer for a send() system call.
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysSendSizeType;
#elif defined(TS_WINDOWS)
    typedef int SysSendSizeType;
#elif defined(TS_UNIX)
    typedef size_t SysSendSizeType;
#endif

    //!
    //! Name of the option for the shutdown() system call which means "close on both directions".
    //! Example:
    //! @code
    //! shutdown(sock, SYS_SOCKET_SHUT_RDWR);
    //! @endcode
    //!
#if defined(DOXYGEN)
    constexpr int SYS_SOCKET_SHUT_RDWR = platform_specific;
#elif defined(TS_WINDOWS)
    constexpr int SYS_SOCKET_SHUT_RDWR = SD_BOTH;
#elif defined(TS_UNIX)
    constexpr int SYS_SOCKET_SHUT_RDWR = SHUT_RDWR;
#endif

    //!
    //! Name of the option for the shutdown() system call which means "close on receive side".
    //! Example:
    //! @code
    //! shutdown(sock, SYS_SOCKET_SHUT_RD);
    //! @endcode
    //!
#if defined(DOXYGEN)
    constexpr int SYS_SOCKET_SHUT_RD = platform_specific;
#elif defined(TS_WINDOWS)
    constexpr int SYS_SOCKET_SHUT_RD = SD_RECEIVE;
#elif defined(TS_UNIX)
    constexpr int SYS_SOCKET_SHUT_RD = SHUT_RD;
#endif

    //!
    //! Name of the option for the shutdown() system call which means "close on send side".
    //! Example:
    //! @code
    //! shutdown(sock, SYS_SOCKET_SHUT_WR);
    //! @endcode
    //!
#if defined(DOXYGEN)
    constexpr int SYS_SOCKET_SHUT_WR = platform_specific;
#elif defined(TS_WINDOWS)
    constexpr int SYS_SOCKET_SHUT_WR = SD_SEND;
#elif defined(TS_UNIX)
    constexpr int SYS_SOCKET_SHUT_WR = SHUT_WR;
#endif

    //!
    //! The close() system call which applies to socket devices.
    //! The "standard" name is @c close but some systems use other exotic names.
    //! @param [in] sock System socket descriptor.
    //! @return Error code.
    //!
    TSDUCKDLL inline int SysCloseSocket(SysSocketType sock)
    {
#if defined(TS_WINDOWS)
        return ::closesocket(sock);
#elif defined(TS_UNIX)
        return ::close(sock);
#else
        #error "Unsupported operating system"
#endif
    }

    //!
    //! Get the std::error_category for getaddrinfo() error code (Unix only).
    //! @return A constant reference to a std::error_category instance.
    //!
    TSDUCKDLL const std::error_category& getaddrinfo_category();

    //------------------------------------------------------------------------
    // Local network interfaces.
    //------------------------------------------------------------------------

    //!
    //! Get the list of all local IPv4 addresses in the system with their network masks.
    //! @param [out] addresses A vector of IPv4AddressMask which receives the list
    //! of all local IPv4 addresses in the system, except IPv4Address::LocalHost.
    //! @param [in] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool GetLocalIPAddresses(IPv4AddressMaskVector& addresses, Report& report = CERR);

    //!
    //! Get the list of all local IPv4 addresses in the system.
    //!
    //! @param [out] addresses A vector of IPv4Address which receives the list
    //! of all local IPv4 addresses in the system, except ts::IPv4Address::LocalHost.
    //! @param [in] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool GetLocalIPAddresses(IPv4AddressVector& addresses, Report& report = CERR);

    //!
    //! Check if a local system interface has a specified IP address.
    //!
    //! @param [in] address The IP address to check.
    //! @return True is @a address is the address of a local system interface, false otherwise.
    //!
    TSDUCKDLL bool IsLocalIPAddress(const IPv4Address& address);
}
