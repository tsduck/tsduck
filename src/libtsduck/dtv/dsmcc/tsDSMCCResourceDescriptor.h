//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DSM-CC dsmccResourceDescriptor() structure.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPSIBuffer.h"
#include "tsTablesDisplay.h"

namespace ts {
    //!
    //! DSM-CC dsmccResourceDescriptor() structure.
    //! @see ISO/IEC 13818-6, 4.7.1
    //! @see ATSC A/90, 12.3.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL DSMCCResourceDescriptor
    {
        TS_DEFAULT_COPY_MOVE(DSMCCResourceDescriptor);
    public:
        uint16_t resourceRequestId = 0;       //!< Correlate the resource specified in the Request message with the result given in the Confirm message.
        uint16_t resourceDescriptorType = 0;  //!< Defines the specific resource being requested.
        uint16_t resourceNum = 0;             //!< A unique number.
        uint16_t associationTag = 0;          //!< Identifies the groups of resources or shared resources that together make up an end-to-end connection.
        uint8_t  resourceFlags = 0;           //!< Resource flags.
        uint8_t  resourceStatus = 0;          //!< Status of the requested resource between the Server and the Network or Client.
        uint16_t resourceDataFieldCount = 0;  //!< Indicates the total number of data fields in the resource descriptor.
        uint32_t typeOwnerId = 0;             //!< 24 bits, required when resourceDescriptorType == 0xFFFF"
        uint32_t typeOwnerValue = 0;          //!< 24 bits, required when resourceDescriptorType == 0xFFFF">

        //!
        //! Resource descriptor data fields.
        //! The structure of resourceDescriptorDataFields() is unclear.
        //! Currently, only the raw binary content can be specified.
        //!
        ByteBlock resourceDescriptorDataFields {};

        //!
        //! Default constructor.
        //!
        DSMCCResourceDescriptor() = default;

        //!
        //! Total number of bytes that is required to serialize the dsmccResourceDescriptor().
        //! @return The total number of bytes that is required to serialize the dsmccResourceDescriptor().
        //!
        size_t binarySize() const;

        //!
        //! Clear the content of the dsmccResourceDescriptor() structure.
        //!
        void clear();

        //!
        //! Serialize the dsmccResourceDescriptor().
        //! @param [in,out] buf Serialization buffer.
        //!
        void serialize(PSIBuffer& buf) const;

        //!
        //! Deserialize the dsmccResourceDescriptor().
        //! @param [in,out] buf Deserialization buffer.
        //!
        void deserialize(PSIBuffer& buf);

        //!
        //! A static method to display a dsmccResourceDescriptor().
        //! @param [in,out] display Display engine.
        //! @param [in,out] buf A PSIBuffer over the dsmccResourceDescriptor().
        //! @param [in] margin Left margin content.
        //! @return True on success, false on error.
        //!
        static bool Display(TablesDisplay& display, PSIBuffer& buf, const UString& margin);

        //!
        //! Default XML name for a dsmccResourceDescriptor() structure.
        //!
        static constexpr const UChar* DEFAULT_XML_NAME = u"dsmccResourceDescriptor";

        //!
        //! This method converts a dsmccResourceDescriptor() to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML dsmccResourceDescriptor().
        //! @param [in] xml_name Name of the XML element to generate.
        //! @return The XML element representing the dsmccResourceDescriptor() or the null pointer if @a only_not_empty
        //! is true and the dsmccResourceDescriptor() is empty.
        //!
        xml::Element* toXML(DuckContext& duck, xml::Element* parent, const UChar* xml_name = DEFAULT_XML_NAME) const;

        //!
        //! This method decodes an XML dsmccResourceDescriptor().
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] parent The XML element containing the dsmccResourceDescriptor() or the
        //! dsmccResourceDescriptor() element itself if @a xml_name is null.
        //! @param [in] xml_name Expected name of the XML dsmccResourceDescriptor() inside @a parent.
        //! If null, @a parent is the Tap() element.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* parent, const UChar* xml_name = DEFAULT_XML_NAME);
    };
}
