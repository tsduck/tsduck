//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a ForwardErrorCorrectionParameters (Multicast ABR).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteSessionId.h"
#include "tsxml.h"

namespace ts::mcast {
    //!
    //! Representation of a ForwardErrorCorrectionParameters (Multicast ABR).
    //! This substructure is used in several XML tables.
    //! @see ETSI TS 103 769
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL ForwardErrorCorrectionParameters
    {
    public:
        //!
        //! Default constructor.
        //!
        ForwardErrorCorrectionParameters() = default;

        //!
        //! Clear the content of this object.
        //!
        void clear();

        //!
        //! Reinitialize the structure from a XML element.
        //! @param [in] element Root XML element to analyze.
        //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
        //! @return True on success, false on error.
        //!
        bool parseXML(const xml::Element* element, bool strict);

        // Public fields coming from the XML representation.
        UString  scheme_identifier {};           //!< SchemeIdentifier
        uint32_t overhead_percentage = 0;        //!< OverheadPercentage.
        std::list<FluteSessionId> endpoints {};  //!< list of \<EndpointAddress>.
    };
}
