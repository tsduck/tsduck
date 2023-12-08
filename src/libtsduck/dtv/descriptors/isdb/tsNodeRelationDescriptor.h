//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB node_relation_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB node_relation_descriptor.
    //! @see ARIB STD-B10, Part 3, 5.2.3
    //! @ingroup descriptor
    //!
    class TSDUCKDLL NodeRelationDescriptor : public AbstractDescriptor
    {
    public:
        // NodeRelationDescriptor public members:
        uint8_t                 reference_type = 0;          //!< 4 bits, reference type.
        std::optional<uint16_t> information_provider_id {};  //!< Optional information provider id.
        std::optional<uint16_t> event_relation_id {};        //!< Optional event relation id.
        uint16_t                reference_node_id = 0;       //!< Reference node id.
        uint8_t                 reference_number = 0;        //!< Reference number.

        //!
        //! Default constructor.
        //!
        NodeRelationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        NodeRelationDescriptor(DuckContext& duck, const Descriptor& bin);

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
