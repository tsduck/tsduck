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

#include "tsIPv4AddressMask.h"


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
