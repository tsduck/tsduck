//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a ServiceListEntryPoints (DVB-I and DVB-NIP).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteFile.h"
#include "tsReport.h"
#include "tsxml.h"

namespace ts::mcast {
    //!
    //! Representation of a ServiceListEntryPoints (DVB-I and DVB-NIP).
    //! Caution: This implementation is partial. Some part of the XML document are not deserialized.
    //! @see ETSI TS 103 770, section 5.3
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL ServiceListEntryPoints : public FluteFile
    {
        TS_RULE_OF_FIVE(ServiceListEntryPoints, override);
    public:
        //!
        //! Default constructor.
        //!
        ServiceListEntryPoints() = default;

        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] file Received file from FLUTE demux.
        //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
        //!
        ServiceListEntryPoints(Report& report, const FluteFile& file, bool strict);

        //!
        //! Definition of an ExtendedURIType or ExtendedURIPathType element.
        //!
        class TSDUCKDLL ExtendedURI
        {
        public:
            //!
            //! Constructor.
            //! @param [in] element XML element containing the extended URI.
            //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
            //!
            ExtendedURI(const xml::Element* element = nullptr, bool strict = true);

            //!
            //! Constructor from a parent element.
            //! @param [in] parent Parent XML element containing the extended URI.
            //! @param [in] element Name of the child element containing the extended URI.
            //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
            //!
            ExtendedURI(const xml::Element* parent, const UString& element, bool strict);

            bool    valid = false;  //!< Element was correctly deserialized.
            UString uri {};         //!< URI.
            UString type {};        //!< MIME type.
        };

        //!
        //! Definition of an OrganizationType element.
        //!
        class TSDUCKDLL Organization
        {
        public:
            //!
            //! Constructor.
            //! @param [in] element XML element containing the organization.
            //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
            //!
            Organization(const xml::Element* element = nullptr, bool strict = true);

            //!
            //! Constructor from a parent element.
            //! @param [in] parent Parent XML element containing the organization.
            //! @param [in] element Name of the child element containing the organization.
            //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
            //!
            Organization(const xml::Element* parent, const UString& element, bool strict);

            bool    valid = false;      //!< Element was correctly deserialized.
            bool    regulator = false;  //!< Attribute "regulatorFlag".
            UString name {};            //!< Main name (other names are not retained).
        };

        //!
        //! Definition of a \<ServiceListOffering> element in a \<ProviderOffering>.
        //!
        class TSDUCKDLL ServiceListOffering
        {
        public:
            //!
            //! Constructor.
            //! @param [in] element XML element containing the organization.
            //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
            //!
            ServiceListOffering(const xml::Element* element = nullptr, bool strict = true);

            bool    valid = false;            //!< Element was correctly deserialized.
            bool    regulator = false;        //!< Attribute "regulatorListFlag".
            UString lang {};                  //!< Attribute "xml:lang"
            UString name {};                  //!< First element \<ServiceListName>.
            UString list_id {};               //!< Element \<ServiceListId>.
            std::list<ExtendedURI> lists {};  //!< Elements \<ServiceListURI>.
        };

        //!
        //! Definition of a \<ProviderOffering> element in \<ServiceListEntryPoints>.
        //!
        class TSDUCKDLL ProviderOffering
        {
        public:
            //!
            //! Constructor.
            //! @param [in] element XML element containing the organization.
            //! @param [in] strict Strict XML parsing, do not tolerate missing mandatory elements or attributes.
            //!
            ProviderOffering(const xml::Element* element = nullptr, bool strict = true);

            bool         valid = false;               //!< Element was correctly deserialized.
            Organization provider {};                 //!< Element \<Provider>.
            std::list<ServiceListOffering> lists {};  //!< Elements \<ServiceListOffering>.
        };

        // ServiceListEntryPoints public fields.
        uint32_t version = 0;                      //!< Attribute "version"
        UString  lang {};                          //!< Attribute "xml:lang"
        std::list<Organization>     entities {};   //!< Elements \<ServiceListRegistryEntity>
        std::list<ProviderOffering> providers {};  //!< Elements \<ProviderOffering>
    };
}
