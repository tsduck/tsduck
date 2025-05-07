//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DSM-CC Tap() structure.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPSIBuffer.h"
#include "tsTablesDisplay.h"

namespace ts {
    //!
    //! DSM-CC Tap() structure.
    //! @see ISO/IEC 13818-6, 5.6.1
    //! @see ETSI TR 101 202, 4.7.2.5
    //! @see ATSC A/90, 12.2.2
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL DSMCCTap
    {
        TS_DEFAULT_COPY_MOVE(DSMCCTap);
    public:
        uint16_t id = 0;               //!< This identifies the Tap to the Client.
        uint16_t use = 0;              //!< This is a indication as to the type of the connection.
        uint16_t association_tag = 0;  //!< This tag identifies a set of U-N Network ResourceDescriptors which have the same association tag value.
        std::optional<uint16_t> selector_type {};  //!< Selector type. If unset, there is no selector.
        uint32_t transaction_id = 0;   //!< When selector_type == 1 (DSMCC_TAPSELTYPE_MESSAGE): Used for session integrity and error processing.
        uint32_t timeout = 0;          //!< When selector_type == 1: In microseconds, specific to the construction of a particular carousel.
        ByteBlock selector_bytes {};   //!< When selector_type is present and != 1: Selector bytes.

        //!
        //! Default constructor.
        //!
        DSMCCTap() = default;

        //!
        //! Total number of bytes that is required to serialize the Tap().
        //! @return The total number of bytes that is required to serialize the Tap().
        //!
        size_t binarySize() const;

        //!
        //! Clear the content of the Tap() structure.
        //!
        void clear();

        //!
        //! Serialize the Tap().
        //! @param [in,out] buf Serialization buffer.
        //!
        void serialize(PSIBuffer& buf) const;

        //!
        //! Deserialize the Tap().
        //! @param [in,out] buf Deserialization buffer.
        //!
        void deserialize(PSIBuffer& buf);

        //!
        //! A static method to display a Tap().
        //! @param [in,out] display Display engine.
        //! @param [in,out] buf A PSIBuffer over the Tap().
        //! @param [in] margin Left margin content.
        //! @return True on success, false on error.
        //!
        static bool Display(TablesDisplay& display, PSIBuffer& buf, const UString& margin);

        //!
        //! Default XML name for a Tap() structure.
        //!
        static constexpr const UChar* DEFAULT_XML_NAME = u"Tap";

        //!
        //! This method converts a Tap() to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML Tap().
        //! @param [in] xml_name Name of the XML element to generate.
        //! @return The XML element representing the Tap() or the null pointer if @a only_not_empty
        //! is true and the Tap() is empty.
        //!
        xml::Element* toXML(DuckContext& duck, xml::Element* parent, const UChar* xml_name = DEFAULT_XML_NAME) const;

        //!
        //! This method decodes an XML Tap().
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] parent The XML element containing the Tap() or the Tap() element itself if @a xml_name is null.
        //! @param [in] xml_name Expected name of the XML Tap() inside @a parent. If null, @a parent is the Tap() element.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* parent, const UChar* xml_name = DEFAULT_XML_NAME);
    };
}
