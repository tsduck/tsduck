//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB content_availability_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB content_availability_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.45
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ContentAvailabilityDescriptor : public AbstractDescriptor
    {
    public:
        // ContentAvailabilityDescriptor public members:
        bool      copy_restriction_mode = false;   //!< Copy restriction.
        bool      image_constraint_token = false;  //!< Image constraint.
        bool      retention_mode = false;          //!< Rentention mode.
        uint8_t   retention_state = 0;             //!< 3 bits.
        bool      encryption_mode = false;         //!< Encryption mode.
        ByteBlock reserved_future_use {};          //!< Additional info.

        //!
        //! Default constructor.
        //!
        ContentAvailabilityDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ContentAvailabilityDescriptor(DuckContext& duck, const Descriptor& bin);

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
