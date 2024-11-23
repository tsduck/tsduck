//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IP v6 address class
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPAddress.h"

namespace ts {

    class IPv6Address;
    using IPv6AddressVector = std::vector<IPv6Address>;  //!< Vector of IPv6 addresses.
    using IPv6AddressSet = std::set<IPv6Address>;        //!< Set of IPv6 addresses.

    //!
    //! A basic representation of an IPv6 address.
    //! This class is a subclass of IPAddress where all instances are bound to IPv6.
    //! @ingroup net
    //!
    class TSDUCKDLL IPv6Address: public IPAddress
    {
        TS_RULE_OF_FIVE(IPv6Address, override);
    public:

        // All constructors directly redirect to the corresponding IPAddress constructor for IPv6.
        // They are not documented again here. All other operations are inherited from IPAddress.

        //! @cond nodoxygen
        IPv6Address() : IPAddress(IP::v6) {}
        IPv6Address(const IPAddress& other);
        IPv6Address(const uint8_t *addr, size_t size);
        IPv6Address(const ByteBlock& bb) : IPv6Address(bb.data(), bb.size()) {}
        IPv6Address(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8) : IPAddress(h1, h2, h3, h4, h5, h6, h7, h8, true) {}
        IPv6Address(uint64_t net, uint64_t ifid) : IPAddress(net, ifid, true) {}
        IPv6Address(const ::sockaddr& a);
        IPv6Address(const ::sockaddr_storage& a);
        IPv6Address(const ::in6_addr& a) : IPAddress(a, true) {}
        IPv6Address(const ::sockaddr_in6& a) : IPAddress(a, true) {}
        IPv6Address(const UString& name, Report& report);
        //! @endcond
    };
}
