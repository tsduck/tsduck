//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
#include "tsException.h"
#include "tsIP.h"

namespace ts {

    class IPAddress;
    using IPAddressVector = std::vector<IPAddress>;  //!< Vector of IP addresses.
    using IPAddressSet = std::set<IPAddress>;        //!< Set of IP addresses.

    //!
    //! A generic representation of an IP address, IPv4 or IPv6.
    //! @ingroup net
    //! @see https://en.wikipedia.org/wiki/IPv6_address
    //!
    //! An instance of this class can hold an IPv4 or an IPv6 address.
    //!
    //! An instance can optionally be bound by construction to a given generation.
    //! If it is bound to a given generation, trying to assign an address of a
    //! different generation will throw an exception. Binding to an IP generation
    //! is a property of the object, the variable, not the value.
    //!
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
    class TSDUCKDLL IPAddress: public AbstractNetworkAddress
    {
    private:
        const IP _bound = IP::Any;  // Fixed (bound) generation of the IP address.
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
        //! This exception is thrown when assigning incompatible IP addresses.
        //!
        TS_DECLARE_EXCEPTION(IncompatibleIPAddress);

        //!
        //! Default constructor with no initial value.
        //! The default initial value is AnyAddress4.
        //!
        IPAddress() = default;

        //!
        //! Constructor with no initial value but optionally bound to a generation.
        //! The default initial value is AnyAddress4, unless @a bound is IP::v6 in which
        //! case the default initial value is AnyAddress6.
        //! @param [in] bound Bound generation of the IP address.
        //! When set to IP::Any, this instance can receive any generation of IP address.
        //! Otherwise, this instance can only receive addresses of the specified generation.
        //! In that case, trying to assign an address from a different generation thows the
        //! exception IncompatibleIPAddress.
        //!
        IPAddress(IP bound);

        //!
        //! Copy constructor.
        //! @param [in] other Another instance to copy.
        //! If @a other is bound to an IP generation, this object is not bound to a generation
        //! (binding is a property of an object, not of a value).
        //!
        IPAddress(const IPAddress& other);

        //!
        //! Copy constructor with optional binding.
        //! @param [in] other Another instance to copy.
        //! @param [in] bound If true, this instance is bound to its generation. Otherwise, it can receive any address.
        //!
        IPAddress(const IPAddress& other, bool bound);

        //!
        //! Destructor.
        //!
        virtual ~IPAddress() override;

        //!
        //! Generic constructor from an address in binary format.
        //! @param [in] addr Address of the memory area containing the address in binary format.
        //! @param [in] size Size of the memory area. If the size is 4, this is an IPv4 address.
        //! If the size is 16, this is an IPv6 address. For all other sizes, the address is AnyAddress4.
        //! @param [in] bound If true, this instance is bound to its generation. Otherwise, it can receive any address.
        //!
        IPAddress(const uint8_t *addr, size_t size, bool bound = false);

        //!
        //! Generic constructor from an address in binary format.
        //! @param [in] bb Byte block containing the address in binary format. If the size is 4, this is an IPv4 address.
        //! If the size is 16, this is an IPv6 address. For all other sizes, the address is AnyAddress4.
        //! @param [in] bound If true, this instance is bound to its generation. Otherwise, it can receive any address.
        //!
        IPAddress(const ByteBlock& bb, bool bound = false) : IPAddress(bb.data(), bb.size(), bound) {}

        //!
        //! IPv4 constructor from an integer IPv4 address.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //! @param [in] bound If true, this instance is bound to IPv4. Otherwise, it can receive any address.
        //!
        IPAddress(uint32_t addr, bool bound = false);

        //!
        //! IPv4 constructor from 4 bytes (classical IPv4 notation).
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //! @param [in] bound If true, this instance is bound to IPv4. Otherwise, it can receive any address.
        //!
        IPAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, bool bound = false);

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
        //! @param [in] bound If true, this instance is bound to IPv6. Otherwise, it can receive any address.
        //!
        IPAddress(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8, bool bound = false);

        //!
        //! IPv6 constructor of an IPv6 address from network prefix and interface identifier.
        //! @param [in] net Network prefix.
        //! @param [in] ifid Interface identifier.
        //! @param [in] bound If true, this instance is bound to IPv6. Otherwise, it can receive any address.
        //!
        IPAddress(uint64_t net, uint64_t ifid, bool bound = false);

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
        //! @param [in] bound If true, this instance is bound to IPv4. Otherwise, it can receive any address.
        //!
        IPAddress(const ::in_addr& a, bool bound = false);

        //!
        //! IPv4 constructor from a system "struct sockaddr_in" structure (IPv4 socket API).
        //! @param [in] a A system "struct sockaddr_in" structure.
        //! @param [in] bound If true, this instance is bound to IPv4. Otherwise, it can receive any address.
        //!
        IPAddress(const ::sockaddr_in& a, bool bound = false);

        //!
        //! IPv6 constructor from a system "struct in6_addr" structure (IPv6 socket API).
        //! @param [in] a A system "struct in6_addr" structure.
        //! @param [in] bound If true, this instance is bound to IPv6. Otherwise, it can receive any address.
        //!
        IPAddress(const ::in6_addr& a, bool bound = false);

        //!
        //! IPv6 constructor from a system "struct sockaddr_in6" structure (IPv6 socket API).
        //! @param [in] a A system "struct sockaddr_in6" structure.
        //! @param [in] bound If true, this instance is bound to IPv6. Otherwise, it can receive any address.
        //!
        IPAddress(const ::sockaddr_in6& a, bool bound = false);

        //!
        //! Constructor from a string, host name or integer format.
        //! If @a name cannot be resolved, the address is set to AnyAddress4.
        //! @param [in] name A string containing either a host name or a numerical representation of the address.
        //! @param [in] report Where to report errors.
        //! @param [in] bound Bound generation of the IP address. If not set to IP::Any (the default),
        //! this instance becomes bound to that IP generation and the name resolution can only produce
        //! an address of that generation.
        //!
        IPAddress(const UString& name, Report& report, IP bound = IP::Any) : IPAddress(bound) { IPAddress::resolve(name, report); }

        //!
        //! Assignment operator.
        //! The binding of this object to an IP generation is not changed.
        //! @param [in] other Another instance to copy.
        //! @return Reference to this object.
        //!
        IPAddress& operator=(const IPAddress& other);

        //!
        //! Equality operator.
        //! @param [in] other Another instance to compare with.
        //! @return True if both object contains the same address, false otherwise.
        //!
        bool operator==(const IPAddress& other) const;
        TS_UNEQUAL_OPERATOR(IPAddress)

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
        //! @return False if this and @a other addresses are both specified and
        //! are different. True otherwise.
        //!
        bool match(const IPAddress& other) const;

        //!
        //! Get the current generation of IP addresses.
        //! @return The IP generation of the address currently in this instance. Never IP::Any.
        //!
        IP generation() const { return _gen; }

        //!
        //! Check if the address is bound to a specific generation of IP addresses.
        //! @return True if the address is bound to a specific generation of IP addresses.
        //!
        bool isBound() const { return _bound != IP::Any; }

        //!
        //! Get the generation of IP addresses this instance is bound to.
        //! @return The generation of IP addresses this instance is bound to.
        //! Return IP::Any if this instance is not bound to a generation.
        //!
        IP boundGeneration() const { return _bound; }

        //!
        //! Check if this object can hold a value of a specific IP generation.
        //! @param [in] gen The IP generation to check.
        //! @return True if this object is not bound to another IP generation.
        //!
        bool isCompatible(IP gen) { return _bound == IP::Any || _bound == gen; }

        //!
        //! Check if this object can hold a value of a specific IP generation.
        //! @param [in] other Another IP to check.
        //! @return True if this object is not bound to an IP generation other than the current generation of @a other.
        //!
        bool isCompatible(const IPAddress& other) { return isCompatible(other._gen); }

        //!
        //! Check if this object can hold a value of a specific IP generation and throw an exception if not.
        //! @param [in] gen The IP generation to check.
        //! @throw IncompatibleIPAddress when this object is bound to another IP generation.
        //!
        void checkCompatibility(IP gen);

        //!
        //! Check if this object can hold a value of a specific IP generation and throw an exception if not.
        //! @param [in] other Another IP to check.
        //! @throw IncompatibleIPAddress when this object is bound to an IP generation other than the current generation of @a other.
        //!
        void checkCompatibility(const IPAddress& other) { checkCompatibility(other._gen); }

        //!
        //! Check if the address is an IPv6 address which is mapped to an IPv4 one.
        //! @return true if the address is an IPv6 address which is mapped to an IPv4 one.
        //!
        bool isIPv4Mapped() const;

        //!
        //! Convert an IP address to another generation, when possible.
        //! @param [in] gen New IP generation to apply. If @a gen is incompatible with the object (bound to another generation),
        //! return false. If @a gen is IP::Any or the same as the current generation, return true. A conversion from IPv4 to IPv6
        //! always works (IPv4-mapped address). The conversion of an IPv6 address is only possible if it is an IPv4-mapped address.
        //! For convenience, the IPv4 and IPv6 loopback addressses are converted to each other.
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
        //! Get the IPv4 address as a 32-bit integer value in host byte order.
        //! @return The IPv4 address as a 32-bit integer value in host byte order or zero if not an IPv4 address.
        //!
        uint32_t address4() const { return _gen == IP::v4 ? _addr4 : 0; }

        //!
        //! Get the IPv6 address as a byte block.
        //! @return Byte block containing the IPv6 bytes.
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
        //! @param [in] addr Address of the memory area containing the address in binary format.
        //! @param [in] size Size of the memory area. If the size is 4, this is an IPv4 address.
        //! If the size is 16, this is an IPv6 address. For all other sizes, the address is AnyAddress4.
        //!
        void setAddress(const uint8_t *addr, size_t size);

        //!
        //! Set the IP address from an address in binary format.
        //! Useful for subclasses to assign the address part only.
        //! @param [in] bb Byte block containing the address in binary format. If the size is 4, this is an IPv4 address.
        //! If the size is 16, this is an IPv6 address. For all other sizes, the address is AnyAddress4.
        //!
        void setAddress(const ByteBlock& bb) { setAddress(bb.data(), bb.size()); }

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
        //!
        void setAddress(const ::sockaddr& a);

        //!
        //! Set the IP address from a system "struct sockaddr_storage" structure (IPv4 or IPv6).
        //! @param [in] a A system "struct sockaddr_storage" structure.
        //!
        void setAddress(const ::sockaddr_storage& a) { setAddress(*reinterpret_cast<const ::sockaddr*>(&a)); }

        //!
        //! Set the IPv4 address from a system "struct in_addr" structure.
        //! @param [in] a A system "struct in_addr" structure.
        //!
        void setAddress4(const ::in_addr& a);

        //!
        //! Set the IPv4 address from a system "struct sockaddr_in" structure.
        //! @param [in] a A system "struct sockaddr_in" structure.
        //!
        void setAddress4(const ::sockaddr_in& a);

        //!
        //! Set the IPv6 address from a system "struct in6_addr" structure.
        //! @param [in] a A system "struct in6_addr" structure.
        //!
        void setAddress6(const ::in6_addr& a);

        //!
        //! Set the IPv6 address from a system "struct sockaddr_in6" structure.
        //! @param [in] a A system "struct sockaddr_in6" structure.
        //!
        void setAddress6(const ::sockaddr_in6& a);

        //!
        //! Copy the address into a system "struct sockaddr_storage" structure (socket API).
        //! Note: the structure "sockaddr" is deprecated because it cannot hold an IPv6 socket address.
        //! The structure "sockaddr_storage" should be used instead.
        //! @param [out] a Address of a system socket address structure.
        //! @param [in] size Size in bytes of the system socket address structure.
        //! @param [in] port Port number for the socket address.
        //! @return Actual number of bytes used in the structure "sockaddr_storage",
        //! zero on error (system socket address structure is too small).
        //!
        size_t getAddress(::sockaddr_storage* a, size_t size, Port port) const;

        //!
        //! Copy the IPv4 address into a system "struct sockaddr_in" structure (socket API).
        //! @param [out] a A system "struct sockaddr_in" structure.
        //! @param [in] port Port number for the socket address.
        //!
        void getAddress4(::sockaddr_in& a, Port port) const;

        //!
        //! Copy the IPv4 address into a system "struct in_addr" structure (socket API).
        //! @param [out] a A system "struct in_addr" structure.
        //!
        void getAddress4(::in_addr& a) const;

        //!
        //! Copy the IPv6 address into a system "struct sockaddr_in6" structure (socket API).
        //! @param [out] a A system "struct sockaddr_in6" structure.
        //! @param [in] port Port number for the socket address.
        //!
        void getAddress6(::sockaddr_in6& a, Port port) const;

        //!
        //! Copy the IPv6 address into a system "struct in6_addr" structure (socket API).
        //! @param [out] a A system "struct in6_addr" structure.
        //!
        void getAddress6(::in6_addr& a) const;

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
