//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a MulticastGatewayConfigurationTransportSession
//!  (Multicast ABR).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastBaseMulticastTransportSession.h"
#include "tsxml.h"

namespace ts::mcast {
    //!
    //! Representation of a MulticastGatewayConfigurationTransportSession (Multicast ABR).
    //! This substructure is used in several XML tables such as MulticastGatewayConfiguration
    //! and MulticastServerConfiguration.
    //! @see ETSI TS 103 769, section 10.2.5
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL GatewayConfigurationTransportSession : public BaseMulticastTransportSession
    {
    public:
        //!
        //! Default constructor.
        //!
        GatewayConfigurationTransportSession() = default;

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
        //! An entry of \<PresentationManifests> or \<InitSegments> in \<ObjectCarousel>.
        //!
        class TSDUCKDLL ReferencingCarouselMediaPresentationResourceType
        {
        public:
            ReferencingCarouselMediaPresentationResourceType() = default;  //!< Constructor.
            bool    compression_preferred = false;  //!< attribute compressionPreferred
            UString target_acquisition_latency {};  //!< attribute targetAcquisitionLatency
            UString service_id_ref {};              //!< attribute serviceIdRef
            UString transport_session_id_ref {};    //!< attribute transportSessionIdRef

            //! @cond nodoxygen
            bool parseXML(const xml::Element*, bool strict);
            //! @endcond
        };

        //!
        //! An entry of \<ResourceLocator> in \<ObjectCarousel>.
        //!
        class TSDUCKDLL CarouselResourceLocatorType
        {
        public:
            CarouselResourceLocatorType() = default; //!< Constructor.
            UString uri {};                          //!< text of \<ResourceLocator>
            bool    compression_preferred = false;   //!< attribute compressionPreferred
            UString target_acquisition_latency {};   //!< attribute targetAcquisitionLatency
            UString revalidation_period {};          //!< attribute revalidationPeriod
        };

        // Public fields coming from the XML representation.
        UStringList         tags {};                                 //!< attribute tags
        UStringToUStringMap macros {};                               //!< map of \<GatewayConfigurationMacro>, indexed by attribute key.
        uint32_t            carousel_transport_size = 0;             //!< attribute aggregateTransportSize in \<ObjectCarousel>.
        uint32_t            carousel_content_size = 0;               //!< attribute aggregateContentSize in \<ObjectCarousel>.
        std::list<CarouselResourceLocatorType> resource_locator {};  //!< all \<ResourceLocator> in \<ObjectCarousel>.
        std::list<ReferencingCarouselMediaPresentationResourceType> carousel_manifests {};  //!< all \<PresentationManifests> in \<ObjectCarousel>.
        std::list<ReferencingCarouselMediaPresentationResourceType> carousel_segment {};    //!< all \<InitSegments> in \<ObjectCarousel>.
    };
}
