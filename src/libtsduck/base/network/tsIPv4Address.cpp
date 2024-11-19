//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPv4Address.h"


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::IPv4Address::IPv4Address(const IPAddress& other) :
    IPAddress(IP::v4)
{
    setAddress(other);
}

ts::IPv4Address::IPv4Address(const uint8_t *addr, size_t size) :
    IPAddress(IP::v4)
{
    setAddress(addr, size);
}

ts::IPv4Address::IPv4Address(const ::sockaddr& a) :
    IPAddress(IP::v4)
{
    setAddress(a);
}

ts::IPv4Address::IPv4Address(const ::sockaddr_storage& a) :
    IPAddress(IP::v4)
{
    setAddress(a);
}

ts::IPv4Address::IPv4Address(const UString& name, Report& report) :
    IPAddress(IP::v4)
{
    resolve(name, report);
}

ts::IPv4Address::~IPv4Address()
{
}
