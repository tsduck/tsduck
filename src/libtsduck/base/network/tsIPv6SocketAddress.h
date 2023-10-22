//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IPv6 Socket address class (IP v6 address & port).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPv6Address.h"

namespace ts {
    //!
    //! IP v6 socket address class (IP v6 address & port).
    //! @ingroup net
    //!
    //! The string representation is "[addr]:port".
    //! In the template string "[addr]:port", the square brackets do not mean
    //! that "addr" is optional. According to IPv6 URL representation, the
    //! square brackets are actual characters in the string.
    //! The address and port fields are optional. If the address is specified alone,
    //! without port, the square brackets may be omitted.
    //!
    class TSDUCKDLL IPv6SocketAddress: public IPv6Address
    {
    private:
        uint16_t _port {AnyPort};  // Port in host byte order
    public:
        //!
        //! Wildcard socket address, unspecified address and port.
        //!
        static const IPv6SocketAddress AnySocketAddress;

        //!
        //! Default constructor
        //!
        IPv6SocketAddress() = default;

        //!
        //! Constructor from an IPv6 address and optional port
        //! @param [in] addr The IPv6 address.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        IPv6SocketAddress(const IPv6Address& addr, uint16_t port = AnyPort) :
            IPv6Address(addr),
            _port(port)
        {
        }

        //!
        //! Constructor from 16 bytes address and optional port.
        //! @param [in] addr Address of the memory area containing the IPv6 bytes.
        //! @param [in] size Size of the memory area. If the size is shorter than 16,
        //! the IPv6 is padded on the left (most significant bytes) with zeroes.
        //! If the size is larger than 16, extra bytes are ignored.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        IPv6SocketAddress(const uint8_t *addr, size_t size, uint16_t port = AnyPort) :
            IPv6Address(addr, size),
            _port(port)
        {
        }

        //!
        //! Constructor from 16 bytes address and optional port.
        //! @param [in] bb Byte block containing the IPv6 bytes. If the size is shorter
        //! than 16, the IPv6 is padded on the left (most significant bytes) with zeroes.
        //! If the size is larger than 16, extra bytes are ignored.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        IPv6SocketAddress(const ByteBlock& bb, uint16_t port = AnyPort) :
            IPv6Address(bb),
            _port(port)
        {
        }

        //!
        //! Constructor from 8 hexlets and optional port.
        //! @param [in] h1 First address hexlet.
        //! @param [in] h2 2nd address hexlet.
        //! @param [in] h3 3rd address hexlet.
        //! @param [in] h4 4th address hexlet.
        //! @param [in] h5 5th address hexlet.
        //! @param [in] h6 6th address hexlet.
        //! @param [in] h7 7th address hexlet.
        //! @param [in] h8 8th address hexlet.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        IPv6SocketAddress(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8, uint16_t port = AnyPort) :
            IPv6Address(h1, h2, h3, h4, h5, h6, h7, h8),
            _port(port)
        {
        }

        //!
        //! Constructor from network prefix and interface identifier.
        //! @param [in] net Network prefix.
        //! @param [in] ifid Interface identifier.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        IPv6SocketAddress(uint64_t net, uint64_t ifid, uint16_t port = AnyPort) :
            IPv6Address(net, ifid),
            _port(port)
        {
        }

        //!
        //! Constructor from a string "[addr]:port".
        //! @param [in] name A string containing either a host name or a numerical
        //! representation of the address and optional port.
        //! In case of error, the integer value of the address is
        //! set to @link AnyAddress @endlink and port to @link AnyPort @endlink.
        //! @param [in] report Where to report errors.
        //!
        IPv6SocketAddress(const UString& name, Report& report) :
            IPv6Address(),
            _port(0)
        {
            IPv6SocketAddress::resolve(name, report);
        }

        //!
        //! Virtual destructor
        //!
        virtual ~IPv6SocketAddress() override;

        // Inherited methods.
        virtual Port port() const override;
        virtual void setPort(Port port) override;
        virtual bool resolve(const UString& name, Report& report) override;
        virtual UString toString() const override;
        virtual UString toFullString() const override;

        //!
        //! Set the socket address from 16 bytes andport.
        //! @param [in] addr Address of the memory area containing the IPv6 bytes.
        //! @param [in] size Size of the memory area. If the size is shorter than 16,
        //! the IPv6 is padded on the left (most significant bytes) with zeroes.
        //! If the size is larger than 16, extra bytes are ignored.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        void set(const uint8_t *addr, size_t size, uint16_t port)
        {
            setAddress(addr, size);
            _port = port;
        }

        //!
        //! Set the socket address from 16 bytes and port.
        //! @param [in] bb Byte block containing the IPv6 bytes. If the size is shorter
        //! than 16, the IPv6 is padded on the left (most significant bytes) with zeroes.
        //! If the size is larger than 16, extra bytes are ignored.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        void set(const ByteBlock& bb, uint16_t port)
        {
            setAddress(bb);
            _port = port;
        }

        //!
        //! Set the socket address from 8 hexlets and port.
        //! @param [in] h1 First address hexlet.
        //! @param [in] h2 2nd address hexlet.
        //! @param [in] h3 3rd address hexlet.
        //! @param [in] h4 4th address hexlet.
        //! @param [in] h5 5th address hexlet.
        //! @param [in] h6 6th address hexlet.
        //! @param [in] h7 7th address hexlet.
        //! @param [in] h8 8th address hexlet.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        void set(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8, uint16_t port)
        {
            setAddress(h1, h2, h3, h4, h5, h6, h7, h8);
            _port = port;
        }

        //!
        //! Set the socket address from network prefix and interface identifier and port.
        //! @param [in] net Network prefix.
        //! @param [in] ifid Interface identifier.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        void set(uint64_t net, uint64_t ifid, uint16_t port)
        {
            setAddress(net, ifid);
            _port = port;
        }

        //!
        //! Check if this socket address "matches" another one.
        //! @param [in] other Another instance to compare.
        //! @return False if this and @a other addresses are both specified and
        //! are different or if the two ports are specified and different. True otherwise.
        //!
        bool match(const IPv6SocketAddress& other) const;

        //!
        //! Equality operator.
        //! @param [in] other Another instance to compare with.
        //! @return True if both object contains the same address, false otherwise.
        //!
        bool operator==(const IPv6SocketAddress& other) const { return IPv6Address::operator==(other) && _port == other._port; }
        TS_UNEQUAL_OPERATOR(IPv6SocketAddress)

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const IPv6SocketAddress& other) const;
    };

    //!
    //! Vector of socket addresses.
    //!
    typedef std::vector<IPv6SocketAddress> IPv6SocketAddressVector;

    //!
    //! Set of socket addresses.
    //!
    typedef std::set<IPv6SocketAddress> IPv6SocketAddressSet;
}
