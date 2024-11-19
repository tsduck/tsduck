//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPv4SocketAddress.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::IPv4SocketAddress::IPv4SocketAddress(const IPAddress& addr, Port port) :
    IPSocketAddress(IP::v4)
{
    setAddress(addr);
    setPort(port);
}

ts::IPv4SocketAddress::IPv4SocketAddress(const uint8_t *addr, size_t size, Port port) :
    IPSocketAddress(IP::v4)
{
    setAddress(addr, size);
    setPort(port);
}

ts::IPv4SocketAddress::IPv4SocketAddress(const ::sockaddr& a) :
    IPSocketAddress(IP::v4)
{
    set(a);
}

ts::IPv4SocketAddress::IPv4SocketAddress(const ::sockaddr_storage& a) :
    IPSocketAddress(IP::v4)
{
    set(a);
}

ts::IPv4SocketAddress::IPv4SocketAddress(const UString& name, Report& report) :
    IPSocketAddress(IP::v4)
{
    resolve(name, report);
}

ts::IPv4SocketAddress::~IPv4SocketAddress()
{
}
