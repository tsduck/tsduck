//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB carousel_compatible_composite_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB carousel_compatible_composite_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.46
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CarouselCompatibleCompositeDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! ISDB carousel subdescriptor.
        //!
        class TSDUCKDLL Subdescriptor
        {
        public:
            uint8_t   type = 0;         //!< Subdescriptor type.
            ByteBlock payload {};       //!< Subdescriptor binary payload.
            Subdescriptor() = default;  //!< Default constructor.
        };

        // CarouselCompatibleCompositeDescriptor public members:
        std::list<Subdescriptor> subdescs {};  //!< List of subdescriptors.

        //!
        //! Default constructor.
        //!
        CarouselCompatibleCompositeDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CarouselCompatibleCompositeDescriptor(DuckContext& duck, const Descriptor& bin);

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
