//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an AAC_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an AAC_descriptor.
    //! @see ETSI EN 300 468, H.2.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AACDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t                profile_and_level = 0;  //!< See ETSI EN 300 468, H.2.1.
        bool                   SAOC_DE = false;        //!< See ETSI EN 300 468, H.2.1.
        std::optional<uint8_t> AAC_type {};            //!< See ETSI EN 300 468, H.2.1.
        ByteBlock              additional_info {};     //!< See ETSI EN 300 468, H.2.1.

        //!
        //! Default constructor.
        //!
        AACDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AACDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Get the string representation of the AAC type.
        //! @return The string representation of the AAC type.
        //!
        UString aacTypeString() const;

        //!
        //! Get the string representation of an AAC type.
        //! @param [in] type The AAC type.
        //! @return The string representation of the AAC type.
        //!
        static UString aacTypeString(uint8_t type);

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
