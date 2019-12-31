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

#include "tsSocketAddress.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SocketAddress::SocketAddress(const ::sockaddr& s) :
    IPAddress(s),
    _port(AnyPort)
{
    if (s.sa_family == AF_INET) {
        assert(sizeof(::sockaddr) >= sizeof(::sockaddr_in));
        const ::sockaddr_in* sp = reinterpret_cast<const ::sockaddr_in*>(&s);
        _port = ntohs(sp->sin_port);
    }
}

ts::SocketAddress::SocketAddress(const ::sockaddr_in& s) :
    IPAddress(s),
    _port(s.sin_family == AF_INET ? ntohs(s.sin_port) : AnyPort)
{
}

ts::SocketAddress::~SocketAddress()
{
}


//----------------------------------------------------------------------------
// Decode a string "addr[:port]" or "[addr:]port".
// Addr can also be a hostname which is resolved.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SocketAddress::resolve(const UString& name, Report& report)
{
    // Clear address & port
    clear();

    // Locate last colon in string
    UString::size_type colon = name.rfind(u":");

    if (colon == NPOS) {
        // No colon in string, can be an address alone or a port alone.
        if (name.toInteger(_port)) {
            // This is an integer, this is a port alone.
            return true;
        }
        else {
            // Not a valid integer, this is an address alone
            _port = AnyPort;
            return IPAddress::resolve(name, report);
        }
    }

    // If there is something after the colon, this must be a port number
    if (colon < name.length() - 1 && !name.substr(colon + 1).toInteger(_port)) {
        report.error(u"invalid port value in \"%s\"", {name});
        return false;
    }

    // If there is something before the colon, this must be an address.
    // Try to decode name as IP address or resolve it as DNS host name.
    return colon == 0 || IPAddress::resolve(name.substr(0, colon), report);
}


//----------------------------------------------------------------------------
// Check if this address "matches" another one.
//----------------------------------------------------------------------------

bool ts::SocketAddress::match(const SocketAddress& other) const
{
    return IPAddress::match(other) && (_port == AnyPort || other._port == AnyPort || _port == other._port);
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::SocketAddress::toString() const
{
    return IPAddress::toString() + (_port == AnyPort ? u"" : UString::Format(u":%d", {_port}));
}


//----------------------------------------------------------------------------
// Comparison "less than" operator.
//----------------------------------------------------------------------------

bool ts::SocketAddress::operator<(const SocketAddress& other) const
{
    return address() < other.address() || (address() == other.address() && _port < other._port);
}
