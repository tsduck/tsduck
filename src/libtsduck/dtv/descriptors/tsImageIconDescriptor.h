//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a image_icon_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a image_icon_descriptor
    //! @see ETSI EN 300 468, 6.4.7.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ImageIconDescriptor : public AbstractDescriptor
    {
    public:
        // ImageIconDescriptor public members:
        uint8_t   descriptor_number = 0;        //!< 4 bits, index of this descriptor for this icon.
        uint8_t   last_descriptor_number = 0;   //!< 4 bits, index of last descriptor for this icon.
        uint8_t   icon_id = 0;                  //!< 3 bits, icon id in this descriptor loop.
        uint8_t   icon_transport_mode = 0;      //!< 2 bits, when descriptor_number == 0.
        bool      has_position = false;         //!< A screen position is specified, when descriptor_number == 0.
        uint8_t   coordinate_system = 0;        //!< 3 bits, when descriptor_number == 0 and has_position == true.
        uint16_t  icon_horizontal_origin = 0;   //!< 12 bits, when descriptor_number == 0 and has_position == true.
        uint16_t  icon_vertical_origin = 0;     //!< 12 bits, when descriptor_number == 0 and has_position == true.
        UString   icon_type {};                 //!< Icon MIME type, when descriptor_number == 0.
        UString   url {};                       //!< Icon URL, when descriptor_number == 0 and icon_transport_mode == 1.
        ByteBlock icon_data {};                 //!< Icon data bytes, when descriptor_number > 0 or icon_transport_mode == 0.

        //!
        //! Default constructor.
        //!
        ImageIconDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ImageIconDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
