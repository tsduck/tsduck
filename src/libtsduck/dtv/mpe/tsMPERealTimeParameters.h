//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of MPE real_time_parameters
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"

namespace ts {
    //!
    //! Representation of MPE real_time_parameters.
    //! This structure is defined in ETSI EN 301 192 and ETSI TS 102 772, with the
    //! same layout but slightly different naming of two fields.
    //! @see ETSI EN 301 192, section 9.10
    //! @see ETSI TS 102 772, section 5.3
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL MPERealTimeParameters
    {
    public:
        MPERealTimeParameters() = default;  //!< Constructor.
        uint16_t delta_t = 0;               //!< 12-bit delta t, semantics depends on the context.
        bool     table_boundary = false;    //!< 1-bit, also named MPE_boundary in ETSI TS 102 772.
        bool     frame_boundary = false;    //!< 1-bit.
        uint32_t address = 0;               //!< 18-bit, also named prev_burst_size in ETSI TS 102 772.

        //!
        //! Access to the @a table_boundary field, with ETSI TS 102 772 naming.
        //! @return A reference to the @a table_boundary field.
        //!
        bool& MPE_boundary() { return table_boundary; }

        //!
        //! Access to the @a table_boundary field, with ETSI TS 102 772 naming.
        //! @return The value of the @a table_boundary field.
        //!
        bool MPE_boundary() const { return table_boundary; }

        //!
        //! Access to the @a address field, with ETSI TS 102 772 naming.
        //! @return A reference to the @a address field.
        //!
        uint32_t& prev_burst_size() { return address; }

        //!
        //! Access to the @a address field, with ETSI TS 102 772 naming.
        //! @return The value of the @a address field.
        //!
        uint32_t prev_burst_size() const { return address; }

        //!
        //! Clear the content of the structure.
        //!
        void clearContent();

        //!
        //! Serialize the content of the structure in a binary buffer.
        //! @param [in,out] buf The serialization buffer.
        //!
        void serializePayload(PSIBuffer& buf) const;

        //!
        //! Deserialize the content of the structure from a binary buffer.
        //! @param [in,out] buf The serialization buffer.
        //!
        void deserialize(PSIBuffer& buf);

        //!
        //! This method converts this object to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the new XML tree.
        //! @param [in] use_etsi_ts_102_772_names Use ETSI TS 102 772 names for attributes.
        //! @param [in] element_name Name of the XML element describing this structure inside @a parent.
        //! @return The created XML element.
        //!
        xml::Element* buildXML(DuckContext& duck, xml::Element* parent, bool use_etsi_ts_102_772_names, const UString& element_name = u"real_time_parameters") const;

        //!
        //! This method converts an XML structure to a table or descriptor in this object.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] parent The parent XML node containing the element to convert.
        //! @param [in] use_etsi_ts_102_772_names Use ETSI TS 102 772 names for attributes.
        //! @param [in] element_name Name of the XML element describing this structure inside @a parent.
        //! @return True on success, false if the element is not found or incorrect.
        //!
        bool analyzeXML(DuckContext& duck, const xml::Element* parent, bool use_etsi_ts_102_772_names, const UString& element_name = u"real_time_parameters");

        //!
        //! A static method to display a real_time_parameters structure.
        //! @param [in,out] display Display engine.
        //! @param [in,out] buf A PSIBuffer over the structure.
        //! @param [in] margin Left margin content.
        //! @param [in] use_etsi_ts_102_772_names Use ETSI TS 102 772 names for display.
        //! @return True on success, false if the element cannot be deserialized.
        //!
        static bool Display(TablesDisplay& display, PSIBuffer& buf, const UString& margin, bool use_etsi_ts_102_772_names);
    };
}
