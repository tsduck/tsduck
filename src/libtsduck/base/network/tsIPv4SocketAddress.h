//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Socket address class (IP v4 address & port).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPv4Address.h"

namespace ts {
    //!
    //! Socket address class (IP v4 address & port).
    //! @ingroup net
    //!
    //! Note: all address and port are used in host byte order.
    //!
    //! The string representation is "addr[:port]" or "[addr:]port".
    //!
    class TSDUCKDLL IPv4SocketAddress: public IPv4Address
    {
        TS_RULE_OF_FIVE(IPv4SocketAddress, override);
    private:
        uint16_t _port = 0;  // Port in host byte order
    public:
        //!
        //! Wildcard socket address, unspecified address and port.
        //!
        static const IPv4SocketAddress AnySocketAddress;

        //!
        //! Default constructor
        //!
        IPv4SocketAddress() = default;

        //!
        //! Constructor from an IP address and optional port
        //! @param [in] addr The IP v4 address.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        IPv4SocketAddress(const IPv4Address& addr, uint16_t port = AnyPort) :
            IPv4Address(addr),
            _port(port)
        {
        }

        //!
        //! Constructor from an integer address and optional port.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        IPv4SocketAddress(uint32_t addr, uint16_t port = AnyPort) :
            IPv4Address(addr),
            _port(port)
        {
        }

        //!
        //! Constructor from 4 bytes (classical IPv4 notation) and optional port.
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        IPv4SocketAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint16_t port = AnyPort) :
            IPv4Address(b1, b2, b3, b4),
            _port(port)
        {
        }

        //!
        //! Constructor from a system "struct in_addr" structure (socket API).
        //! @param [in] a A system "struct in_addr" structure.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        IPv4SocketAddress(const ::in_addr& a, uint16_t port = AnyPort) :
            IPv4Address(a),
            _port(port)
        {
        }

        //!
        //! Constructor from a system "struct sockaddr" structure (socket API).
        //! @param [in] s A system "struct sockaddr" structure.
        //!
        IPv4SocketAddress(const ::sockaddr& s);

        //!
        //! Constructor from a system "struct sockaddr_in" structure (socket API).
        //! @param [in] s A system "struct sockaddr_in" structure.
        //!
        IPv4SocketAddress(const ::sockaddr_in& s);

        //!
        //! Constructor from a string "addr[:port]" or "[addr:]port".
        //! @param [in] name A string containing either a host name or a numerical
        //! representation of the address and optional port.
        //! In case of error, the integer value of the address is
        //! set to @link AnyAddress @endlink and port to @link AnyPort @endlink.
        //! @param [in] report Where to report errors.
        //!
        IPv4SocketAddress(const UString& name, Report& report) { IPv4SocketAddress::resolve(name, report); }

        // Inherited methods.
        virtual Port port() const override;
        virtual void setPort(Port port) override;
        virtual bool resolve(const UString& name, Report& report) override;
        virtual UString toString() const override;

        //!
        //! Set an integer address and port.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        void set(uint32_t addr, uint16_t port)
        {
            setAddress(addr);
            _port = port;
        }

        //!
        //! Set address from 4 bytes (classical IPv4 notation) and port.
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        void set(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint16_t port)
        {
            setAddress(b1, b2, b3, b4);
            _port = port;
        }

        //!
        //! Copy into a system "struct sockaddr" structure (socket API).
        //! @param [out] s A system "struct sockaddr" structure.
        //!
        void copy(::sockaddr& s) const
        {
            IPv4Address::copy(s, _port);
        }

        //!
        //! Copy into a system "struct sockaddr_in" structure (socket API).
        //! @param [out] s A system "struct sockaddr_in" structure.
        //!
        void copy(::sockaddr_in& s) const
        {
            IPv4Address::copy(s, _port);
        }

        //!
        //! Copy the address into a system "struct in_addr" structure (socket API).
        //! @param [out] a A system "struct in_addr" structure.
        //!
        void copy(::in_addr& a) const
        {
            IPv4Address::copy(a);
        }

        //!
        //! Check if this socket address "matches" another one.
        //! @param [in] other Another instance to compare.
        //! @return False if this and @a other addresses are both specified and
        //! are different or if the two ports are specified and different. True otherwise.
        //!
        bool match(const IPv4SocketAddress& other) const;

        //!
        //! Equality operator.
        //! @param [in] other Another instance to compare with.
        //! @return True if both object contains the same address, false otherwise.
        //!
        bool operator==(const IPv4SocketAddress& other) const { return address() == other.address() && _port == other._port; }
        TS_UNEQUAL_OPERATOR(IPv4SocketAddress)

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const IPv4SocketAddress& other) const;
    };

    //!
    //! Vector of socket addresses.
    //!
    typedef std::vector<IPv4SocketAddress> IPv4SocketAddressVector;

    //!
    //! Set of socket addresses.
    //!
    typedef std::set<IPv4SocketAddress> IPv4SocketAddressSet;

    //! @cond nodoxygen
    // Legacy definitions.
    typedef IPv4SocketAddress IPSocketAddress;
    typedef IPv4SocketAddressVector IPSocketAddressVector;
    typedef IPv4SocketAddressSet IPSocketAddressSet;
    //! @endcond
}
