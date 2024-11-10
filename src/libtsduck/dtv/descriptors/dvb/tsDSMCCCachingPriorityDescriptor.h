//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a caching_priority_descriptor (DSM-CC U-N Message DII specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a caching_priority_descriptor (DSM-CC U-N Message DII specific).
    //! This descriptor cannot be present in other tables than a DII (0x3B)
    //!
    //! @see ETSI TS 102 809 V1.3.1 (2017-06), B.2.2.4.2
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DSMCCCachingPriorityDescriptor: public AbstractDescriptor {
    public:
        // DSMCCCachingPriorityDescriptor public members:
        uint8_t priority_value = 0;      //!< Indicates the caching priority for the objects within this module.
        uint8_t transparency_level = 0;  //!< Transparency level that shall be used by the receiver if it caches objects contained in this module.

        //!
        //! Default constructor.
        //!
        DSMCCCachingPriorityDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DSMCCCachingPriorityDescriptor(DuckContext& duck, const Descriptor& bin);

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
}  // namespace ts
