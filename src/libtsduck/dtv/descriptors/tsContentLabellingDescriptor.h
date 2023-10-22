//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEG-defined content_labelling_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined content_labelling_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.56.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ContentLabellingDescriptor : public AbstractDescriptor
    {
    public:
        // ContentLabellingDescriptor public members:
        uint16_t  metadata_application_format = 0;             //!< Meta-data application format.
        uint32_t  metadata_application_format_identifier = 0;  //!< When metadata_application_format== 0xFFFF.
        uint8_t   content_time_base_indicator = 0;             //!< 4 bits.
        ByteBlock content_reference_id {};                     //!< Content reference id data.
        uint64_t  content_time_base_value = 0;                 //!< 33 bits, when content_time_base_indicator == 1 or 2.
        uint64_t  metadata_time_base_value = 0;                //!< 33 bits, when content_time_base_indicator == 1 or 2.
        uint8_t   content_id = 0;                              //!< 7 bits, when content_time_base_indicator == 2.
        ByteBlock time_base_association_data {};               //!< When content_time_base_indicator == 3 to 7.
        ByteBlock private_data {};                             //!< Private data.

        //!
        //! Default constructor.
        //!
        ContentLabellingDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ContentLabellingDescriptor(DuckContext& duck, const Descriptor& bin);

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
