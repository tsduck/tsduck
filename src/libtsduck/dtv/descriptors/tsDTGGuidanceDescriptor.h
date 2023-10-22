//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DTG guidance_descriptor.
//!  This is a private descriptor, must be preceded by the DTG/OFCOM PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a DTG guidance_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the DTG/OFCOM PDS.
    //! @see The D-Book 7 Part A (DTG), section 8.5.3.20
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DTGGuidanceDescriptor : public AbstractDescriptor
    {
    public:
        // DTGGuidanceDescriptor public members:
        uint8_t   guidance_type = 0;         //!< 2-bit guidance type.
        UString   ISO_639_language_code {};  //!< 3-char language code, when guidance_type == 0 or 1.
        UString   text {};                   //!< Guidance text, when guidance_type == 0 or 1.
        bool      guidance_mode = false;     //!< Guidance mode, when guidance_type == 1.
        ByteBlock reserved_future_use {};    //!< When guidance_type >= 2.

        //!
        //! Default constructor.
        //!
        DTGGuidanceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DTGGuidanceDescriptor(DuckContext& duck, const Descriptor& bin);

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
