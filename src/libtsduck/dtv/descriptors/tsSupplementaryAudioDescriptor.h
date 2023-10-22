//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a supplementary_audio_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a supplementary_audio_descriptor.
    //! @see ETSI EN 300 468, 6.4.11.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SupplementaryAudioDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t   mix_type = 0;                  //!< Complete or dependent stream, 1 bit.
        uint8_t   editorial_classification = 0;  //!< Editorial classification, 5 bits.
        UString   language_code {};              //!< ISO-639 language code, 3 characters or empty.
        ByteBlock private_data {};               //!< Private data.

        //!
        //! Default constructor.
        //!
        SupplementaryAudioDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SupplementaryAudioDescriptor(DuckContext& duck, const Descriptor& bin);

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
