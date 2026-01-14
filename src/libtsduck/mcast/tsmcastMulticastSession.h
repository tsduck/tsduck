//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a MulticastSession (Multicast ABR).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastBaseMulticastTransportSession.h"
#include "tsmcastReportingLocator.h"
#include "tsxml.h"

namespace ts::mcast {
    //!
    //! Representation of a MulticastSession (Multicast ABR).
    //! This substructure is used in several XML tables.
    //! @see ETSI TS 103 769, section 10.2.2.1
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL MulticastSession
    {
    public:
        //!
        //! Default constructor.
        //!
        MulticastSession() = default;

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

        //!
        //! An entry of \<PresentationManifestLocator>.
        //! @see ETSI TS 103 769, section 10.2.2.1
        //!
        class TSDUCKDLL PresentationManifestLocator
        {
        public:
            PresentationManifestLocator() = default;   //!< Constructor.
            UString uri {};                            //!< Content of element \<PresentationManifestLocator>.
            UString manifest_id {};                    //!< attribute manifestId.
            UString content_type {};                   //!< attribute contentType.
            UString transport_object_uri {};           //!< attribute transportObjectURI.
            UString content_playback_path_pattern {};  //!< attribute contentPlaybackPathPattern.
        };

        //!
        //! An entry of \<MulticastTransportSession>.
        //! @see ETSI TS 103 769, section 10.2.3.1
        //!
        class TSDUCKDLL MulticastTransportSession : public BaseMulticastTransportSession
        {
        public:
            MulticastTransportSession() = default;   //!< Constructor.
            UString id {};                           //!< attribute id.
            UString content_ingest_method {};        //!< attribute contentIngestMethod.
            UString transmission_mode {};            //!< attribute transmissionMode.
        };

        // Public fields coming from the XML representation.
        UString          service_identifier {};                        //!< attribute serviceIdentifier.
        cn::milliseconds content_playback_availability_offset {};      //!< attribute contentPlaybackAvailabilityOffset
        std::list<PresentationManifestLocator> manifest_locators {};   //!< elements \<PresentationManifestLocator>.
        std::list<ReportingLocator>            reporting_locators {};  //!< elements \<ReportingLocator> in \<MulticastGatewaySessionReporting>
        std::list<MulticastTransportSession>   transport_sessions {};  //!< elements \<MulticastTransportSession>.
    };
}
