//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB logo_transmission_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB logo_transmission_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.44
    //! @ingroup descriptor
    //!
    class TSDUCKDLL LogoTransmissionDescriptor : public AbstractDescriptor
    {
    public:
        // LogoTransmissionDescriptor public members:
        uint8_t   logo_transmission_type = 0;  //!< Logo transmission type, condition all subsequent fields.
        uint16_t  logo_id = 0;                 //!< 9 bits, when logo_transmission_type is 0x01 or 0x02.
        uint16_t  logo_version = 0;            //!< 12 bits, when logo_transmission_type is 0x01.
        uint16_t  download_data_id = 0;        //!< 16 bits, when logo_transmission_type is 0x01.
        UString   logo_char {};                //!< Simple logo content, when logo_transmission_type is 0x03.
        ByteBlock reserved_future_use {};      //!< When logo_transmission_type is different from 0x01, 0x02, 0x03.

        //!
        //! Default constructor.
        //!
        LogoTransmissionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        LogoTransmissionDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
