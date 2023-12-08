//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB data_content_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB data_content_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.28
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DataContentDescriptor : public AbstractDescriptor
    {
    public:
        // DataContentDescriptor public members:
        uint16_t  data_component_id = 0;     //!< Data component id as defined in ARIB STD-B10, Part 2, Annex J.
        uint8_t   entry_component = 0;       //!< Entry component.
        ByteBlock selector_bytes {};         //!< Selector bytes.
        ByteBlock component_refs {};         //!< One byte per component reference.
        UString   ISO_639_language_code {};  //!< Language code.
        UString   text {};                   //!< Content description.

        //!
        //! Default constructor.
        //!
        DataContentDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DataContentDescriptor(DuckContext& duck, const Descriptor& bin);

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
