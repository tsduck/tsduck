//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPSocketAddress.h"
#include "tsUString.h"

// Wildcard socket address, unspecified address and port.
const ts::IPSocketAddress ts::IPSocketAddress::AnySocketAddress4(IP::v4);
const ts::IPSocketAddress ts::IPSocketAddress::AnySocketAddress6(IP::v6);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

// Generic constructor from a system "struct sockaddr" structure (IPv4 or IPv6).
ts::IPSocketAddress::IPSocketAddress(const ::sockaddr& s) :
    IPAddress(s)
{
    if (s.sa_family == AF_INET) {
        const ::sockaddr_in* sp = reinterpret_cast<const ::sockaddr_in*>(&s);
        _port = ntohs(sp->sin_port);
    }
    else if (s.sa_family == AF_INET6) {
        const ::sockaddr_in6* sp = reinterpret_cast<const ::sockaddr_in6*>(&s);
        _port = ntohs(sp->sin6_port);
    }
}

// IPv4 constructor from a system "struct sockaddr_in" structure.
ts::IPSocketAddress::IPSocketAddress(const ::sockaddr_in& s, bool bound) :
    IPAddress(s, bound),
    _port(s.sin_family == AF_INET ? ntohs(s.sin_port) : AnyPort)
{
}

// IPv6 constructor from a system "struct sockaddr_in6" structure.
ts::IPSocketAddress::IPSocketAddress(const ::sockaddr_in6& s, bool bound) :
    IPAddress(s, bound),
    _port(s.sin6_family == AF_INET6 ? ntohs(s.sin6_port) : AnyPort)
{
}

ts::IPSocketAddress::~IPSocketAddress()
{
}


//----------------------------------------------------------------------------
// AbstractNetworkAddress interface.
//----------------------------------------------------------------------------

ts::IPSocketAddress::Port ts::IPSocketAddress::port() const
{
    return _port;
}

void ts::IPSocketAddress::setPort(Port port)
{
    _port = port;
}


//----------------------------------------------------------------------------
// Set/get address (IP specific)
//----------------------------------------------------------------------------

void ts::IPSocketAddress::set(const ::sockaddr& s)
{
    setAddress(s);
    if (s.sa_family == AF_INET) {
        const ::sockaddr_in* sp = reinterpret_cast<const ::sockaddr_in*>(&s);
        _port = ntohs(sp->sin_port);
    }
    else if (s.sa_family == AF_INET6) {
        const ::sockaddr_in6* sp = reinterpret_cast<const ::sockaddr_in6*>(&s);
        _port = ntohs(sp->sin6_port);
    }
}


//----------------------------------------------------------------------------
// Decode a string "addr[:port]" or "[addr:]port".
//----------------------------------------------------------------------------

bool ts::IPSocketAddress::resolve(const UString& name, Report& report)
{
    // Clear address & port
    clear();

    // Locate last colon in string and square brackets.
    // Square brackets are used in IPv6 numerical address "[ipv6-address]:port".
    const size_t colon = name.rfind(':');
    const size_t br1 = name.find('[');
    const size_t br2 = name.rfind(']');
    bool ok = true;

    // Process the square bracket case.
    if (br1 == 0 && br2 < name.length()) {
        // This is typical IPv6 socket address. There must be a port or nothing.
        ok = br2 == name.size() - 1 || (colon == br2 + 1 && (colon == name.length() - 1 || name.substr(colon + 1).toInteger(_port)));
        if (ok) {
            return IPAddress::resolve(name.substr(br1 + 1, br2 - br1 - 1), report);
        }
    }
    else {
        // If this is not an IPv6 socket address syntax, no square bracket shall be present.
        ok = br1 == NPOS && br2 == NPOS;
    }
    if (!ok) {
        report.error(u"invalid socket address \"%s\"", name);
        return false;
    }

    // Without colon in the string, it can be an address alone or a port alone.
    if (colon == NPOS) {
        if (name.empty() || name.toInteger(_port)) {
            // Empty valid default address or an integer (a port alone).
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
        report.error(u"invalid port value in \"%s\"", name);
        return false;
    }

    // If there is something before the colon, this must be an address.
    // Try to decode name as IP address or resolve it as DNS host name.
    return colon == 0 || IPAddress::resolve(name.substr(0, colon), report);
}


//----------------------------------------------------------------------------
// Check if this address "matches" another one.
//----------------------------------------------------------------------------

bool ts::IPSocketAddress::match(const IPSocketAddress& other) const
{
    return IPAddress::match(other) && (_port == AnyPort || other._port == AnyPort || _port == other._port);
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::IPSocketAddress::toString() const
{
    UString str;
    if (generation() == IP::v6) {
        // IPv6 numeric addresses need square brackets around address because the address contains colons.
        str.format(u"[%s]", IPAddress::toString());
    }
    else {
        str = IPAddress::toString();
    }
    if (_port != AnyPort) {
        str.format(u":%d", _port);
    }
    return str;
}

ts::UString ts::IPSocketAddress::toFullString() const
{
    UString str;
    if (generation() == IP::v6) {
        // IPv6 numeric addresses need square brackets around address because the address contains colons.
        str.format(u"[%s]", IPAddress::toFullString());
    }
    else {
        str = IPAddress::toFullString();
    }
    if (_port != AnyPort) {
        str.format(u":%d", _port);
    }
    return str;
}


//----------------------------------------------------------------------------
// Comparison "less than" operator.
//----------------------------------------------------------------------------

bool ts::IPSocketAddress::operator<(const IPSocketAddress& other) const
{
    return IPAddress::operator<(other) || (IPAddress::operator==(other) && _port < other._port);
}
