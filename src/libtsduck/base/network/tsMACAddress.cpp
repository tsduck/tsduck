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

#include "tsMACAddress.h"
#include "tsMemory.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MACAddress::MACAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6) :
    _addr((uint64_t(b1) << 40) | (uint64_t(b2) << 32) | (uint64_t(b3) << 24) | (uint64_t(b4) << 16) | (uint64_t(b5) << 8) | uint64_t(b6))
{
}


//----------------------------------------------------------------------------
// Set/get address
//----------------------------------------------------------------------------

void ts::MACAddress::setAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6)
{
    _addr = (uint64_t(b1) << 40) | (uint64_t(b2) << 32) | (uint64_t(b3) << 24) | (uint64_t(b4) << 16) | (uint64_t(b5) << 8) | uint64_t(b6);
}

void ts::MACAddress::getAddress(uint8_t& b1, uint8_t& b2, uint8_t& b3, uint8_t& b4, uint8_t& b5, uint8_t& b6) const
{
    b1 = uint8_t(_addr >> 40);
    b2 = uint8_t(_addr >> 32);
    b3 = uint8_t(_addr >> 24);
    b4 = uint8_t(_addr >> 16);
    b5 = uint8_t(_addr >> 8);
    b6 = uint8_t(_addr);
}


//----------------------------------------------------------------------------
// Decode a string or hostname which is resolved.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::MACAddress::resolve(const UString& name, Report& report)
{
    // Replace all separators with spaces.
    UString s(name);
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == u':' || s[i] == u'-' || s[i] == u'.') {
            s[i] = u' ';
        }
    }

    uint8_t b1, b2, b3, b4, b5, b6;
    if (s.scan(u"%x %x %x %x %x %x", {&b1, &b2, &b3, &b4, &b5, &b6})) {
        setAddress(b1, b2, b3, b4, b5, b6);
        return true;
    }
    else {
        report.error(u"invalid MAC address '%s', use format 'xx:xx:xx:xx:xx:xx'", {name});
        _addr = 0;
        return false;
    }
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::MACAddress::toString() const
{
    return UString::Format(u"%02X:%02X:%02X:%02X:%02X:%02X",
                {(_addr >> 40) & 0xFF, (_addr >> 32) & 0xFF, (_addr >> 24) & 0xFF, (_addr >> 16) & 0xFF, (_addr >> 8) & 0xFF, _addr & 0xFF});
}


//----------------------------------------------------------------------------
// Get the multicast MAC address for a given IPv4 address.
//----------------------------------------------------------------------------

bool ts::MACAddress::toMulticast(const IPAddress& ip)
{
    if (ip.isMulticast()) {
        _addr = MULTICAST_PREFIX | (ip.address() & ~MULTICAST_MASK);
        return true;
    }
    else {
        clear();
        return false;
    }
}
