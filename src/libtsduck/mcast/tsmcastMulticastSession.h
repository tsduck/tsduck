//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a MulticastSession (Multicast ABR).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastTransportProtocol.h"
#include "tsmcastFluteSessionId.h"
#include "tsxml.h"

namespace ts::mcast {
    //!
    //! Representation of a MulticastSession (Multicast ABR).
    //! This substructure is used in several XML tables.
    //! Caution: This implementation is partial. Some part of the XML document are not deserialized.
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
        //! @return True on success, false on error.
        //!
        bool parseXML(const xml::Element* element);

        //!
        //! An entry of \<MulticastTransportSession>.
        //! @see ETSI TS 103 769, section 10.2.3
        //!
        class TSDUCKDLL MulticastTransportSession
        {
        public:
            MulticastTransportSession() = default;   //!< Constructor.
            UString id {};                           //!< attribute id.
            UString service_class {};                //!< attribute serviceClass.
            UString content_ingest_method {};        //!< attribute contentIngestMethod.
            UString transmission_mode {};            //!< attribute transmissionMode.
            UString transport_security {};           //!< attribute transportSecurity.
            TransportProtocol protocol {};           //!< element \<TransportProtocol>.
            std::list<FluteSessionId> endpoints {};  //!< list of \<EndpointAddress>.
        };

        // Public fields coming from the XML representation.
        UString service_identifier {};                               //!< attribute serviceIdentifier.
        std::list<MulticastTransportSession> transport_sessions {};  //!< elements \<MulticastTransportSession>.
    };
}
