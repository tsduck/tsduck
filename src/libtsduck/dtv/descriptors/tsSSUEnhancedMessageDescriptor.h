//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SSU_enhanced_message_descriptor (UNT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an SSU_enhanced_message_descriptor (UNT specific).
    //!
    //! This descriptor cannot be present in other tables than a UNT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 102 006, 9.5.2.14
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SSUEnhancedMessageDescriptor : public AbstractDescriptor
    {
    public:
        // SSUEnhancedMessageDescriptor public members:
        uint8_t descriptor_number = 0;       //!< 4 bits, descriptor number.
        uint8_t last_descriptor_number = 0;  //!< 4 bits, last descriptor number.
        UString ISO_639_language_code {};    //!< 3 char, language code.
        uint8_t message_index = 0;           //!< 5 bits, message index.
        UString text {};                     //!< Message text.

        //!
        //! Default constructor.
        //!
        SSUEnhancedMessageDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SSUEnhancedMessageDescriptor(DuckContext& duck, const Descriptor& bin);

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
