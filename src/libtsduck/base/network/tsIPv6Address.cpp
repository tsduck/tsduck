//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPv6Address.h"


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::IPv6Address::IPv6Address(const IPAddress& other) :
    IPAddress(IP::v6)
{
    setAddress(other);
}

ts::IPv6Address::IPv6Address(const uint8_t *addr, size_t size) :
    IPAddress(IP::v6)
{
    setAddress(addr, size);
}

ts::IPv6Address::IPv6Address(const ::sockaddr& a) :
    IPAddress(IP::v6)
{
    setAddress(a);
}

ts::IPv6Address::IPv6Address(const ::sockaddr_storage& a) :
    IPAddress(IP::v6)
{
    setAddress(a);
}

ts::IPv6Address::IPv6Address(const UString& name, Report& report) :
    IPAddress(IP::v6)
{
    resolve(name, report);
}

ts::IPv6Address::~IPv6Address()
{
}
