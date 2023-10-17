//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a carousel_identifier_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a carousel_identifier_descriptor.
    //! @see ISO/IEC 13818-6 (DSM-CC), 11.4.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CarouselIdentifierDescriptor : public AbstractDescriptor
    {
    public:
        // CarouselIdentifierDescriptor public members:
        uint32_t  carousel_id = 0;  //!< Carousel identifier.
        ByteBlock private_data {};  //!< Private data.

        //!
        //! Default constructor.
        //!
        CarouselIdentifierDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CarouselIdentifierDescriptor(DuckContext& duck, const Descriptor& bin);

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
