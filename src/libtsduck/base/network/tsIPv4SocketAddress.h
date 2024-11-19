//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IPv4 socket address class (IP v4 address & port).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPSocketAddress.h"
#include "tsIPv4Address.h"

namespace ts {

    class IPv4SocketAddress;
    using IPv4SocketAddressVector = std::vector<IPv4SocketAddress>;  //!< Vector of socket addresses.
    using IPv4SocketAddressSet = std::set<IPv4SocketAddress>;        //!< Set of socket addresses.

    //!
    //! IPv4 socket address class (IP v4 address & port).
    //! This class is a subclass of IPSocketAddress where all instances are bound to IPv4.
    //! @ingroup net
    //!
    class TSDUCKDLL IPv4SocketAddress: public IPSocketAddress
    {
        TS_RULE_OF_FIVE(IPv4SocketAddress, override);
    public:

        // All constructors directly redirect to the corresponding IPSocketAddress constructor for IPv4.
        // They are not documented again here. All other operations are inherited from IPSocketAddress.

        //! @cond nodoxygen
        IPv4SocketAddress() : IPSocketAddress(IP::v4) {}
        IPv4SocketAddress(const IPAddress& addr, Port port = AnyPort);
        IPv4SocketAddress(const uint8_t *addr, size_t size, Port port = AnyPort);
        IPv4SocketAddress(const ByteBlock& bb, Port port = AnyPort) : IPv4SocketAddress(bb.data(), bb.size(), port) {}
        IPv4SocketAddress(uint32_t addr, Port port = AnyPort) : IPSocketAddress(addr, port, true) {}
        IPv4SocketAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, Port port = AnyPort) : IPSocketAddress(b1, b2, b3, b4, port, true) {}
        IPv4SocketAddress(const ::sockaddr& s);
        IPv4SocketAddress(const ::sockaddr_storage& s);
        IPv4SocketAddress(const ::in_addr& a, Port port = AnyPort) : IPSocketAddress(a, port, true) {}
        IPv4SocketAddress(const ::sockaddr_in& s) : IPSocketAddress(s, true) {}
        IPv4SocketAddress(const UString& name, Report& report);
        //! @endcond
    };
}
