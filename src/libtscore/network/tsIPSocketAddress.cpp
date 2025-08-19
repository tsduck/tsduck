//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPSocketAddress.h"
#include "tsUString.h"

// Wildcard socket address, unspecified address and port.
const ts::IPSocketAddress ts::IPSocketAddress::AnySocketAddress4;
const ts::IPSocketAddress ts::IPSocketAddress::AnySocketAddress6(0, 0, 0, 0, 0, 0, 0, 0, AnyPort);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

// Destructor.
ts::IPSocketAddress::~IPSocketAddress()
{
}

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

bool ts::IPSocketAddress::set(const ::sockaddr& s)
{
    const bool ok = setAddress(s);
    if (!ok) {
        _port = 0;
    }
    else if (s.sa_family == AF_INET) {
        const ::sockaddr_in* sp = reinterpret_cast<const ::sockaddr_in*>(&s);
        _port = ntohs(sp->sin_port);
    }
    else if (s.sa_family == AF_INET6) {
        const ::sockaddr_in6* sp = reinterpret_cast<const ::sockaddr_in6*>(&s);
        _port = ntohs(sp->sin6_port);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Remove the port number from a "addr[:port]" or "[addr:]port" string.
//----------------------------------------------------------------------------

void ts::IPSocketAddress::RemovePort(UString& name)
{
    const size_t colon = name.rfind(':');
    const size_t br2 = name.rfind(']');

    if (colon == NPOS && br2 == NPOS) {
        // No colon. If the string is only digits, this is a port alone. Otherwise this is a host name alone.
        for (UChar c : name) {
            if (!IsDigit(c)) {
                return;
            }
        }
        // Only digits => port alone.
        name.clear();
    }
    else if (colon != NPOS && (br2 == NPOS || br2 < colon)) {
        // There is a port.
        name.resize(colon);
    }
}


//----------------------------------------------------------------------------
// Decode a string "addr[:port]" or "[addr:]port".
//----------------------------------------------------------------------------

bool ts::IPSocketAddress::resolve(const UString& name, Report& report)
{
    return resolve(name, report, IP::Any);
}

bool ts::IPSocketAddress::resolve(const UString& name, Report& report, IP preferred)
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
            return IPAddress::resolve(name.substr(br1 + 1, br2 - br1 - 1), report, preferred);
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
            return IPAddress::resolve(name, report, preferred);
        }
    }

    // If there is something after the colon, this must be a port number
    if (colon < name.length() - 1 && !name.substr(colon + 1).toInteger(_port)) {
        report.error(u"invalid port value in \"%s\"", name);
        return false;
    }

    // If there is something before the colon, this must be an address.
    // Try to decode name as IP address or resolve it as DNS host name.
    return colon == 0 || IPAddress::resolve(name.substr(0, colon), report, preferred);
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
    if (_port == AnyPort) {
        return IPAddress::toString();
    }
    else if (generation() == IP::v6) {
        // IPv6 numeric addresses need square brackets around address because the address contains colons.
        return UString::Format(u"[%s]:%d", IPAddress::toString(), _port);
    }
    else {
        return UString::Format(u"%s:%d", IPAddress::toString(), _port);
    }
}

ts::UString ts::IPSocketAddress::toFullString() const
{
    if (_port == AnyPort) {
        return IPAddress::toFullString();
    }
    else if (generation() == IP::v6) {
        // IPv6 numeric addresses need square brackets around address because the address contains colons.
        return UString::Format(u"[%s]:%d", IPAddress::toFullString(), _port);
    }
    else {
        return UString::Format(u"%s:%d", IPAddress::toFullString(), _port);
    }
}


//----------------------------------------------------------------------------
// Comparison "less than" operator.
//----------------------------------------------------------------------------

bool ts::IPSocketAddress::operator<(const IPSocketAddress& other) const
{
    return IPAddress::operator<(other) || (IPAddress::operator==(other) && _port < other._port);
}
