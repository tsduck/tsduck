//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a ReportingLocator (Multicast ABR).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxml.h"
#include "tsUString.h"

namespace ts::mcast {
    //!
    //! Representation of a ReportingLocator (Multicast ABR).
    //! This substructure is used in several XML tables.
    //! @see ETSI TS 103 769, A.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL ReportingLocator
    {
    public:
        //!
        //! Default constructor.
        //!
        ReportingLocator() = default;

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
        UString          uri {};                                 //!< Content of element \<ReportingLocator>.
        double           proportion = 1.0;                       //!< Attribut "proportion".
        cn::milliseconds period {};                              //!< Attribut "period".
        cn::milliseconds random_delay {};                        //!< Attribut "randomDelay".
        bool             report_session_running_events = false;  //!< Attribut "reportSessionRunningEvents".
    };
}
