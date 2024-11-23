//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IPv6 Socket address class (IP v6 address & port).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPSocketAddress.h"
#include "tsIPv6Address.h"

namespace ts {

    class IPv6SocketAddress;
    using IPv6SocketAddressVector = std::vector<IPv6SocketAddress>;  //!< Vector of socket addresses.
    using IPv6SocketAddressSet = std::set<IPv6SocketAddress>;        //!< Set of socket addresses.

    //!
    //! IP v6 socket address class (IP v6 address & port).
    //! This class is a subclass of IPSocketAddress where all instances are bound to IPv6.
    //! @ingroup net
    //!
    class TSDUCKDLL IPv6SocketAddress: public IPSocketAddress
    {
        TS_RULE_OF_FIVE(IPv6SocketAddress, override);
    public:

        // All constructors directly redirect to the corresponding IPSocketAddress constructor for IPv6.
        // They are not documented again here. All other operations are inherited from IPSocketAddress.

        //! @cond nodoxygen
        IPv6SocketAddress() : IPSocketAddress(IP::v6) {}
        IPv6SocketAddress(const IPAddress& addr, Port port = AnyPort);
        IPv6SocketAddress(const uint8_t *addr, size_t size, Port port = AnyPort);
        IPv6SocketAddress(const ByteBlock& bb, Port port = AnyPort) : IPv6SocketAddress(bb.data(), bb.size(), port) {}
        IPv6SocketAddress(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8, Port port = AnyPort) :
            IPSocketAddress(h1, h2, h3, h4, h5, h6, h7, h8, port, true) {}
        IPv6SocketAddress(uint64_t net, uint64_t ifid, Port port = AnyPort) : IPSocketAddress(net, ifid, port, true) {}
        IPv6SocketAddress(const ::sockaddr& a);
        IPv6SocketAddress(const ::sockaddr_storage& a);
        IPv6SocketAddress(const ::in6_addr& a, Port port = AnyPort) : IPSocketAddress(a, port, true) {}
        IPv6SocketAddress(const ::sockaddr_in6& a) : IPSocketAddress(a, true) {}
        IPv6SocketAddress(const UString& name, Report& report);
        //! @endcond
    };
}
