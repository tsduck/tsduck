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
//!
//!  @file
//!  @ingroup net
//!  Utilities for IP networking
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCerrReport.h"
#include "tsSysUtils.h"
#include "tsIPAddress.h"
#include "tsIPAddressMask.h"
#include "tsIPv6Address.h"

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
    TSDUCKDLL bool IPInitialize(Report& = CERR);

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
    TSDUCKDLL inline UString SocketErrorCodeMessage(SocketErrorCode code = LastSocketErrorCode())
    {
        // Note for windows: although error codes types are different for system and
        // winsock, it appears that the system error message also applies to winsock.
        return ErrorCodeMessage(code);
    }

    //!
    //! Get the list of all local IPv4 addresses in the system with their network masks.
    //!
    //! @param [out] addresses A vector of IPAddressMask which receives the list
    //! of all local IPv4 addresses in the system, except @link ts::IPAddress::LocalHost @endlink.
    //! @param [in] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool GetLocalIPAddresses(IPAddressMaskVector& addresses, Report& report = CERR);

    //!
    //! Get the list of all local IPv4 addresses in the system.
    //!
    //! @param [out] addresses A vector of IPAddress which receives the list
    //! of all local IPv4 addresses in the system, except @link ts::IPAddress::LocalHost @endlink.
    //! @param [in] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool GetLocalIPAddresses(IPAddressVector& addresses, Report& report = CERR);

    //!
    //! Check if a local system interface has a specified IP address.
    //!
    //! @param [in] address The IP address to check.
    //! @return True is @a address is the address of a local system interface, false otherwise.
    //!
    TSDUCKDLL bool IsLocalIPAddress(const IPAddress& address);

    //------------------------------------------------------------------------
    // Internals of the IPv4 protocol.
    //------------------------------------------------------------------------

    constexpr uint8_t IPv4_VERSION          =     4;   //!< Protocol version of IPv4 is ... 4 !
    constexpr size_t  IPv4_PROTOCOL_OFFSET  =     9;   //!< Offset of the protocol identifier in an IPv4 header.
    constexpr size_t  IPv4_CHECKSUM_OFFSET  =    10;   //!< Offset of the checksum in an IPv4 header.
    constexpr size_t  IPv4_SRC_ADDR_OFFSET  =    12;   //!< Offset of source IP address in an IPv4 header.
    constexpr size_t  IPv4_DEST_ADDR_OFFSET =    16;   //!< Offset of destination IP address in an IPv4 header.
    constexpr size_t  IPv4_MIN_HEADER_SIZE  =    20;   //!< Minimum size of an IPv4 header.
    constexpr size_t  UDP_HEADER_SIZE       =     8;   //!< Size of a UDP header.
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

    //------------------------------------------------------------------------
    // Real-time Transport Protocol (RTP)
    //------------------------------------------------------------------------

    constexpr size_t   RTP_HEADER_SIZE =    12;  //!< Size in bytes of the fixed part of the RTP header.
    constexpr uint8_t  RTP_PT_MP2T     =    33;  //!< RTP payload type for MPEG2-TS.
    constexpr uint64_t RTP_RATE_MP2T   = 90000;  //!< RTP clock rate for MPEG2-TS.
}
