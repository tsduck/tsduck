//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPv6SocketAddress.h"
#include "tsUString.h"

// Wildcard socket address, unspecified address and port.
const ts::IPv6SocketAddress ts::IPv6SocketAddress::AnySocketAddress;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::IPv6SocketAddress::~IPv6SocketAddress()
{
}


//----------------------------------------------------------------------------
// Get/set port
//----------------------------------------------------------------------------

ts::IPv6SocketAddress::Port ts::IPv6SocketAddress::port() const
{
    return _port;
}

void ts::IPv6SocketAddress::setPort(Port port)
{
    _port = port;
}


//----------------------------------------------------------------------------
// Decode a string "[addr]:port".
//----------------------------------------------------------------------------

bool ts::IPv6SocketAddress::resolve(const UString& name, Report& report)
{
    // Clear address & port
    clear();

    // Locate square brackets.
    const size_t br1 = name.find('[');
    const size_t br2 = name.rfind(']');

    if (br1 == NPOS && br2 == NPOS) {
        // No square brackets in string, can be an address alone or a port alone.
        if (name.toInteger(_port)) {
            // This is an integer, this is a port alone.
            return true;
        }
        else {
            // Not a valid integer, this is an address alone
            _port = AnyPort;
            return IPv6Address::resolve(name, report);
        }
    }

    // If there a square bracket, both of them must be present.
    bool ok = br1 == 0 && br2 != NPOS;

    // If there is something after the closing square bracket, it must be ":port".
    if (ok && br2 < name.length() - 1) {
        ok = name[br2 + 1] == ':' && (br2 == name.length() - 1 || name.substr(br2 + 2).toInteger(_port));
    }

    // Decode the IPv6 address.
    if (ok) {
        return IPv6Address::resolve(name.substr(1, br2 - 1), report);
    }
    else {
        report.error(u"invalid IPv6 socket address \"%s\"", {name});
        return false;
    }
}


//----------------------------------------------------------------------------
// Check if this address "matches" another one.
//----------------------------------------------------------------------------

bool ts::IPv6SocketAddress::match(const IPv6SocketAddress& other) const
{
    return IPv6Address::match(other) && (_port == AnyPort || other._port == AnyPort || _port == other._port);
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::IPv6SocketAddress::toString() const
{
    return _port == AnyPort ? IPv6Address::toString() : UString::Format(u"[%s]:%d", {IPv6Address::toString(), _port});
}

ts::UString ts::IPv6SocketAddress::toFullString() const
{
    return _port == AnyPort ? IPv6Address::toFullString() : UString::Format(u"[%s]:%d", {IPv6Address::toFullString(), _port});
}


//----------------------------------------------------------------------------
// Comparison "less than" operator.
//----------------------------------------------------------------------------

bool ts::IPv6SocketAddress::operator<(const IPv6SocketAddress& other) const
{
    return IPv6Address::operator<(other) || (IPv6Address::operator==(other) && _port < other._port);
}
