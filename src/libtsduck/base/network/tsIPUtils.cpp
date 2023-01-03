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

#include "tsIPUtils.h"
#include "tsIPv4Address.h"


//----------------------------------------------------------------------------
// Initialize IP usage. Shall be called once at least.
//----------------------------------------------------------------------------

bool ts::IPInitialize(Report& report)
{
#if defined(TS_WINDOWS)
    // Execute only once (except - harmless - race conditions during startup).
    static volatile bool done = false;
    if (!done) {
        // Request version 2.2 of Winsock
        ::WSADATA data;
        int err = ::WSAStartup(MAKEWORD(2, 2), &data);
        if (err != 0) {
            report.error(u"WSAStartup failed, WinSock error %X", {err});
            return false;
        }
        done = true;
    }
#endif

    return true;
}


//----------------------------------------------------------------------------
// Check if a local system interface has a specified IP address.
//----------------------------------------------------------------------------

bool ts::IsLocalIPAddress(const IPv4Address& address)
{
    IPv4AddressVector locals;
    return address == IPv4Address::LocalHost || (GetLocalIPAddresses(locals) && std::find(locals.begin(), locals.end(), address) != locals.end());
}


//----------------------------------------------------------------------------
// This method returns the list of all local IPv4 addresses in the system
// with their network mask.
//----------------------------------------------------------------------------

bool ts::GetLocalIPAddresses(IPv4AddressMaskVector& list, Report& report)
{
    bool status = true;
    list.clear();

#if defined(TS_MAC) || defined(TS_BSD)

    // Get the list of local addresses. The memory is allocated by getifaddrs().
    ::ifaddrs* start = nullptr;
    if (::getifaddrs(&start) != 0) {
        report.error(u"error getting local addresses: %s", {SysErrorCodeMessage()});
        return false;
    }

    // Browse the list of interfaces.
    for (::ifaddrs* ifa = start; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr != nullptr) {
            IPv4Address addr(*ifa->ifa_addr);
            if (addr.hasAddress() && addr != IPv4Address::LocalHost) {
                list.push_back(IPv4AddressMask(addr, IPv4Address(*ifa->ifa_netmask)));
            }
        }
    }

    // Free the system-allocated memory.
    ::freeifaddrs(start);

#elif defined(TS_WINDOWS) || defined(TS_UNIX)

    // Create a socket to query the system on
    SysSocketType sock = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == SYS_SOCKET_INVALID) {
        report.error(u"error creating socket: %s", {SysSocketErrorCodeMessage()});
        return false;
    }

#if defined(TS_WINDOWS)

    // Windows implementation

    ::INTERFACE_INFO info[32];  // max 32 local interface (arbitrary)
    ::DWORD retsize;
    if (::WSAIoctl(sock, SIO_GET_INTERFACE_LIST, 0, 0, info, ::DWORD(sizeof(info)), &retsize, 0, 0) != 0) {
        report.error(u"error getting local addresses: %s", {SysSocketErrorCodeMessage()});
        status = false;
    }
    else {
        retsize = std::max<::DWORD>(0, std::min(retsize, ::DWORD(sizeof(info))));
        size_t count = retsize / sizeof(::INTERFACE_INFO);
        for (size_t i = 0; i < count; ++i) {
            IPv4Address addr(info[i].iiAddress.Address);
            if (addr.hasAddress() && addr != IPv4Address::LocalHost) {
                list.push_back(IPv4AddressMask(addr, IPv4Address(info[i].iiNetmask.Address)));
            }
        }
    }

#else

    // UNIX implementation (may not work on all UNIX, works on Linux)

    ::ifconf ifc;
    ::ifreq info[32];  // max 32 local interface (arbitrary)

    ifc.ifc_req = info;
    ifc.ifc_len = sizeof(info);

    if (::ioctl(sock, SIOCGIFCONF, &ifc) != 0) {
        report.error(u"error getting local addresses: %s", {SysSocketErrorCodeMessage()});
        status = false;
    }
    else {
        ifc.ifc_len = std::max(0, std::min(ifc.ifc_len, int(sizeof(info))));
        size_t count = ifc.ifc_len / sizeof(::ifreq);
        for (size_t i = 0; i < count; ++i) {
            IPv4Address addr(info[i].ifr_addr);
            IPv4Address mask;
            if (addr.hasAddress() && addr != IPv4Address::LocalHost) {
                // Get network mask for this interface.
                ::ifreq req;
                req = info[i];
                if (::ioctl(sock, SIOCGIFNETMASK, &req) != 0) {
                    const SysSocketErrorCode err = LastSysSocketErrorCode();
                    report.error(u"error getting network mask for %s: %s", {addr, SysSocketErrorCodeMessage(err)});
                }
                else {
                    mask = IPv4Address(req.ifr_netmask);
                }
                list.push_back(IPv4AddressMask(addr, mask));
            }
        }
    }

#endif

    // Close socket
    SysCloseSocket(sock);

#else

    report.error(u"getting local addresses is not implemented");
    status = false;

#endif

    return status;
}


//----------------------------------------------------------------------------
// This method returns the list of all local IPv4 addresses in the system
//----------------------------------------------------------------------------

bool ts::GetLocalIPAddresses(IPv4AddressVector& list, Report& report)
{
    IPv4AddressMaskVector full_list;
    list.clear();

    if (GetLocalIPAddresses(full_list, report)) {
        list.resize(full_list.size());
        for (size_t i = 0; i < full_list.size(); ++i) {
            list[i] = full_list[i].address;
        }
        return true;
    }
    else {
        return false;
    }
}
