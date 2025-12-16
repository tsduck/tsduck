//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a TransportProtocol XML structure
//!  (Multicast ABR and DVB-NIP).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcast.h"
#include "tsxml.h"

namespace ts::mcast {
    //!
    //! Representation of a TransportProtocol XML structure (Multicast ABR and DVB-NIP).
    //! This substructure is used in several XML tables.
    //! @see ETSI TS 103 769, section 10.2.2.1
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TransportProtocol
    {
    public:
        //!
        //! Default constructor.
        //!
        TransportProtocol() = default;

        //!
        //! Clear the content of this object.
        //!
        void clear();

        //!
        //! Reinitialize the structure from a XML element.
        //! @param [in] element Root XML element to analyze.
        //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
        //! @param [in] child_name Name of the child element in @a element. If empty, directly use @a element.
        //! @return True on success, false on error.
        //!
        bool parseXML(const xml::Element* element, bool strict, const UString& child_name = u"TransportProtocol");

        // Public fields coming from the XML representation.
        FileTransport protocol = FT_UNKNOWN;   //!< interpretation of attribute protocolIdentifier.
        uint32_t      version = 0;             //!< interpretation of attribute protocolVersion.
        UString       protocol_identifier {};  //!< attribute protocolIdentifier.
        UString       protocol_version {};     //!< attribute protocolVersion.
    };
}
