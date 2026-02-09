//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DSM-CC LiteComponent structure (BIOP::Object Location, DSM::ConnBinder)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDSMCC.h"
#include "tsDSMCCTap.h"
#include "tsPSIBuffer.h"
#include "tsTablesDisplay.h"
#include "tsxmlElement.h"

namespace ts {
    //!
    //! Representation of LiteComponent structure (BIOP::Object Location, DSM::ConnBinder)
    //! @see ISO/IEC 13818-6
    //! @see ETSI TR 101 202 V1.2.1 (2003-01), Table 4.5
    //!
    class TSDUCKDLL DSMCCLiteComponent
    {
    public:
        uint32_t component_id_tag = 0;  //!< Component idenfitier tag (eg. TAG_ObjectLocation, TAG_ConnBinder).

        // BIOPObjectLocation context
        uint32_t carousel_id = 0;      //!< The carouselId field provides a context for the moduleId field.
        uint16_t module_id = 0;        //!< Identifies the module in which the object is conveyed within the carousel.
        uint8_t version_major = 0x01;  //!< Fixed, BIOP protocol major version 1.
        uint8_t version_minor = 0x00;  //!< Fixed, BIOP protocol minor version 0.
        ByteBlock object_key_data {};  //!< Identifies the object within the module in which it is broadcast.

        // DSMConnBinder context
        DSMCCTap tap {};  //!< Tap structure

        // UnknownComponent context
        std::optional<ByteBlock> component_data {};  //!< Optional component data, for UnknownComponent.

        //!
        //! Default constructor.
        //!
        DSMCCLiteComponent() = default;

        //!
        //! Serialize the LiteComponent structure.
        //! @param [in,out] buf Serialization buffer.
        //!
        void serialize(PSIBuffer& buf) const;

        //!
        //! Deserialize the LiteComponent structure.
        //! @param [in,out] buf Deserialization buffer.
        //!
        void deserialize(PSIBuffer& buf);

        //!
        //! Display the LiteComponent structure.
        //! @param [in,out] display Display engine.
        //! @param [in,out] buf A PSIBuffer over the LiteComponent.
        //! @param [in] margin Left margin content.
        //!
        static void Display(TablesDisplay& display, PSIBuffer& buf, const UString& margin);

        //!
        //! This method converts a LiteComponent to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML LiteComponent.
        //!
        void toXML(DuckContext& duck, xml::Element* parent) const;

        //!
        //! This method decodes an XML LiteComponent.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element The XML element containing the LiteComponent.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* element);
    };
}  // namespace ts
