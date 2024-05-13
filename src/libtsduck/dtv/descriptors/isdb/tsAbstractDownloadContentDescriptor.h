//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for ISDB download content descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Base class for ISDB download content descriptors.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AbstractDownloadContentDescriptor : public AbstractDescriptor
    {
        TS_RULE_OF_FIVE(AbstractDownloadContentDescriptor, override);
    public:
        //!
        //! ISDB download content subdescriptor.
        //!
        class TSDUCKDLL ContentSubdescriptor
        {
        public:
            uint8_t   type = 0;                   //!< Subdescriptor type.
            ByteBlock additional_information {};  //!< Subdescriptor data.

            //! @cond nodoxygen
            // Delegated methods.
            ContentSubdescriptor() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        //!
        //! ISDB download content descriptor.
        //!
        class TSDUCKDLL ContentDescriptor
        {
        public:
            uint8_t  descriptor_type = 0;                 //!< See ARIB STD-B21, 12.2.1.1.
            uint8_t  specifier_type = 0;                  //!< See ARIB STD-B21, 12.2.1.1.
            uint32_t specifier_data = 0;                  //!< 24 bits. See ARIB STD-B21, 12.2.1.1.
            uint16_t model = 0;                           //!< See ARIB STD-B21, 12.2.1.1.
            uint16_t version = 0;                         //!< See ARIB STD-B21, 12.2.1.1.
            std::list<ContentSubdescriptor> subdescs {};  //!< List of subdescriptors.

            //! @cond nodoxygen
            // Delegated methods.
            ContentDescriptor() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        //!
        //! ISDB download content compatibility descriptor.
        //!
        class TSDUCKDLL CompatibilityDescriptor
        {
        public:
            std::list<ContentDescriptor> descs {}; //!< List of compatibility desciptors.

            //! @cond nodoxygen
            // Delegated methods.
            CompatibilityDescriptor() = default;
            bool empty() const { return descs.empty(); }
            void clear() { descs.clear(); }
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* parent);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        //!
        //! ISDB download content module.
        //!
        class TSDUCKDLL Module
        {
        public:
            uint16_t  module_id = 0;    //!< See ARIB STD-B21, 12.2.1.1.
            uint32_t  module_size = 0;  //!< See ARIB STD-B21, 12.2.1.1.
            ByteBlock module_info {};   //!< See ARIB STD-B21, 12.2.1.1.

            //! @cond nodoxygen
            // Delegated methods.
            Module() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

        //!
        //! ISDB download text info.
        //!
        class TSDUCKDLL TextInfo
        {
        public:
            UString ISO_639_language_code {};  //!< ISO-639 language code, 3 characters.
            UString text {};                   //!< Text info.

            //! @cond nodoxygen
            // Delegated methods.
            TextInfo() = default;
            void serializePayload(PSIBuffer&) const;
            void deserializePayload(PSIBuffer&);
            void buildXML(DuckContext& duck, xml::Element* root) const;
            bool analyzeXML(DuckContext& duck, const xml::Element* element);
            static bool Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin);
            //! @endcond
        };

    protected:
        //!
        //! Protected constructor for subclasses.
        //! @param [in] tag Descriptor tag.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] pds Required private data specifier if this is a private descriptor.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractDownloadContentDescriptor(DID tag, const UChar* xml_name, Standards standards, PDS pds, const UChar* xml_legacy_name = nullptr) :
            AbstractDescriptor(tag, xml_name, standards, pds, xml_legacy_name)
        {
        }
    };
}
