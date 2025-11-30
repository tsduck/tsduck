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
#include "tsmcast.h"
#include "tsStringifyInterface.h"
#include "tsIPSocketAddress.h"
#include "tsxml.h"

namespace ts::mcast {
    //!
    //! Representation of a FLUTE session identification.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteSessionId : public StringifyInterface
    {
    public:
        IPAddress       source {};          //!< Source IP address.
        IPSocketAddress destination {};     //!< Destination IP address and UDP port.
        uint64_t        tsi = INVALID_TSI;  //!< Transport Session Identifier.

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
        //! Clear the content of this object.
        //!
        void clear();

        //!
        //! Check if this session id "matches" another one.
        //! @param [in] other Another instance to compare.
        //! @return False if any addresse, port, or TSI in @a this and @a other are both specified and are different.
        //! True otherwise.
        //!
        bool match(const FluteSessionId& other) const;

        //!
        //! Check if this session is in the DVB-NIP Announcement Channel.
        //! @return True if this session is in the DVB-NIP Announcement Channel.
        //!
        bool nipAnnouncementChannel() const;

        //!
        //! Reinitialize the structure from a XML element.
        //! @param [in] element Root XML element to analyze.
        //! @return True on success, false on error.
        //!
        bool parseXML(const xml::Element* element);

        // Implementation of StringifyInterface.
        virtual UString toString() const override;
    };
}
