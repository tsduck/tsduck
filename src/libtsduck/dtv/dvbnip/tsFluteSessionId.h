//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a FLUTE session identification.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsIPSocketAddress.h"

namespace ts {
    //!
    //! Representation of a FLUTE session identification.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteSessionId : public StringifyInterface
    {
    public:
        IPAddress       source {};       //!< Source IP address.
        IPSocketAddress destination {};  //!< Destination IP address and UDP port.
        uint64_t        tsi = 0;         //!< Transport Session Identifier.

        //!
        //! Default constructor.
        //!
        FluteSessionId() = default;

        //!
        //! Explicit constructor.
        //! @param [in] source Source IP address.
        //! @param [in] destination Destination IP address and UDP port.
        //! @param [in] tsi Transport Session Identifier.
        //!
        FluteSessionId(const IPAddress& source, const IPSocketAddress& destination, uint64_t tsi);

        //!
        //! Comparison operator for use as index in maps.
        //! @param [in] other Another instance to compare.
        //! @return True is this instance is logically less that @a other.
        //!
        bool operator<(const FluteSessionId& other) const;

        //!
        //! Check if this session id "matches" another one.
        //! @param [in] other Another instance to compare.
        //! @return False if this and @a other addresses and porsts are both specified and are different.
        //! True otherwise.
        //!
        bool match(const FluteSessionId& other) const;

        // Implementation of StringifyInterface.
        virtual UString toString() const override;
    };
}
