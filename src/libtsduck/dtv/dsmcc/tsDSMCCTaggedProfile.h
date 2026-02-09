//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DSM-CC TaggedProfile structure.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDSMCCLiteComponent.h"

namespace ts {
    //!
    //! Representation of TaggedProfile structure (BIOP Profile Body, Lite Options Profile Body)
    //! @see ISO/IEC 13818-6
    //! @see ETSI TR 101 202 V1.2.1 (2003-01), 4.7.3.2, 4.7.3.3
    //!
    class TSDUCKDLL DSMCCTaggedProfile
    {
    public:
        uint32_t profile_id_tag = 0;          //!< Profile identifier tag (eg. TAG_BIOP, TAG_LITE_OPTIONS).
        uint8_t profile_data_byte_order = 0;  //!< Fixed 0x00, big endian byte order.

        // BIOP Profile Body context
        std::list<DSMCCLiteComponent> lite_components {};  //!< List of LiteComponent.

        // Any other profile context
        std::optional<ByteBlock> profile_data {};  //!< Optional profile data, for UnknownProfile.

        //!
        //! Default constructor.
        //!
        DSMCCTaggedProfile() = default;

        //!
        //! Serialize the TaggedProfile structure.
        //! @param [in,out] buf Serialization buffer.
        //!
        void serialize(PSIBuffer& buf) const;

        //!
        //! Deserialize the TaggedProfile structure.
        //! @param [in,out] buf Deserialization buffer.
        //!
        void deserialize(PSIBuffer& buf);

        //!
        //! Display the TaggedProfile structure.
        //! @param [in,out] display Display engine.
        //! @param [in,out] buf A PSIBuffer over the TaggedProfile.
        //! @param [in] margin Left margin content.
        //!
        static void Display(TablesDisplay& display, PSIBuffer& buf, const UString& margin);

        //!
        //! This method converts a TaggedProfile to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML TaggedProfile.
        //!
        void toXML(DuckContext& duck, xml::Element* parent) const;

        //!
        //! This method decodes an XML TaggedProfile.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element The XML element containing the TaggedProfile.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* element);
    };
}  // namespace ts
