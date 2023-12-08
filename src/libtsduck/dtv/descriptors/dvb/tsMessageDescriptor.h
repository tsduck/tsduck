//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a message_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a message_descriptor.
    //! @see ETSI EN 300 468, 6.4.7.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MessageDescriptor : public AbstractDescriptor
    {
    public:
        // MessageDescriptor public members:
        uint8_t message_id = 0;    //!< Message identifier.
        UString language_code {};  //!< ISO-639 language code, 3 characters.
        UString message {};        //!< Message content.

        //!
        //! Default constructor.
        //!
        MessageDescriptor();

        //!
        //! Constructor.
        //! @param [in] id Message id.
        //! @param [in] lang ISO-639 language code, 3 characters.
        //! @param [in] text Message content.
        //!
        MessageDescriptor(uint8_t id, const UString& lang, const UString& text);

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MessageDescriptor(DuckContext& duck, const Descriptor& bin);

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
