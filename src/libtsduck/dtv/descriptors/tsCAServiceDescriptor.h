//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB CA_service_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB CA_service_descriptor.
    //! @see ARIB STD-B25, Part 1, 4.7.3
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CAServiceDescriptor : public AbstractDescriptor
    {
    public:
        // CAServiceDescriptor public members:
        uint16_t              CA_system_id = 0;             //!< Conditional access system id as defined in ARIB STD-B10, Part 2, Annex M.
        uint8_t               ca_broadcaster_group_id = 0;  //!< CA broadcaster group.
        uint8_t               message_control = 0;          //!< Delay time in days.
        std::vector<uint16_t> service_ids {};               //!< List of service ids.

        //!
        //! Default constructor.
        //!
        CAServiceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CAServiceDescriptor(DuckContext& duck, const Descriptor& bin);

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
