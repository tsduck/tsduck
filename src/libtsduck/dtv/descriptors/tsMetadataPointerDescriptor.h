//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEG-defined metadata_pointer_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined metadata_pointer_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.58.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MetadataPointerDescriptor : public AbstractDescriptor
    {
    public:
        // MetadataPointerDescriptor public members:
        uint16_t  metadata_application_format = 0;             //!< Meta-data application format.
        uint32_t  metadata_application_format_identifier = 0;  //!< When metadata_application_format== 0xFFFF.
        uint8_t   metadata_format = 0;                         //!< Meta-data format.
        uint32_t  metadata_format_identifier = 0;              //!< When metadata_format== 0xFF.
        uint8_t   metadata_service_id = 0;                     //!< Meta-data service id.
        uint8_t   MPEG_carriage_flags = 0;                     //!< 2 bits.
        ByteBlock metadata_locator {};                         //!< Meta-data locator record.
        uint16_t  program_number = 0;                          //!< When MPEG_carriage_flags <= 2.
        uint16_t  transport_stream_location = 0;               //!< When MPEG_carriage_flags == 1.
        uint16_t  transport_stream_id = 0;                     //!< When MPEG_carriage_flags == 1.
        ByteBlock private_data {};                             //!< Private data.

        //!
        //! Default constructor.
        //!
        MetadataPointerDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MetadataPointerDescriptor(DuckContext& duck, const Descriptor& bin);

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
