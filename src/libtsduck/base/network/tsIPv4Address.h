//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IP v4 address class
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractNetworkAddress.h"
#include "tsIP.h"

namespace ts {
    //!
    //! A basic representation of an IP v4 address.
    //! @ingroup net
    //!
    //! IP v4 addresses are sometimes manipulated as 32-bit integer values.
    //! There is always some ambiguity in the operating system interface about the
    //! byte order of these integer values. In this class, all publicly available
    //! integer values are in the natural host byte order. Whenever a conversion
    //! is required, the internal guts of this class will do it for you (and hide
    //! it from you).
    //!
    //! The string representation is "int.int.int.int".
    //!
    class TSDUCKDLL IPv4Address: public AbstractNetworkAddress
    {
        TS_RULE_OF_FIVE(IPv4Address, override);
    private:
        uint32_t _addr = 0;  // An IPv4 address is a 32-bit word in host byte order
    public:
        //!
        //! Size in bits of an IPv4 address.
        //!
        static constexpr size_t BITS = 32;

        //!
        //! Size in bytes of an IPv4 address.
        //!
        static constexpr size_t BYTES = 4;

        //!
        //! Wildcard integer value for "any IP address".
        //!
        static constexpr uint32_t AnyAddress = 0;

        //!
        //! Local host address.
        //! Usually resolves to the host name "localhost".
        //!
        static const IPv4Address LocalHost;

        //!
        //! Default constructor
        //!
        IPv4Address() = default;

        //!
        //! Constructor from an integer address.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //!
        IPv4Address(uint32_t addr) : _addr(addr) {}

        //!
        //! Constructor from 4 bytes (classical IPv4 notation).
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //!
        IPv4Address(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

        //!
        //! Constructor from a system "struct in_addr" structure (socket API).
        //! @param [in] a A system "struct in_addr" structure.
        //!
        IPv4Address(const ::in_addr& a) : _addr(ntohl(a.s_addr)) {}

        //!
        //! Constructor from a system "struct sockaddr" structure (socket API).
        //! @param [in] a A system "struct sockaddr" structure.
        //!
        IPv4Address(const ::sockaddr& a);

        //!
        //! Constructor from a system "struct sockaddr_in" structure (socket API).
        //! @param [in] a A system "struct sockaddr_in" structure.
        //!
        IPv4Address(const ::sockaddr_in& a);

        //!
        //! Constructor from a string, host name or "a.b.c.d" integer format.
        //! If @a name cannot be resolved, the integer value of the address is
        //! set to @link AnyAddress @endlink.
        //! @param [in] name A string containing either a host name or a numerical
        //! representation of the address "a.b.c.d".
        //! @param [in] report Where to report errors.
        //!
        IPv4Address(const UString& name, Report& report) { IPv4Address::resolve(name, report); }

        // Inherited methods.
        virtual size_t binarySize() const override;
        virtual bool hasAddress() const override;
        virtual size_t getAddress(void* addr, size_t size) const override;
        virtual bool setAddress(const void* addr, size_t size) override;
        virtual void clearAddress() override;
        virtual bool isMulticast() const override;
        virtual bool resolve(const UString& name, Report& report) override;
        virtual UString toString() const override;

        //!
        //! Get the IP address as a 32-bit integer value in host byte order.
        //! @return The IP address as a 32-bit integer value in host byte order.
        //!
        uint32_t address() const { return _addr; }

        //!
        //! Set the IP address from a 32-bit integer value in host byte order.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //!
        void setAddress(uint32_t addr) { _addr = addr; }

        //!
        //! Set the IP address from another IPv4Address object.
        //! Useful for subclasses.
        //! @param [in] addr Another IP address.
        //!
        void setAddress(const IPv4Address& addr) { _addr = addr._addr; }

        //!
        //! Set the IP address from 4 bytes (classical IPv4 notation).
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //!
        void setAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

        //!
        //! Check if the address is a source specific multicast (SSM) address.
        //! Note: SSM addresses are in the range 232.0.0.0/8.
        //! @return True if the address is an SSM address, false otherwise.
        //!
        bool isSSM() const { return (_addr & 0xFF000000) == 0xE8000000; }

        //!
        //! Copy the address into a system "struct sockaddr" structure (socket API).
        //! @param [out] a A system "struct sockaddr" structure.
        //! @param [in] port Port number for the socket address.
        //!
        void copy(::sockaddr& a, uint16_t port) const;

        //!
        //! Copy the address into a system "struct sockaddr_in" structure (socket API).
        //! @param [out] a A system "struct sockaddr_in" structure.
        //! @param [in] port Port number for the socket address.
        //!
        void copy(::sockaddr_in& a, uint16_t port) const;

        //!
        //! Copy the address into a system "struct in_addr" structure (socket API).
        //! @param [out] a A system "struct in_addr" structure.
        //!
        void copy(::in_addr& a) const { a.s_addr = htonl(_addr); }

        //!
        //! Check if this address "matches" another one.
        //! @param [in] other Another instance to compare.
        //! @return False if this and @a other addresses are both specified and
        //! are different. True otherwise.
        //!
        bool match(const IPv4Address& other) const;

        //!
        //! Equality operator.
        //! @param [in] a Another instance to compare with.
        //! @return True if both object contains the same address, false otherwise.
        //!
        bool operator==(const IPv4Address& a) const { return _addr == a._addr; }
        TS_UNEQUAL_OPERATOR(IPv4Address)

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const IPv4Address& other) const { return _addr < other._addr; }
    };

    //!
    //! Vector of IP addresses.
    //!
    typedef std::vector<IPv4Address> IPv4AddressVector;

    //!
    //! Set of IP addresses.
    //!
    typedef std::set<IPv4Address> IPv4AddressSet;

    //! @cond nodoxygen
    // Legacy definitions.
    typedef IPv4Address IPAddress;
    typedef IPv4AddressVector IPAddressVector;
    typedef IPv4AddressSet IPAddressSet;
    //! @endcond
}
