//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPv4AddressMask.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::IPv4AddressMask::IPv4AddressMask(const IPv4Address& a, const IPv4Address& m) :
    address(a),
    mask(m)
{
}


//----------------------------------------------------------------------------
// Get the network mask size in bits.
//----------------------------------------------------------------------------

int ts::IPv4AddressMask::maskSize() const
{
    int size = 0;
    for (uint32_t m = mask.address(); m != 0; m = m << 1) {
        size++;
    }
    return size;
}


//----------------------------------------------------------------------------
// Get the associated broadcast address.
//----------------------------------------------------------------------------

ts::IPv4Address ts::IPv4AddressMask::broadcastAddress() const
{
    return IPv4Address(address.address() | ~mask.address());
}


//----------------------------------------------------------------------------
// Convert to a string object in numeric format "a.b.c.d".
//----------------------------------------------------------------------------

ts::UString ts::IPv4AddressMask::toString() const
{
    return UString::Format(u"%s/%d", {address, maskSize()});
}
