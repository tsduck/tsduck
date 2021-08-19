//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  @ingroup net
//!  Utilities for IP networking
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCerrReport.h"
#include "tsSysUtils.h"
#include "tsIPv4Address.h"
#include "tsIPv4AddressMask.h"
#include "tsIPv6Address.h"
#include "tsIPv4SocketAddress.h"

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
    //! Type for socket error codes as returned by system calls.
    //!
    typedef int SysSocketErrorCode;

    //!
    //! System error code value meaning "connection reset by peer".
    //!
#if defined(DOXYGEN)
    constexpr SysSocketErrorCode SYS_SOCKET_ERR_RESET = platform_specific;
#elif defined(TS_WINDOWS)
    constexpr SysSocketErrorCode SYS_SOCKET_ERR_RESET = WSAECONNRESET;
#elif defined(TS_UNIX)
    constexpr SysSocketErrorCode SYS_SOCKET_ERR_RESET = EPIPE;
#endif

    //!
    //! System error code value meaning "peer socket not connected".
    //!
#if defined(DOXYGEN)
    constexpr SysSocketErrorCode SYS_SOCKET_ERR_NOTCONN = platform_specific;
#elif defined(TS_WINDOWS)
    constexpr SysSocketErrorCode SYS_SOCKET_ERR_NOTCONN = WSAENOTCONN;
#elif defined(TS_UNIX)
    constexpr SysSocketErrorCode SYS_SOCKET_ERR_NOTCONN = ENOTCONN;
#endif

    //!
    //! Get the error code of the last socket system call.
    //! The validity of the returned value may depends on specific conditions.
    //! @return The error code of the last socket system call.
    //!
    TSDUCKDLL inline SysSocketErrorCode LastSysSocketErrorCode()
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
    //! @param [in] code An error code from the operating system.
    //! Typically a result from LastSysSocketErrorCode().
    //! @return A string describing the error.
    //!
    TSDUCKDLL inline UString SysSocketErrorCodeMessage(SysSocketErrorCode code = LastSysSocketErrorCode())
    {
        // Note for windows: although error codes types are different for system and
        // winsock, it appears that the system error message also applies to winsock.
        return SysErrorCodeMessage(code);
    }

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
    TSDUCKDLL inline SysSocketErrorCode SysCloseSocket(SysSocketType sock)
    {
#if defined(TS_WINDOWS)
        return ::closesocket(sock);
#elif defined(TS_UNIX)
        return ::close(sock);
#else
        #error "Unsupported operating system"
#endif
    }

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

    //------------------------------------------------------------------------
    // Internals of the IPv4 protocol.
    //------------------------------------------------------------------------

    constexpr uint8_t IPv4_VERSION          =     4;   //!< Protocol version of IPv4 is ... 4 !
    constexpr size_t  IPv4_PROTOCOL_OFFSET  =     9;   //!< Offset of the protocol identifier in an IPv4 header.
    constexpr size_t  IPv4_CHECKSUM_OFFSET  =    10;   //!< Offset of the checksum in an IPv4 header.
    constexpr size_t  IPv4_SRC_ADDR_OFFSET  =    12;   //!< Offset of source IP address in an IPv4 header.
    constexpr size_t  IPv4_DEST_ADDR_OFFSET =    16;   //!< Offset of destination IP address in an IPv4 header.
    constexpr size_t  IPv4_MIN_HEADER_SIZE  =    20;   //!< Minimum size of an IPv4 header.
    constexpr size_t  IP_MAX_PACKET_SIZE    = 65536;   //!< Maximum size of an IP packet.

    //!
    //! Selected IP protocol identifiers.
    //!
    enum : uint8_t {
        IPv4_PROTO_ICMP = 1,   //!< Protocol identifier for ICMP.
        IPv4_PROTO_IGMP = 2,   //!< Protocol identifier for IGMP.
        IPv4_PROTO_TCP  = 6,   //!< Protocol identifier for TCP.
        IPv4_PROTO_UDP  = 17,  //!< Protocol identifier for UDP.
    };

    //!
    //! Get the size in bytes of an IPv4 header.
    //!
    //! @param [in] data Address of the IP packet.
    //! @param [in] size Size of the IP packet or header (must be larger than the header size).
    //! @return The size in bytes of the IP header or zero on error.
    //!
    TSDUCKDLL size_t IPHeaderSize(const void* data, size_t size);

    //!
    //! Compute the checksum of an IPv4 header.
    //!
    //! @param [in] data Address of the IP packet.
    //! @param [in] size Size of the IP packet or header (must be larger than the header size).
    //! @return The computed checksum of the header.
    //!
    TSDUCKDLL uint16_t IPHeaderChecksum(const void* data, size_t size);

    //!
    //! Verify the checksum of an IPv4 header.
    //!
    //! @param [in] data Address of the IP packet.
    //! @param [in] size Size of the IP packet or header (must be larger than the header size).
    //! @return True if the checksum of the header if correct, false otherwise.
    //!
    TSDUCKDLL bool VerifyIPHeaderChecksum(const void* data, size_t size);

    //!
    //! Update the checksum of an IPv4 header.
    //!
    //! @param [in,out] data Address of the IP packet.
    //! @param [in] size Size of the IP packet or header (must be larger than the header size).
    //! @return True if the checksum was update, false on incorrect buffer.
    //!
    TSDUCKDLL bool UpdateIPHeaderChecksum(void* data, size_t size);

    //!
    //! Validate and analyze an IPv4 packet.
    //! @param [in] data Address of the IP packet.
    //! @param [in] size Size of the IP packet.
    //! @param [out] protocol Protocol number (IPv4_PROTO_TCP, IPv4_PROTO_UDP, etc.)
    //! @param [out] ip_header_size Size of the IP header.
    //! @param [out] protocol_header_size Size of the protocol header (zero if neither TCP nor UDP).
    //! @param [out] source Source IPv4 address. The port is present for TCP or UDP only.
    //! @param [out] destination Destination IPv4 address. The port is present for TCP or UDP only.
    //! @return True if the packet is valid, false otherwise.
    //!
    TSDUCKDLL bool AnalyzeIPPacket(const void* data,
                                   size_t size,
                                   uint8_t& protocol,
                                   size_t& ip_header_size,
                                   size_t& protocol_header_size,
                                   IPv4SocketAddress& source,
                                   IPv4SocketAddress& destination);

    //------------------------------------------------------------------------
    // Ethernet II link layer.
    //------------------------------------------------------------------------

    constexpr size_t ETHER_DEST_ADDR_OFFSET =  0;   //!< Offset of destination MAC address in an Ethernet II header.
    constexpr size_t ETHER_SRC_ADDR_OFFSET  =  6;   //!< Offset of destination MAC address in an Ethernet II header.
    constexpr size_t ETHER_TYPE_OFFSET      = 12;   //!< Offset of destination MAC address in an Ethernet II header.
    constexpr size_t ETHER_HEADER_SIZE      = 14;   //!< Size of an Ethernet II header.
    constexpr size_t ETHER_ADDR_SIZE        =  6;   //!< Size in bytes of a MAC address in an Ethernet II header.
    constexpr size_t ETHER_CRC_SIZE         =  4;   //!< Size in bytes of the trailing CRC in an Ethernet II frame.

    //!
    //! Selected Ethernet II protocol type identifiers.
    //! @see https://en.wikipedia.org/wiki/EtherType
    //!
    enum : uint16_t {
        ETHERTYPE_IPv4   = 0x0800,  //!< Protocol identifier for IPv4.
        ETHERTYPE_ARP    = 0x0806,  //!< Protocol identifier for ARP.
        ETHERTYPE_WOL    = 0x0842,  //!< Protocol identifier for Wake-on-LAN.
        ETHERTYPE_RARP   = 0x8035,  //!< Protocol identifier for RARP.
        ETHERTYPE_802_1Q = 0x8100,  //!< Protocol identifier for a 2-byte IEEE 802.1Q tag (VLAN) after EtherType, then real EtherType.
        ETHERTYPE_IPv6   = 0x86DD,  //!< Protocol identifier for IPv6.
    };

    //------------------------------------------------------------------------
    // User Datagram Protocol (UDP)
    //------------------------------------------------------------------------

    constexpr size_t UDP_SRC_PORT_OFFSET  = 0;   //!< Offset of source port in a UDP header.
    constexpr size_t UDP_DEST_PORT_OFFSET = 2;   //!< Offset of destination port in a UDP header.
    constexpr size_t UDP_LENGTH_OFFSET    = 4;   //!< Offset of packet length (UDP header + UDP payload) in a UDP header.
    constexpr size_t UDP_CHECKSUM_OFFSET  = 6;   //!< Offset of checksum in a UDP header.
    constexpr size_t UDP_HEADER_SIZE      = 8;   //!< Size of a UDP header.

    //------------------------------------------------------------------------
    // User Datagram Protocol (TCP)
    //------------------------------------------------------------------------

    constexpr size_t TCP_SRC_PORT_OFFSET      =  0;   //!< Offset of source port in a TCP header.
    constexpr size_t TCP_DEST_PORT_OFFSET     =  2;   //!< Offset of destination port in a TCP header.
    constexpr size_t TCP_HEADER_LENGTH_OFFSET = 12;   //!< Offset of TCP header length in a TCP header (number of 32-bit words).
    constexpr size_t TCP_MIN_HEADER_SIZE      = 20;   //!< Minimum size in bytes of a TCP header.

    //------------------------------------------------------------------------
    // Real-time Transport Protocol (RTP)
    //------------------------------------------------------------------------

    constexpr size_t   RTP_HEADER_SIZE =    12;  //!< Size in bytes of the fixed part of the RTP header.
    constexpr uint8_t  RTP_PT_MP2T     =    33;  //!< RTP payload type for MPEG2-TS.
    constexpr uint64_t RTP_RATE_MP2T   = 90000;  //!< RTP clock rate for MPEG2-TS.
}
