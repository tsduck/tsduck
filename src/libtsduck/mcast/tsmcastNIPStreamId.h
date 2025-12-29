//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of the DVB-NIP Stream Id.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsUString.h"
#include "tsxml.h"

namespace ts::mcast {
    //!
    //! Representation of the DVB-NIP Stream Id.
    //! @see ETSI TS 103 876, section 8.1.4.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NIPStreamId : public StringifyInterface
    {
    public:
        //!
        //! NIPNetworkID
        //! - MPE: original_network_id
        //! - GSE: interactive_network_id
        //!
        uint16_t network_id = 0;
        //!
        //! NIPCarrierID
        //! - MPE: transport_stream_id
        //! - GSE: modulation_system_id
        //!
        uint16_t carrier_id = 0;
        //!
        //! NIPLinkID
        //! - MPE: PHY_stream_id (PLP)
        //! - GSE: link_id
        //!
        uint16_t link_id = 0;
        //!
        //! NIPServiceID
        //! - MPE: service_id
        //! - GSE: 0
        //!
        uint16_t service_id = 0;

        //!
        //! Default constructor.
        //!
        NIPStreamId() = default;

        //!
        //! Clear the content of a structure.
        //!
        void clear();

        //!
        //! Read from a XML element.
        //! @param [in] element Root XML element to analyze.
        //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
        //! @return True on success, false on error.
        //!
        bool parseXML(const xml::Element* element, bool strict);

        //!
        //! Comparison operator for use as index in maps.
        //! @param [in] other Another instance to compare.
        //! @return True is this instance is logically less that @a other.
        //!
        bool operator<(const NIPStreamId& other) const;

        // Implementation of StringifyInterface.
        virtual UString toString() const override;
    };
}
