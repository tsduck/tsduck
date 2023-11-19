//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractNetworkAddress.h"
#include "tsUString.h"

// All methods here are default implementations,
// when a subclass does not implement the corresponding method.

ts::AbstractNetworkAddress::~AbstractNetworkAddress()
{
}

void ts::AbstractNetworkAddress::clear()
{
    clearAddress();
    clearPort();
}

bool ts::AbstractNetworkAddress::hasPort() const
{
    return port() != AnyPort;
}

ts::AbstractNetworkAddress::Port ts::AbstractNetworkAddress::port() const
{
    return AnyPort;
}

void ts::AbstractNetworkAddress::setPort(Port port)
{
}

void ts::AbstractNetworkAddress::clearPort()
{
    setPort(AnyPort);
}

ts::UString ts::AbstractNetworkAddress::toFullString() const
{
    return toString();
}
