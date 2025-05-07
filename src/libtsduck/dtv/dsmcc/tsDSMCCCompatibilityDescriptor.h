//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DSM-CC compatibilityDescriptor() structure.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDSMCC.h"
#include "tsPSIBuffer.h"
#include "tsTablesDisplay.h"

namespace ts {
    //!
    //! DSM-CC compatibilityDescriptor() structure.
    //! @see ISO/IEC 13818-6, 6.1
    //! @see ATSC A/90, 6.1
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL DSMCCCompatibilityDescriptor
    {
        TS_DEFAULT_COPY_MOVE(DSMCCCompatibilityDescriptor);
    public:
        //!
        //! DSM-CC SubDescriptor.
        //!
        class TSDUCKDLL SubDescriptor
        {
        public:
            SubDescriptor() = default;           //!< Default constructor.
            uint8_t   subDescriptorType = 0;     //!< Subdescriptor type.
            ByteBlock additionalInformation {};  //!< Subdescriptor payload.
        };

        //!
        //! DSM-CC descriptor in a compatibilityDescriptor() structure.
        //!
        class TSDUCKDLL Descriptor
        {
        public:
            Descriptor() = default;                     //!< Default constructor.
            uint8_t  descriptorType = DSMCC_DTYPE_PAD;  //!< Descriptor type, one of DSMCC_DTYPE_*.
            uint8_t  specifierType = DSMCC_SPTYPE_OUI;  //!< Specifier type, one of DSMCC_SPTYPE_*.
            uint32_t specifierData = 0;                 //!< Specifier value, 24 bits.
            uint16_t model = 0;                         //!< Model.
            uint16_t version = 0;                       //!< Version.
            std::list<SubDescriptor> subdescs {};       //!< List of subdescriptors.
        };

        //!
        //! The compatibilityDescriptor() structure only contains a list of descriptors.
        //!
        std::list<Descriptor> descs {};

        //!
        //! Default constructor.
        //!
        DSMCCCompatibilityDescriptor() = default;

        //!
        //! Total number of bytes that is required to serialize the compatibilityDescriptor().
        //! @return The total number of bytes that is required to serialize the compatibilityDescriptor().
        //!
        size_t binarySize() const;

        //!
        //! Clear the content of the compatibilityDescriptor() structure.
        //!
        void clear() { descs.clear(); }

        //!
        //! Check if the compatibilityDescriptor() is sempty.
        //! @return True if the compatibilityDescriptor() is empty.
        //!
        bool empty() const { return descs.empty(); }

        //!
        //! Serialize the compatibilityDescriptor().
        //! @param [in,out] buf Serialization buffer.
        //! @param [in] zero_size_if_empty If true and the compatibilityDescriptor() is empty (no descriptor),
        //! generate a zero-size structure, without number of descriptors.
        //!
        void serialize(PSIBuffer& buf, bool zero_size_if_empty = false) const;

        //!
        //! Deserialize the compatibilityDescriptor().
        //! @param [in,out] buf Deserialization buffer.
        //!
        void deserialize(PSIBuffer& buf);

        //!
        //! A static method to display a compatibilityDescriptor().
        //! @param [in,out] display Display engine.
        //! @param [in,out] buf A PSIBuffer over the compatibilityDescriptor().
        //! @param [in] margin Left margin content.
        //! @return True on success, false on error.
        //!
        static bool Display(TablesDisplay& display, PSIBuffer& buf, const UString& margin);

        //!
        //! Default XML name for a compatibilityDescriptor() structure.
        //!
        static constexpr const UChar* DEFAULT_XML_NAME = u"compatibilityDescriptor";

        //!
        //! This method converts a compatibilityDescriptor() to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML compatibilityDescriptor().
        //! @param [in] only_not_empty If true and the compatibilityDescriptor() is empty, do not insert an XML element.
        //! @param [in] xml_name Name of the XML element to generate.
        //! @return The XML element representing the compatibilityDescriptor() or the null pointer if @a only_not_empty
        //! is true and the compatibilityDescriptor() is empty.
        //!
        xml::Element* toXML(DuckContext& duck, xml::Element* parent, bool only_not_empty = false, const UChar* xml_name = DEFAULT_XML_NAME) const;

        //!
        //! This method decodes an XML compatibilityDescriptor().
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] parent The XML element containing the compatibilityDescriptor() or the compatibilityDescriptor()
        //! element itself if @a xml_name is null.
        //! @param [in] required If false, the compatibilityDescriptor element is optional.
        //! @param [in] xml_name Expected name of the XML compatibilityDescriptor() inside @a parent.
        //! If null, @a parent is the compatibilityDescriptor() element.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* parent, bool required = true, const UChar* xml_name = DEFAULT_XML_NAME);
    };
}
