//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a BaseMulticastTransportSession (Multicast ABR).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastTransportProtocol.h"
#include "tsmcastForwardErrorCorrectionParameters.h"
#include "tsmcastFluteSessionId.h"
#include "tsxml.h"

namespace ts::mcast {
    //!
    //! Representation of a BaseMulticastTransportSession (Multicast ABR).
    //! It serves as base type for substructures which are used in several XML tables.
    //! @see ETSI TS 103 769
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL BaseMulticastTransportSession
    {
    public:
        //!
        //! Default constructor.
        //!
        BaseMulticastTransportSession() = default;

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
        //! A URI with an associated weighting attribute.
        //!
        class TSDUCKDLL WeightedURIType
        {
        public:
            WeightedURIType() = default;   //!< Constructor.
            UString  uri {};               //!< URI
            uint32_t relative_weight = 0;  //!< Relative weight.
        };

        // Public fields coming from the XML representation.
        UString                    service_class {};         //!< attribute serviceClass.
        UString                    transport_security {};    //!< attribute transportSecurity.
        uint32_t                   bitrate_average = 0;      //!< attribute average in \<BitRate>.
        uint32_t                   bitrate_maximum = 0;      //!< attribute maximum in \<BitRate>.
        UString                    repair_obj_base_uri {};   //!< attribute transportObjectBaseURI in \<UnicastRepairParameters>.
        cn::milliseconds           repair_recv_timeout {};   //!< attribute transportObjectReceptionTimeout in \<UnicastRepairParameters>.
        cn::milliseconds           repair_fixed_backoff {};  //!< attribute fixedBackOffPeriod in \<UnicastRepairParameters>.
        cn::milliseconds           repair_rand_backoff {};   //!< attribute randomBackOffPeriod in \<UnicastRepairParameters>.
        std::list<WeightedURIType> repair_base_url {};       //!< elements \<BaseURL> in \<UnicastRepairParameters>.
        TransportProtocol          protocol {};              //!< element \<TransportProtocol>.
        std::list<FluteSessionId>  endpoints {};             //!< elements \<EndpointAddress>.
        std::list<ForwardErrorCorrectionParameters> fec {};  //!< elements \<ForwardErrorCorrectionParameters>.
   };
}
