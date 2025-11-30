//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsTransportProtocol.h"
#include "tsFluteSessionId.h"
#include "tsxml.h"

namespace ts {
    //!
    //! Representation of a MulticastGatewayConfigurationTransportSession (Multicast ABR).
    //! This substructure is used in several XML tables such as MulticastGatewayConfiguration
    //! and MulticastServerConfiguration.
    //! @see ETSI TS 103 769, section 10.2.5
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL MulticastGatewayConfigurationTransportSession
    {
    public:
        //!
        //! Default constructor.
        //!
        MulticastGatewayConfigurationTransportSession() = default;

        //!
        //! Clear the content of this object.
        //!
        void clear();

        //!
        //! Reinitialize the structure from a XML element.
        //! @param [in] element Root XML element to analyze.
        //! @return True on success, false on error.
        //!
        bool parseXML(const xml::Element* element);

        //!
        //! A URI with an associated weighting attribute.
        //!
        class TSDUCKDLL WeightedURIType
        {
        public:
            WeightedURIType() = default;   //!< Constructor.
            UString  uri {};               //!< URI
            uint32_t relative_weight = 0;  //!< Relative weight.
        };

        //!
        //! An entry of \<ForwardErrorCorrectionParameters>.
        //!
        class TSDUCKDLL ForwardErrorCorrectionParametersType
        {
        public:
            ForwardErrorCorrectionParametersType() = default;  //!< Constructor.
            UString  scheme_identifier {};                     //!< SchemeIdentifier
            uint32_t overhead_percentage = 0;                  //!< OverheadPercentage.
            std::list<FluteSessionId> endpoints {};            //!< list of \<EndpointAddress>.
        };

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
            bool parseXML(const xml::Element*);
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
        UString           service_class {};             //!< attribute serviceClass.
        UString           transport_security {};        //!< attribute transportSecurity
        UStringList       tags {};                      //!< attribute tags
        TransportProtocol protocol {};                  //!< element \<TransportProtocol>.
        uint32_t          bitrate_average = 0;          //!< attribute average in \<BitRate>.
        uint32_t          bitrate_maximum = 0;          //!< attribute maximum in \<BitRate>.
        std::list<WeightedURIType> repair_base_url {};  //!< elements \<BaseURL> in \<UnicastRepairParameters>.
        UString           repair_obj_base_uri {};       //!< attribute transportObjectBaseURI in \<UnicastRepairParameters>.
        cn::milliseconds  repair_recv_timeout {};       //!< attribute transportObjectReceptionTimeout in \<UnicastRepairParameters>.
        cn::milliseconds  repair_fixed_backoff {};      //!< attribute fixedBackOffPeriod in \<UnicastRepairParameters>.
        cn::milliseconds  repair_rand_backoff {};       //!< attribute randomBackOffPeriod in \<UnicastRepairParameters>.
        std::list<FluteSessionId> endpoints {};         //!< list of \<EndpointAddress>.
        UStringToUStringMap       macros {};            //!< map of \<MulticastGatewayConfigurationMacro>, indexed by attribute key.
        std::list<ForwardErrorCorrectionParametersType> fec {}; //!< list of \<ForwardErrorCorrectionParameters>.
        uint32_t          carousel_transport_size = 0;  //!< attribute aggregateTransportSize in \<ObjectCarousel>.
        uint32_t          carousel_content_size = 0;    //!< attribute aggregateContentSize in \<ObjectCarousel>.
        std::list<CarouselResourceLocatorType> resource_locator {}; //!< all \<ResourceLocator> in \<ObjectCarousel>.
        std::list<ReferencingCarouselMediaPresentationResourceType> carousel_manifests {};  //!< all \<PresentationManifests> in \<ObjectCarousel>.
        std::list<ReferencingCarouselMediaPresentationResourceType> carousel_segment {};    //!< all \<InitSegments> in \<ObjectCarousel>.
    };
}
