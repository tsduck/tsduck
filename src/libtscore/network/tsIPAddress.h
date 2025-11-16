//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generic IP address class, IPv4 or IPv6.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractNetworkAddress.h"
#include "tsByteBlock.h"
#include "tsIP.h"

namespace ts {

    class IPAddress;
    using IPAddressVector = std::vector<IPAddress>;  //!< Vector of IP addresses.
    using IPAddressSet = std::set<IPAddress>;        //!< Set of IP addresses.

    //!
    //! A generic representation of an IP address, IPv4 or IPv6.
    //! @ingroup libtscore net
    //! @see https://en.wikipedia.org/wiki/IPv6_address
    //!
    //! An instance of this class can hold an IPv4 or an IPv6 address.
    //! An instance always have a generation IPv4 or IPv6. The default initial value
    //! is the IPv4 generic address for "any address".
    //!
    //! In this class, methods which apply to only one generation, IPv4 or IPv6, have
    //! a name ending in 4 or 6, respectively. Some constructors implicitly build one
    //! generation of address, IPv4 or IPv6, but their names remain IPAddress() without
    //! trailing 4 or 6 by design of C++.
    //!
    //! IP v4 addresses are sometimes manipulated as 32-bit integer values. There
    //! is always some ambiguity in the operating system interface about the byte
    //! order of these integer values. In this class, all publicly available integer
    //! values are in the natural host byte order. Whenever a conversion is required,
    //! the internal guts of this class will do it for you (and hide it from you).
    //!
    //! An IPv6 address is made of 128 bits (16 bytes). It can be manipulated as
    //! - 16 bytes
    //! - 8 groups of 16 bits or hextets.
    //! - 2 64-bit values, the network prefix and the network identifier.
    //!
    class TSCOREDLL IPAddress: public AbstractNetworkAddress
    {
        TS_RULE_OF_FIVE(IPAddress, override);
    private:
        IP       _gen = IP::v4;     // Current generation of the IP address. Never IP::Any.
        uint32_t _addr4 = 0;        // An IPv4 address is a 32-bit word in host byte order
        uint8_t  _bytes6[16] {};    // Raw content of the IPv6 address.

    public:
        static constexpr size_t BITS4  =  32;  //!< Size in bits of an IPv4 address.
        static constexpr size_t BYTES4 =   4;  //!< Size in bytes of an IPv4 address.
        static constexpr size_t BITS6  = 128;  //!< Size in bits of an IPv6 address.
        static constexpr size_t BYTES6 =  16;  //!< Size in bytes of an IPv6 address.

        static const IPAddress AnyAddress4;     //!< Wildcard value for "any IPv4 address".
        static const IPAddress AnyAddress6;     //!< Wildcard value for "any IPv6 address".
        static const IPAddress LocalHost4;      //!< Local host IPv4 address ("localhost").
        static const IPAddress LocalHost6;      //!< Local host IPv6 address (::1, "localhost").

        //!
        //! Get the "any address" template for a given generation of IP protocols.
        //! @param [in] gen Generation of IP protocols.
        //! @return A constant reference to the corresponding "any address".
        //!
        static const IPAddress& AnyAddress(IP gen) { return gen == IP::v6 ? AnyAddress6 : AnyAddress4; }

        //!
        //! Get the "local host" address for a given generation of IP protocols.
        //! @param [in] gen Generation of IP protocols.
        //! @return A constant reference to the corresponding "local host" address.
        //!
        static const IPAddress& LocalHost(IP gen) { return gen == IP::v6 ? LocalHost6 : LocalHost4; }

        //!
        //! Get the address size in bits for a given generation of IP protocols.
        //! @param [in] gen Generation of IP protocols.
        //! @return Address size in bits.
        //!
        static size_t AddressBits(IP gen) { return gen == IP::v6 ? BITS6 : BITS4; }

        //!
        //! Default constructor with no initial value.
        //! The default initial value is AnyAddress4.
        //!
        IPAddress() = default;

        //!
        //! Generic constructor from an address in binary format.
        //! @param [in] addr Address of the memory area containing the address in binary format.
        //! @param [in] size Size of the memory area. If the size is 4, this is an IPv4 address.
        //! If the size is 16, this is an IPv6 address. For all other sizes, the address is AnyAddress4.
        //!
        IPAddress(const void* addr, size_t size) { IPAddress::setAddress(addr, size); }

        //!
        //! Generic constructor from an address in binary format.
        //! @param [in] bb Byte block containing the address in binary format. If the size is 4, this is an IPv4 address.
        //! If the size is 16, this is an IPv6 address. For all other sizes, the address is AnyAddress4.
        //!
        IPAddress(const ByteBlock& bb) { IPAddress::setAddress(bb.data(), bb.size()); }

        //!
        //! IPv4 constructor from an integer IPv4 address.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //!
        IPAddress(uint32_t addr) : _addr4(addr) {}

        //!
        //! IPv4 constructor from 4 bytes (classical IPv4 notation).
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //!
        IPAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

        //!
        //! IPv6 constructor of an IPv6 address from 8 hexlets.
        //! @param [in] h1 First address hexlet.
        //! @param [in] h2 2nd address hexlet.
        //! @param [in] h3 3rd address hexlet.
        //! @param [in] h4 4th address hexlet.
        //! @param [in] h5 5th address hexlet.
        //! @param [in] h6 6th address hexlet.
        //! @param [in] h7 7th address hexlet.
        //! @param [in] h8 8th address hexlet.
        //!
        IPAddress(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8);

        //!
        //! IPv6 constructor of an IPv6 address from network prefix and interface identifier.
        //! @param [in] net Network prefix.
        //! @param [in] ifid Interface identifier.
        //!
        IPAddress(uint64_t net, uint64_t ifid);

        //!
        //! Generic constructor from a system "struct sockaddr" structure (IPv4 or IPv6).
        //! @param [in] a A system "struct sockaddr" structure.
        //! Note: the structure "sockaddr" is deprecated because it cannot hold an IPv6 socket address.
        //! The structure "sockaddr_storage" should be used instead.
        //!
        IPAddress(const ::sockaddr& a);

        //!
        //! Generic constructor from a system "struct sockaddr_storage" structure (IPv4 or IPv6).
        //! @param [in] a A system "struct sockaddr_storage" structure.
        //!
        IPAddress(const ::sockaddr_storage& a) : IPAddress(*reinterpret_cast<const ::sockaddr*>(&a)) {}

        //!
        //! IPv4 constructor from a system "struct in_addr" structure (IPv4 socket API).
        //! @param [in] a A system "struct in_addr" structure.
        //!
        IPAddress(const ::in_addr& a);

        //!
        //! IPv4 constructor from a system "struct sockaddr_in" structure (IPv4 socket API).
        //! @param [in] a A system "struct sockaddr_in" structure.
        //!
        IPAddress(const ::sockaddr_in& a) : IPAddress(*reinterpret_cast<const ::sockaddr*>(&a)) {}

        //!
        //! IPv6 constructor from a system "struct in6_addr" structure (IPv6 socket API).
        //! @param [in] a A system "struct in6_addr" structure.
        //!
        IPAddress(const ::in6_addr& a);

        //!
        //! IPv6 constructor from a system "struct sockaddr_in6" structure (IPv6 socket API).
        //! @param [in] a A system "struct sockaddr_in6" structure.
        //!
        IPAddress(const ::sockaddr_in6& a) : IPAddress(*reinterpret_cast<const ::sockaddr*>(&a)) {}

        //!
        //! Constructor from a string, host name or integer format.
        //! If @a name cannot be resolved, the address is set to AnyAddress4.
        //! @param [in] name A string containing either a host name or a numerical representation of the address.
        //! @param [in] report Where to report errors.
        //! @param [in] preferred Preferred IP generation of the returned address. Return the first availabale address by default.
        //!
        IPAddress(const UString& name, Report& report, IP preferred = IP::Any) { IPAddress::resolve(name, report, preferred); }

        //!
        //! Equality operator.
        //! @param [in] other Another instance to compare with.
        //! @return True if both object contains the same address, false otherwise.
        //!
        bool operator==(const IPAddress& other) const;

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const IPAddress& other) const;

        //!
        //! Check if this address "matches" another one.
        //! @param [in] other Another instance to compare.
        //! @return False if this and @a other addresses are both specified and are different. True otherwise.
        //!
        bool match(const IPAddress& other) const;

        //!
        //! Get the current generation of IP addresses.
        //! @return The IP generation of the address currently in this instance. Never IP::Any.
        //!
        IP generation() const { return _gen; }

        //!
        //! Check if the address is an IPv6 address which is mapped to an IPv4 one.
        //! @return true if the address is an IPv6 address which is mapped to an IPv4 one.
        //!
        bool isIPv4Mapped() const;

        //!
        //! Convert an IP address to another generation, when possible.
        //! @param [in] gen New IP generation to apply. If @a gen is IP::Any or the same as the current
        //! generation, return true. A conversion from IPv4 to IPv6 always works (IPv4-mapped address).
        //! The conversion of an IPv6 address into IPv4 is only possible if it is an IPv4-mapped address.
        //! For convenience, the IPv4 and IPv6 ampty and loopback addressses are converted to each other.
        //! @return True if the conversion was successful, false if the conversion was no possible.
        //!
        bool convert(IP gen);

        // AbstractNetworkAddress interface.
        virtual size_t binarySize() const override;
        virtual const UChar* familyName() const override;
        virtual bool hasAddress() const override;
        virtual size_t getAddress(void* addr, size_t size) const override;
        virtual bool setAddress(const void* addr, size_t size) override;
        virtual void clearAddress() override;
        virtual bool isMulticast() const override;
        virtual bool resolve(const UString& name, Report& report) override;
        virtual UString toFullString() const override;
        virtual UString toString() const override;

        //!
        //! Check if the address is a source specific multicast (SSM) address.
        //! @return True if the address is an SSM address, false otherwise.
        //!
        bool isSSM() const;

        //!
        //! Check if two IPv6 multicast addresses are identical, excluding the "scope" bits.
        //! @param [in] mc Another IPv6 multicast address.
        //! @return True if this address and @a mc are two IPv6 multicast addresses and are
        //! identical, excluding the "scope" bits in the comparison. False otherwise.
        //!
        bool sameMulticast6(const IPAddress& mc) const;

        //!
        //! Get the IPv6 multicast "scope" bits of this address.
        //! @return The IPv6 multicast "scope" bits of this address (from 0x00 ro 0x0F) or 0xFF if not an IPv6 multicast address.
        //!
        uint8_t scopeMulticast6() const;

        //!
        //! Check if the address is a link-local address, typically an auto-configured address.
        //! @return True if the address is a link-local address, false otherwise.
        //!
        bool isLinkLocal() const;

        //!
        //! Get the IPv4 address as a 32-bit integer value in host byte order.
        //! @return The IPv4 address as a 32-bit integer value in host byte order or zero if not an IPv4 address.
        //!
        uint32_t address4() const { return _gen == IP::v4 ? _addr4 : 0; }

        //!
        //! Get the IPv6 address as a byte block.
        //! @return Byte block containing the IPv6 bytes or an empty blcok if not an IPv6 address.
        //!
        ByteBlock address6() const;

        //!
        //! Get the IPv6 network prefix (64 most significant bits) of the IPv6 address.
        //! @return The network prefix (64 most significant bits) of the IPv6 address.
        //!
        uint64_t networkPrefix6() const;

        //!
        //! Get the IPv6 interface identifier (64 least significant bits) of the IPv6 address.
        //! @return The interface identifier (64 least significant bits) of the IPv6 address.
        //!
        uint64_t interfaceIdentifier6() const;

        //!
        //! Get one of the 16-bit hexlets in the IPv6 address.
        //! @param [in] i Hexlet index, from 0 to 7.
        //! @return The corresponding hexlet or zero if @a i is out of range.
        //!
        uint16_t hexlet6(size_t i) const;

        //!
        //! Set the IP address from another IPAddress object.
        //! Useful for subclasses to assign the address part only.
        //! @param [in] other Another IP address.
        //!
        void setAddress(const IPAddress& other);

        //!
        //! Set the IP address from an address in binary format.
        //! Useful for subclasses to assign the address part only.
        //! @param [in] bb Byte block containing the address in binary format. If the size is 4, this is an IPv4 address.
        //! If the size is 16, this is an IPv6 address. For all other sizes, the address is AnyAddress4.
        //! @return True on success, false on error (incorrect data size).
        //!
        bool setAddress(const ByteBlock& bb) { return setAddress(bb.data(), bb.size()); }

        //!
        //! Set the IP address from an IPv4 address as a 32-bit integer value in host byte order.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //!
        void setAddress4(uint32_t addr);

        //!
        //! Set the IP address from 4 bytes (classical IPv4 notation).
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //!
        void setAddress4(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

        //!
        //! Set the IP address from an IPv6 address as 8 hexlets.
        //! @param [in] h1 First address hexlet.
        //! @param [in] h2 2nd address hexlet.
        //! @param [in] h3 3rd address hexlet.
        //! @param [in] h4 4th address hexlet.
        //! @param [in] h5 5th address hexlet.
        //! @param [in] h6 6th address hexlet.
        //! @param [in] h7 7th address hexlet.
        //! @param [in] h8 8th address hexlet.
        //!
        void setAddress6(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8);

        //!
        //! Set the IP address from an IPv6 address as network prefix and interface identifier.
        //! @param [in] net Network prefix.
        //! @param [in] ifid Interface identifier.
        //!
        void setAddress6(uint64_t net, uint64_t ifid);

        //!
        //! Set the IP address from a system "struct sockaddr" structure (IPv4 or IPv6).
        //! @param [in] a A system "struct sockaddr" structure.
        //! Note: the structure "sockaddr" is deprecated because it cannot hold an IPv6 socket address.
        //! The structure "sockaddr_storage" should be used instead.
        //! @return True on success, false on error (incorrect family type).
        //!
        bool setAddress(const ::sockaddr& a);

        //!
        //! Set the IP address from a system "struct sockaddr_storage" structure (IPv4 or IPv6).
        //! @param [in] a A system "struct sockaddr_storage" structure.
        //! @return True on success, false on error (incorrect family type).
        //!
        bool setAddress(const ::sockaddr_storage& a) { return setAddress(*reinterpret_cast<const ::sockaddr*>(&a)); }

        //!
        //! Set the IPv4 address from a system "struct sockaddr_in" structure.
        //! @param [in] a A system "struct sockaddr_in" structure.
        //! @return True on success, false on error (incorrect family type).
        //!
        bool setAddress(const ::sockaddr_in& a) { return setAddress(*reinterpret_cast<const ::sockaddr*>(&a)); }

        //!
        //! Set the IPv6 address from a system "struct sockaddr_in6" structure.
        //! @param [in] a A system "struct sockaddr_in6" structure.
        //! @return True on success, false on error (incorrect family type).
        //!
        bool setAddress(const ::sockaddr_in6& a) { return setAddress(*reinterpret_cast<const ::sockaddr*>(&a)); }

        //!
        //! Set the IPv4 address from a system "struct in_addr" structure.
        //! @param [in] a A system "struct in_addr" structure.
        //!
        void setAddress4(const ::in_addr& a);

        //!
        //! Set the IPv6 address from a system "struct in6_addr" structure.
        //! @param [in] a A system "struct in6_addr" structure.
        //!
        void setAddress6(const ::in6_addr& a);

        //!
        //! Copy the address into a system "struct sockaddr_storage" structure (socket API).
        //! Note: the structure "sockaddr" is deprecated because it cannot hold an IPv6 socket address.
        //! The structure "sockaddr_storage" should be used instead.
        //! @param [out] a A system "struct sockaddr_storage" structure. This type of structure is large
        //! enough to hold a structure sockaddr_in or sockaddr_in6.
        //! @param [in] port Port number for the socket address.
        //! @return Actual number of bytes used in the structure "sockaddr_storage".
        //!
        size_t getAddress(::sockaddr_storage& a, Port port) const;

        //!
        //! Copy the IPv4 address into a system "struct sockaddr_in" structure (socket API).
        //! @param [out] a A system "struct sockaddr_in" structure.
        //! @param [in] port Port number for the socket address.
        //! @return True on success, false on error (incorrect family type).
        //!
        bool getAddress4(::sockaddr_in& a, Port port) const;

        //!
        //! Copy the IPv4 address into a system "struct in_addr" structure (socket API).
        //! @param [out] a A system "struct in_addr" structure.
        //! @return True on success, false on error (incorrect family type).
        //!
        bool getAddress4(::in_addr& a) const;

        //!
        //! Copy the IPv6 address into a system "struct sockaddr_in6" structure (socket API).
        //! @param [out] a A system "struct sockaddr_in6" structure.
        //! @param [in] port Port number for the socket address.
        //! @return True on success, false on error (incorrect family type).
        //!
        bool getAddress6(::sockaddr_in6& a, Port port) const;

        //!
        //! Copy the IPv6 address into a system "struct in6_addr" structure (socket API).
        //! @param [out] a A system "struct in6_addr" structure.
        //! @return True on success, false on error (incorrect family type).
        //!
        bool getAddress6(::in6_addr& a) const;

        //!
        //! Decode a string containing a network address in family-specific format.
        //! @param [in] name A string containing either a host name or a numerical representation of the address.
        //! @param [in] report Where to report errors.
        //! @param [in] preferred Preferred IP generation of the returned address. Return the first availabale address by default.
        //! If no address of that generation is available, return one from the other generation if available.
        //! @return True if @a name was successfully resolved, false otherwise.
        //! In the later case, the address is invalidated.
        //!
        virtual bool resolve(const UString& name, Report& report, IP preferred);

        //!
        //! Decode a host name and get all possible addresses for that host.
        //! @param [out] addresses List of possible addresses for @a name.
        //! If @a name is a valid numerical addresse, only this one is returned.
        //! @param [in] name A string containing either a host name or a numerical representation of the address.
        //! @param [in,out] report Where to report errors.
        //! @param [in] gen IP generation of the returned address. Can be used to restrict the result to
        //! IPv4 or IPv6 addresses. Return all by default.
        //! @return True if @a name was successfully resolved, false otherwise.
        //!
        static bool ResolveAllAddresses(IPAddressVector& addresses, const UString& name, Report& report, IP gen = IP::Any);

    private:
        // Try to decode a literal IPv4 or IPv6 address in numerical form.
        bool decode4(const UString&);
        bool decode6(const UString&);

        // Call ::getaddrinfo(). On success, the result is not null and must be freed using ::freeaddrinfo().
        static ::addrinfo* GetAddressInfo(IP gen, const UString& name, Report& report);
    };
}
