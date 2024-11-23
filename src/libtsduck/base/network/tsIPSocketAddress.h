//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generic socket address class (IPv4 or IPv6 address & port).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPAddress.h"

namespace ts {

    class IPSocketAddress;
    using IPSocketAddressVector = std::vector<IPSocketAddress>;  //!< Vector of socket addresses.
    using IPSocketAddressSet = std::set<IPSocketAddress>;        //!< Set of socket addresses.

    //!
    //! Generic socket address class (IPv4 or IPv6 address & port).
    //! @ingroup net
    //!
    //! The string representation is "addr[:port]" or "[addr:]port".
    //!
    //! In this class, methods which apply to only one generation, IPv4 or IPv6, have
    //! a name ending in 4 or 6, respectively. Some constructors implicitly build one
    //! generation of address, IPv4 or IPv6, but their names remain IPAddress() without
    //! trailing 4 or 6 by design of C++.
    //!
    class TSDUCKDLL IPSocketAddress: public IPAddress
    {
        TS_RULE_OF_FIVE(IPSocketAddress, override);
    private:
        Port _port = AnyPort;  // Port in host byte order
    public:
        static const IPSocketAddress AnySocketAddress4;  //!< Wildcard socket address, unspecified IPv4 address and port.
        static const IPSocketAddress AnySocketAddress6;  //!< Wildcard socket address, unspecified IPv6 address and port.

        //!
        //! Default constructor
        //! The default initial value is AnySocketAddress4.
        //!
        IPSocketAddress() = default;

        //!
        //! Constructor with no initial value but optionally bound to a generation.
        //! The default initial value is AnySocketAddress4, unless @a bound is IP::v6 in which
        //! case the default initial value is AnySocketAddress6.
        //! @param [in] bound Bound generation of the IP address.
        //! When set to IP::Any, this instance can receive any generation of IP address.
        //! Otherwise, this instance can only receive addresses of the specified generation.
        //! In that case, trying to assign an address from a different generation thows the
        //! exception IncompatibleIPAddress.
        //!
        IPSocketAddress(IP bound) :
            IPAddress(bound)
        {}

        //!
        //! Generic constructor from an address and port.
        //! @param [in] addr Address.
        //! @param [in] port Port number as an integer in host byte order.
        //!
        IPSocketAddress(const IPAddress addr, Port port) :
            IPAddress(addr),
            _port(port)
        {}

        //!
        //! Generic constructor from an address in binary format.
        //! @param [in] addr Address of the memory area containing the address in binary format.
        //! @param [in] size Size of the memory area. If the size is 4, this is an IPv4 address.
        //! If the size is 16, this is an IPv6 address. For all other sizes, the address is AnyAddress4.
        //! @param [in] port Optional port number as an integer in host byte order.
        //! @param [in] bound If true, this instance is bound to it generation. Otherwise, it can receive any address.
        //!
        IPSocketAddress(const uint8_t *addr, size_t size, Port port, bool bound = false) :
            IPAddress(addr, size, bound),
            _port(port)
        {}

        //!
        //! Generic constructor from an address in binary format.
        //! @param [in] bb Byte block containing the address in binary format. If the size is 4, this is an IPv4 address.
        //! If the size is 16, this is an IPv6 address. For all other sizes, the address is AnyAddress4.
        //! @param [in] port Optional port number as an integer in host byte order.
        //! @param [in] bound If true, this instance is bound to it generation. Otherwise, it can receive any address.
        //!
        IPSocketAddress(const ByteBlock& bb, Port port, bool bound = false) :
            IPAddress(bb.data(), bb.size(), bound),
            _port(port)
        {}

        //!
        //! IPv4 constructor from an integer IPv4 address.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //! @param [in] port Optional port number as an integer in host byte order.
        //! @param [in] bound If true, this instance is bound to IPv4. Otherwise, it can receive any address.
        //!
        IPSocketAddress(uint32_t addr, Port port, bool bound = false) :
            IPAddress(addr, bound),
            _port(port)
        {}

        //!
        //! IPv4 constructor from 4 bytes (classical IPv4 notation).
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //! @param [in] port Optional port number as an integer in host byte order.
        //! @param [in] bound If true, this instance is bound to IPv4. Otherwise, it can receive any address.
        //!
        IPSocketAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, Port port, bool bound = false) :
            IPAddress(b1, b2, b3, b4, bound),
            _port(port)
        {}

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
        //! @param [in] port Optional port number as an integer in host byte order.
        //! @param [in] bound If true, this instance is bound to IPv6. Otherwise, it can receive any address.
        //!
        IPSocketAddress(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8, Port port, bool bound = false) :
            IPAddress(h1, h2, h3, h4, h5, h6, h7, h8, bound),
            _port(port)
        {}

        //!
        //! IPv6 constructor of an IPv6 address from network prefix and interface identifier.
        //! @param [in] net Network prefix.
        //! @param [in] ifid Interface identifier.
        //! @param [in] port Optional port number as an integer in host byte order.
        //! @param [in] bound If true, this instance is bound to IPv6. Otherwise, it can receive any address.
        //!
        IPSocketAddress(uint64_t net, uint64_t ifid, Port port, bool bound = false) :
            IPAddress(net, ifid, bound),
            _port(port)
        {}

        //!
        //! Generic constructor from a system "struct sockaddr" structure (IPv4 or IPv6).
        //! @param [in] a A system "struct sockaddr" structure.
        //! Note: the structure "sockaddr" is deprecated because it cannot hold an IPv6 socket address.
        //! The structure "sockaddr_storage" should be used instead.
        //!
        IPSocketAddress(const ::sockaddr& a);

        //!
        //! Generic constructor from a system "struct sockaddr_storage" structure (IPv4 or IPv6).
        //! @param [in] a A system "struct sockaddr_storage" structure.
        //!
        IPSocketAddress(const ::sockaddr_storage& a) : IPSocketAddress(*reinterpret_cast<const ::sockaddr*>(&a)) {}

        //!
        //! IPv4 constructor from a system "struct in_addr" structure (IPv4 socket API).
        //! @param [in] a A system "struct in_addr" structure.
        //! @param [in] port Optional port number as an integer in host byte order.
        //! @param [in] bound If true, this instance is bound to IPv4. Otherwise, it can receive any address.
        //!
        IPSocketAddress(const ::in_addr& a, Port port, bool bound = false) :
            IPAddress(a, bound),
            _port(port)
        {}

        //!
        //! IPv4 constructor from a system "struct sockaddr_in" structure (IPv4 socket API).
        //! @param [in] a A system "struct sockaddr_in" structure.
        //! @param [in] bound If true, this instance is bound to IPv4. Otherwise, it can receive any address.
        //!
        IPSocketAddress(const ::sockaddr_in& a, bool bound = false);

        //!
        //! IPv6 constructor from a system "struct in6_addr" structure (IPv6 socket API).
        //! @param [in] a A system "struct in6_addr" structure.
        //! @param [in] port Optional port number as an integer in host byte order.
        //! @param [in] bound If true, this instance is bound to IPv6. Otherwise, it can receive any address.
        //!
        IPSocketAddress(const ::in6_addr& a, Port port, bool bound = false) :
            IPAddress(a, bound),
            _port(port)
        {}

        //!
        //! IPv6 constructor from a system "struct sockaddr_in6" structure (IPv6 socket API).
        //! @param [in] a A system "struct sockaddr_in6" structure.
        //! @param [in] bound If true, this instance is bound to IPv6. Otherwise, it can receive any address.
        //!
        IPSocketAddress(const ::sockaddr_in6& a, bool bound = false);

        //!
        //! Constructor from a string, host name or integer format.
        //! If @a name cannot be resolved, the address is set to AnyAddress4.
        //! @param [in] name A string containing either a host name or a numerical representation of the address.
        //! @param [in] report Where to report errors.
        //! @param [in] bound Bound generation of the IP address. If not set to IP::Any (the default),
        //! this instance becomes bound to that IP generation and the name resolution can only produce
        //! an address of that generation.
        //!
        IPSocketAddress(const UString& name, Report& report, IP bound = IP::Any) : ts::IPSocketAddress(bound) { IPSocketAddress::resolve(name, report); }

        // Inherited methods.
        virtual Port port() const override;
        virtual void setPort(Port port) override;
        virtual bool resolve(const UString& name, Report& report) override;
        virtual UString toFullString() const override;
        virtual UString toString() const override;

        //!
        //! Set the IP address and port from a system "struct sockaddr" structure (IPv4 or IPv6).
        //! @param [in] a A system "struct sockaddr" structure.
        //! Note: the structure "sockaddr" is deprecated because it cannot hold an IPv6 socket address.
        //! The structure "sockaddr_storage" should be used instead.
        //!
        void set(const ::sockaddr& a);

        //!
        //! Set the IP address and port from a system "struct sockaddr_storage" structure (IPv4 or IPv6).
        //! @param [in] a A system "struct sockaddr_storage" structure.
        //!
        void set(const ::sockaddr_storage& a) { set(*reinterpret_cast<const ::sockaddr*>(&a)); }

        //!
        //! Set the IPv4 address and port from a system "struct sockaddr_in" structure.
        //! @param [in] a A system "struct sockaddr_in" structure.
        //!
        void set4(const ::sockaddr_in& a)
        {
            setAddress4(a);
            _port = ntohs(a.sin_port);
        }

        //!
        //! Set the IPv6 address and port from a system "struct sockaddr_in6" structure.
        //! @param [in] a A system "struct sockaddr_in6" structure.
        //!
        void set6(const ::sockaddr_in6& a)
        {
            setAddress6(a);
            _port = ntohs(a.sin6_port);
        }

        //!
        //! Get the address and port into a system "struct sockaddr_storage" structure.
        //! Note: the structure "sockaddr" is deprecated because it cannot hold an IPv6 socket address.
        //! The structure "sockaddr_storage" should be used instead.
        //! @param [out] a Address of a system socket address structure.
        //! @param [in] size Size in bytes of the system socket address structure.
        //! @return Actual number of bytes used in the structure "sockaddr_storage",
        //! zero on error (system socket address structure is too small).
        //!
        size_t get(::sockaddr_storage* a, size_t size) const
        {
            return IPAddress::getAddress(a, size, _port);
        }

        //!
        //! Get the IPv4 address and port into a system "struct sockaddr_in" structure.
        //! @param [out] a A system "struct sockaddr_in" structure.
        //!
        void get4(::sockaddr_in& a) const
        {
            IPAddress::getAddress4(a, _port);
        }

        //!
        //! Get the IPv6 address and port into a system "struct sockaddr_in6" structure.
        //! @param [out] a A system "struct sockaddr_in6" structure.
        //!
        void get6(::sockaddr_in6& a) const
        {
            IPAddress::getAddress6(a, _port);
        }

        //!
        //! Check if this socket address "matches" another one.
        //! @param [in] other Another instance to compare.
        //! @return False if this and @a other addresses are both specified and
        //! are different or if the two ports are specified and different. True otherwise.
        //!
        bool match(const IPSocketAddress& other) const;

        //!
        //! Equality operator.
        //! @param [in] other Another instance to compare with.
        //! @return True if both object contains the same address, false otherwise.
        //!
        bool operator==(const IPSocketAddress& other) const { return IPAddress::operator==(other) && _port == other._port; }
        TS_UNEQUAL_OPERATOR(IPSocketAddress)

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const IPSocketAddress& other) const;
    };
}
