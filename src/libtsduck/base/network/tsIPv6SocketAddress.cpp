//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPv6SocketAddress.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::IPv6SocketAddress::IPv6SocketAddress(const IPAddress& addr, Port port) :
    IPSocketAddress(IP::v6)
{
    setAddress(addr);
    setPort(port);
}

ts::IPv6SocketAddress::IPv6SocketAddress(const uint8_t *addr, size_t size, Port port) :
    IPSocketAddress(IP::v6)
{
    setAddress(addr, size);
    setPort(port);
}

ts::IPv6SocketAddress::IPv6SocketAddress(const ::sockaddr& a) :
    IPSocketAddress(IP::v6)
{
    set(a);
}

ts::IPv6SocketAddress::IPv6SocketAddress(const ::sockaddr_storage& a) :
    IPSocketAddress(IP::v6)
{
    set(a);
}

ts::IPv6SocketAddress::IPv6SocketAddress(const UString& name, Report& report) :
    IPSocketAddress(IP::v6)
{
    resolve(name, report);
}

ts::IPv6SocketAddress::~IPv6SocketAddress()
{
}
