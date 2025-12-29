//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a ServiceList (DVB-I and DVB-NIP).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteFile.h"
#include "tsReport.h"
#include "tsxml.h"

namespace ts::mcast {
    //!
    //! Representation of a ServiceList (DVB-I and DVB-NIP).
    //! Caution: This implementation is partial. Some part of the XML document are not deserialized.
    //! @see ETSI TS 103 770, section 5.5
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL ServiceList : public FluteFile
    {
        TS_RULE_OF_FIVE(ServiceList, override);
    public:
        //!
        //! Default constructor.
        //!
        ServiceList() = default;

        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] file Received file from FLUTE demux.
        //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
        //!
        ServiceList(Report& report, const FluteFile& file, bool strict);

        //!
        //! Definition of a \<LCN>.
        //!
        class TSDUCKDLL LCNTableEntry
        {
        public:
            LCNTableEntry() = default;    //!< Constructor.
            bool     selectable = false;  //!< Attribute "selectable".
            bool     visible = false;     //!< Attribute "visible".
            uint32_t channel_number = 0;  //!< Attribute "channelNumber".
            UString  service_ref {};      //!< Attribute "serviceRef".
        };

        //!
        //! Definition of a \<LCNTable>.
        //!
        class TSDUCKDLL LCNTable
        {
        public:
            //!
            //! Constructor.
            //! @param [in] element XML element containing the object.
            //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
            //!
            LCNTable(const xml::Element* element = nullptr, bool strict = true);

            bool valid = false;                   //!< Element was correctly deserialized.
            bool preserve_broadcast_lcn = false;  //!< Attribute "preserveBroadcastLCN".
            std::list<LCNTableEntry> lcns {};     //!< Elements \<LCN>.
        };

        //!
        //! Definition of a \<ServiceInstance>.
        //! There are multiple types of delivery parameters. Only a subset is deserialized.
        //! The \<IdentifierBasedDeliveryParameters> is used for HLS services and contains the HLS playlist.
        //! However, sometimes, they use \<OtherDeliveryParameters> for HLS services.
        //!
        class TSDUCKDLL ServiceInstance
        {
        public:
            ServiceInstance() = default;     //!< Constructor.
            uint32_t priority = 0;           //!< Attribute "priority".
            UString  id {};                  //!< Attribute "id"
            UString  lang {};                //!< Attribute "xml:lang"
            UString  media_params {};        //!< Media playlist, manifest, etc.
            UString  media_params_type {};   //!< Media parameters type.
        };

        //!
        //! Definition of a \<Service> or \<TestService>.
        //!
        class TSDUCKDLL ServiceType
        {
        public:
            //!
            //! Constructor.
            //! @param [in] element XML element containing the object.
            //! @param [in] test True if this is a test service.
            //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
            //!
            ServiceType(const xml::Element* element = nullptr, bool test = false, bool strict = true);

            bool     valid = false;                   //!< Element was correctly deserialized.
            bool     test_service = false;            //!< This is a \<TestService>, not a \<Service>.
            bool     dynamic = false;                 //!< Attribute "dynamic".
            bool     replay_available = false;        //!< Attribute "replayAvailable".
            uint32_t version = 0;                     //!< Attribute "version"
            UString  lang {};                         //!< Attribute "xml:lang"
            UString  unique_id {};                    //!< Element \<UniqueIdentifier>.
            UString  service_name {};                 //!< Element \<ServiceName>.
            UString  service_type {};                 //!< Element \<ServiceType>.
            UString  provider_name {};                //!< Element \<ProviderName>.
            std::list<ServiceInstance> instances {};  //!< Elements \<ServiceInstance>.
        };

        // ServiceListEntryPoints public fields.
        uint32_t version = 0;                //!< Attribute "version"
        UString  list_id {};                 //!< Attribute "id"
        UString  lang {};                    //!< Attribute "xml:lang"
        UString  list_name {};               //!< First element \<Name>.
        UString  provider_name {};           //!< First element \<ProviderName>.
        std::list<ServiceType> services {};  //!< Elements \<Service> and \<TestService>.
        std::list<LCNTable> lcn_tables {};   //!< Elements \<LCNTable> in \<LCNTableList>.
    };
}
