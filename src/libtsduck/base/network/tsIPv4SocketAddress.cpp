//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPv4SocketAddress.h"
#include "tsUString.h"

// Wildcard socket address, unspecified address and port.
const ts::IPv4SocketAddress ts::IPv4SocketAddress::AnySocketAddress;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::IPv4SocketAddress::IPv4SocketAddress(const ::sockaddr& s) :
    IPv4Address(s),
    _port(AnyPort)
{
    if (s.sa_family == AF_INET) {
        assert(sizeof(::sockaddr) >= sizeof(::sockaddr_in));
        const ::sockaddr_in* sp = reinterpret_cast<const ::sockaddr_in*>(&s);
        _port = ntohs(sp->sin_port);
    }
}

ts::IPv4SocketAddress::IPv4SocketAddress(const ::sockaddr_in& s) :
    IPv4Address(s),
    _port(s.sin_family == AF_INET ? ntohs(s.sin_port) : AnyPort)
{
}

ts::IPv4SocketAddress::~IPv4SocketAddress()
{
}


//----------------------------------------------------------------------------
// Get/set port
//----------------------------------------------------------------------------

ts::IPv4SocketAddress::Port ts::IPv4SocketAddress::port() const
{
    return _port;
}

void ts::IPv4SocketAddress::setPort(Port port)
{
    _port = port;
}


//----------------------------------------------------------------------------
// Decode a string "addr[:port]" or "[addr:]port".
//----------------------------------------------------------------------------

bool ts::IPv4SocketAddress::resolve(const UString& name, Report& report)
{
    // Clear address & port
    clear();

    // Locate last colon in string
    const size_t colon = name.rfind(':');

    if (colon == NPOS) {
        // No colon in string, can be an address alone or a port alone.
        if (name.empty() || name.toInteger(_port)) {
            // Empty valid default address or an integer (a port alone).
            return true;
        }
        else {
            // Not a valid integer, this is an address alone
            _port = AnyPort;
            return IPv4Address::resolve(name, report);
        }
    }

    // If there is something after the colon, this must be a port number
    if (colon < name.length() - 1 && !name.substr(colon + 1).toInteger(_port)) {
        report.error(u"invalid port value in \"%s\"", {name});
        return false;
    }

    // If there is something before the colon, this must be an address.
    // Try to decode name as IP address or resolve it as DNS host name.
    return colon == 0 || IPv4Address::resolve(name.substr(0, colon), report);
}


//----------------------------------------------------------------------------
// Check if this address "matches" another one.
//----------------------------------------------------------------------------

bool ts::IPv4SocketAddress::match(const IPv4SocketAddress& other) const
{
    return IPv4Address::match(other) && (_port == AnyPort || other._port == AnyPort || _port == other._port);
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::IPv4SocketAddress::toString() const
{
    return IPv4Address::toString() + (_port == AnyPort ? u"" : UString::Format(u":%d", {_port}));
}


//----------------------------------------------------------------------------
// Comparison "less than" operator.
//----------------------------------------------------------------------------

bool ts::IPv4SocketAddress::operator<(const IPv4SocketAddress& other) const
{
    return address() < other.address() || (address() == other.address() && _port < other._port);
}
