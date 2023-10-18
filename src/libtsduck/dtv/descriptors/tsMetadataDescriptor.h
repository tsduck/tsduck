//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEG-defined metadata_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined metadata_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.60.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MetadataDescriptor : public AbstractDescriptor
    {
    public:
        // MetadataDescriptor public members:
        uint16_t  metadata_application_format = 0;             //!< Meta-data application format.
        uint32_t  metadata_application_format_identifier = 0;  //!< When metadata_application_format== 0xFFFF.
        uint8_t   metadata_format = 0;                         //!< Meta-data format.
        uint32_t  metadata_format_identifier = 0;              //!< When metadata_format== 0xFF.
        uint8_t   metadata_service_id = 0;                     //!< Meta-data service id.
        uint8_t   decoder_config_flags = 0;                    //!< 3 bits.
        ByteBlock service_identification {};                   //!< Service identification record.
        ByteBlock decoder_config {};                           //!< When decoder_config_flags == '001'.
        ByteBlock dec_config_identification {};                //!< When decoder_config_flags == '011'.
        uint8_t   decoder_config_metadata_service_id = 0;      //!< When decoder_config_flags == '100'.
        ByteBlock reserved_data {};                            //!< When decoder_config_flags == '101' or '110'.
        ByteBlock private_data {};                             //!< Private data.

        //!
        //! Default constructor.
        //!
        MetadataDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MetadataDescriptor(DuckContext& duck, const Descriptor& bin);

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
