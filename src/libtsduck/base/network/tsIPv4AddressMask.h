//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A combination of IP v4 address and network mask.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPv4Address.h"
#include "tsStringifyInterface.h"

namespace ts {
    //!
    //! A combination of IP v4 address and network mask.
    //! @ingroup net
    //!
    class TSDUCKDLL IPv4AddressMask: public StringifyInterface
    {
    public:
        IPv4Address address {};  //!< IPv4 address.
        IPv4Address mask {};     //!< Network mask.

        //!
        //! Default constructor.
        //! @param [in] a IPv4 address.
        //! @param [in] m Network mask.
        //!
        IPv4AddressMask(const IPv4Address& a = IPv4Address(), const IPv4Address& m = IPv4Address());

        //!
        //! Get the network mask size in bits.
        //! @return The mask size (e.g. 24 for mask 255.255.255.0).
        //!
        int maskSize() const;

        //!
        //! Get the associated broadcast address.
        //! @return The associated broadcast address.
        //!
        IPv4Address broadcastAddress() const;

        // Implementation of StringifyInterface
        virtual UString toString() const override;
    };

    //!
    //! Vector of IP addresses and network masks.
    //!
    typedef std::vector<IPv4AddressMask> IPv4AddressMaskVector;

    //! @cond nodoxygen
    // Legacy definitions.
    typedef IPv4AddressMask IPAddressMask;
    typedef IPv4AddressMaskVector IPAddressMaskVector;
    //! @endcond
}
