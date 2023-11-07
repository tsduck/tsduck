//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IP v6 address class
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractNetworkAddress.h"
#include "tsByteBlock.h"
#include "tsMemory.h"

namespace ts {
    //!
    //! A basic representation of an IPv6 address.
    //! @ingroup net
    //! @see https://en.wikipedia.org/wiki/IPv6_address
    //!
    //! An IPv6 address is made of 128 bits (16 bytes). It can be manipulated as
    //! - 16 bytes
    //! - 8 groups of 16 bits or hextets.
    //! - 2 64-bit values, the network prefix and the network identifier.
    //!
    //! This class is incomplete. Currently, it does not allow IPv6 networking.
    //! It is only designed to manipulate IPv6 addresses in DVB signalization.
    //!
    class TSDUCKDLL IPv6Address: public AbstractNetworkAddress
    {
    public:
        //!
        //! Size in bits of an IPv6 address.
        //!
        static constexpr size_t BITS = 128;

        //!
        //! Size in bytes of an IPv6 address.
        //!
        static constexpr size_t BYTES = 16;

        //!
        //! Wildcard integer value for "any IP address".
        //!
        static const IPv6Address AnyAddress;

        //!
        //! Local host address (::1).
        //! Usually resolves to the host name "localhost".
        //!
        static const IPv6Address LocalHost;

        //!
        //! Default constructor.
        //! The default value is AnyAddress.
        //!
        IPv6Address() { IPv6Address::clearAddress(); }

        //!
        //! Constructor from 16 bytes.
        //! @param [in] addr Address of the memory area containing the IPv6 bytes.
        //! @param [in] size Size of the memory area. If the size is shorter than 16,
        //! the IPv6 is padded on the left (most significant bytes) with zeroes.
        //! If the size is larger than 16, extra bytes are ignored.
        //!
        IPv6Address(const uint8_t *addr, size_t size) { IPv6Address::setAddress(addr, size); }

        //!
        //! Constructor from 16 bytes.
        //! @param [in] bb Byte block containing the IPv6 bytes. If the size is shorter
        //! than 16, the IPv6 is padded on the left (most significant bytes) with zeroes.
        //! If the size is larger than 16, extra bytes are ignored.
        //!
        IPv6Address(const ByteBlock& bb) { setAddress(bb); }

        //!
        //! Constructor from 8 hexlets.
        //! @param [in] h1 First address hexlet.
        //! @param [in] h2 2nd address hexlet.
        //! @param [in] h3 3rd address hexlet.
        //! @param [in] h4 4th address hexlet.
        //! @param [in] h5 5th address hexlet.
        //! @param [in] h6 6th address hexlet.
        //! @param [in] h7 7th address hexlet.
        //! @param [in] h8 8th address hexlet.
        //!
        IPv6Address(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8)
        {
            setAddress(h1, h2, h3, h4, h5, h6, h7, h8);
        }

        //!
        //! Constructor from network prefix and interface identifier.
        //! @param [in] net Network prefix.
        //! @param [in] ifid Interface identifier.
        //!
        IPv6Address(uint64_t net, uint64_t ifid) { setAddress(net, ifid); }

        // Inherited methods.
        virtual size_t binarySize() const override;
        virtual bool hasAddress() const override;
        virtual size_t getAddress(void* addr, size_t size) const override;
        virtual bool setAddress(const void* addr, size_t size) override;
        virtual void clearAddress() override;
        virtual bool isMulticast() const override;
        virtual bool resolve(const UString& name, Report& report) override;
        virtual UString toString() const override;
        virtual UString toFullString() const override;

        //!
        //! Constructor from a string in standard IPv6 numerical format.
        //! If @a name cannot be resolved, the address is set to @link AnyAddress @endlink.
        //! @param [in] name A string containing a string in standard IPv6 numerical format.
        //! @param [in] report Where to report errors.
        //! @see https://en.wikipedia.org/wiki/IPv6_address
        //!
        IPv6Address(const UString& name, Report& report) { IPv6Address::resolve(name, report); }

        //!
        //! Get the IP address as a byte block.
        //! @param [out] bb Byte block containing the IPv6 bytes.
        //!
        void getAddress(ByteBlock& bb) const { bb.copy(_bytes, sizeof(_bytes)); }

        //!
        //! Get the network prefix (64 most significant bits) of the IPv6 address.
        //! @return The network prefix (64 most significant bits) of the IPv6 address.
        //!
        uint64_t networkPrefix() const { return GetUInt64(_bytes); }

        //!
        //! Get the interface identifier (64 least significant bits) of the IPv6 address.
        //! @return The interface identifier (64 least significant bits) of the IPv6 address.
        //!
        uint64_t interfaceIdentifier() const { return GetUInt64(_bytes + 8); }

        //!
        //! Get one of the 16-bit hexlets in the address.
        //! @param [in] i Hexlet index, from 0 to 7.
        //! @return The corresponding hexlet or zero if @a i is out of range.
        //!
        uint16_t hexlet(size_t i) const;

        //!
        //! Set the IP address from 16 bytes.
        //! @param [in] bb Byte block containing the IPv6 bytes. If the size is shorter
        //! than 16, the IPv6 is padded on the left (most significant bytes) with zeroes.
        //! If the size is larger than 16, extra bytes are ignored.
        //!
        void setAddress(const ByteBlock& bb) { IPv6Address::setAddress(bb.data(), bb.size());}

        //!
        //! Set the IP address from 8 hexlets.
        //! @param [in] h1 First address hexlet.
        //! @param [in] h2 2nd address hexlet.
        //! @param [in] h3 3rd address hexlet.
        //! @param [in] h4 4th address hexlet.
        //! @param [in] h5 5th address hexlet.
        //! @param [in] h6 6th address hexlet.
        //! @param [in] h7 7th address hexlet.
        //! @param [in] h8 8th address hexlet.
        //!
        void setAddress(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8);

        //!
        //! Set the IP address from network prefix and interface identifier.
        //! @param [in] net Network prefix.
        //! @param [in] ifid Interface identifier.
        //!
        void setAddress(uint64_t net, uint64_t ifid);

        //!
        //! Check if this address "matches" another one.
        //! @param [in] other Another instance to compare.
        //! @return False if this and @a other addresses are both specified and
        //! are different. True otherwise.
        //!
        bool match(const IPv6Address& other) const;

        //!
        //! Get the IP address as a byte block.
        //! @return Byte block containing the IPv6 bytes.
        //!
        ByteBlock toBytes() const { return ByteBlock(_bytes, sizeof(_bytes)); }

        //!
        //! Equality operator.
        //! @param [in] other Another instance to compare with.
        //! @return True if both object contains the same address, false otherwise.
        //!
        bool operator==(const IPv6Address& other) const { return std::memcmp(_bytes, other._bytes, sizeof(_bytes)) == 0; }
        TS_UNEQUAL_OPERATOR(IPv6Address)

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const IPv6Address& other) const { return std::memcmp(_bytes, other._bytes, sizeof(_bytes)) < 0; }

    private:
        uint8_t _bytes[BYTES]; // Raw content of the IPv6 address.
    };

    //!
    //! Vector of IPv6 addresses.
    //!
    typedef std::vector<IPv6Address> IPv6AddressVector;

    //!
    //! Set of IPv6 addresses.
    //!
    typedef std::set<IPv6Address> IPv6AddressSet;
}
