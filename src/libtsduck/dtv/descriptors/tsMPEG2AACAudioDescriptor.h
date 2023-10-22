//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEG2_AAC_audio_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an MPEG2_AAC_audio_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.68.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MPEG2AACAudioDescriptor : public AbstractDescriptor
    {
    public:
        // MPEG2AACAudioDescriptor public members:
        uint8_t MPEG2_AAC_profile = 0;                 //!< Profile
        uint8_t MPEG2_AAC_channel_configuration = 0;   //!< Channel configuration
        uint8_t MPEG2_AAC_additional_information = 0;  //!< Additional information

        //!
        //! Default constructor.
        //!
        MPEG2AACAudioDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MPEG2AACAudioDescriptor(DuckContext& duck, const Descriptor& bin);

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
