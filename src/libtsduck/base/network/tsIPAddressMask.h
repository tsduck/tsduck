//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A combination of IP address and network mask or prefix.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPAddress.h"
#include "tsStringifyInterface.h"

namespace ts {

    class IPAddressMask;
    using IPAddressMaskVector = std::vector<IPAddressMask>;  //!< Vector of IP addresses and network masks.

    //!
    //! A combination of IP address and network mask or prefix.
    //! @ingroup net
    //!
    class TSDUCKDLL IPAddressMask: public IPAddress
    {
    private:
        size_t _prefix = 0; // Prefix size in bits
    public:
        //!
        //! Default constructor.
        //! @param [in] addr IP address.
        //! @param [in] prefix Prefix size in bits.
        //!
        IPAddressMask(const IPAddress& addr = IPAddress(), size_t prefix = 0);

        //!
        //! Constructor from a network mask.
        //! @param [in] addr IP address.
        //! @param [in] mask Network mask.
        //!
        IPAddressMask(const IPAddress& addr, const IPAddress& mask);

        //!
        //! Get the prefix size or network mask size in bits.
        //! @return The prefix size (e.g. 24 for mask 255.255.255.0).
        //!
        size_t prefixSize() const;

        //!
        //! Set a new prefix size.
        //! @param [in] prefix Prefix size in bits.
        //!
        void setPrefixSize(size_t prefix);

        //!
        //! Set a new network mask.
        //! @param [in] mask Network mask.
        //!
        void setMask(const IPAddress& mask);

        //!
        //! Get the associated address mask.
        //! @return The associated address mask.
        //!
        IPAddress mask() const;

        //!
        //! Get the associated broadcast address (IPv4 only).
        //! @return The associated broadcast address. With IPv6 return AnyAddress6.
        //!
        IPAddress broadcastAddress() const;

        // Inherited methods.
        virtual UString toFullString() const override;
        virtual UString toString() const override;

        //!
        //! Compute the size of a prefix from a network mask.
        //! @param [in] mask Network mask.
        //! @return The prefix size in bits.
        //!
        static size_t ComputePrefixSize(const IPAddress& mask);

    private:
        // Compute a 32-bit mask from a prefix.
        static inline uint32_t Mask32(size_t prefix)
        {
            return uint32_t(0xFFFFFFFFL << (BITS4 - std::min(prefix, BITS4)));
        }
    };
}
