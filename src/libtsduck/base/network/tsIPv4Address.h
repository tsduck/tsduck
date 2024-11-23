//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IP v4 address class
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPAddress.h"

namespace ts {

    class IPv4Address;
    using IPv4AddressVector = std::vector<IPv4Address>;  //!< Vector of IPv4 addresses.
    using IPv4AddressSet = std::set<IPv4Address>;        //!< Set of IPv4 addresses.

    //!
    //! A basic representation of an IP v4 address.
    //! This class is a subclass of IPAddress where all instances are bound to IPv4.
    //! @ingroup net
    //!
    class TSDUCKDLL IPv4Address: public IPAddress
    {
        TS_RULE_OF_FIVE(IPv4Address, override);
    public:

        // All constructors directly redirect to the corresponding IPAddress constructor for IPv4.
        // They are not documented again here. All other operations are inherited from IPAddress.

        //! @cond nodoxygen
        IPv4Address() : IPAddress(IP::v4) {}
        IPv4Address(const IPAddress& other);
        IPv4Address(const uint8_t *addr, size_t size);
        IPv4Address(const ByteBlock& bb) : IPv4Address(bb.data(), bb.size()) {}
        IPv4Address(uint32_t addr) : IPAddress(addr, true) {}
        IPv4Address(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) : IPAddress(b1, b2, b3, b4, true) {}
        IPv4Address(const ::sockaddr& a);
        IPv4Address(const ::sockaddr_storage& a);
        IPv4Address(const ::in_addr& a) : IPAddress(a, true) {}
        IPv4Address(const ::sockaddr_in& a) : IPAddress(a, true) {}
        IPv4Address(const UString& name, Report& report);
        //! @endcond
    };
}
