//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an subgroup_association_descriptor (DSM-CC U-N Message DSI specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a subgroup_association_descriptor (DSM-CC U-N Message DSI specific).
    //!
    //! This descriptor cannot be present in other tables than a DSI
    //!
    //! @see ETSI TS 102 006, 9.6.2.1
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DSMCCSubgroupAssociationDescriptor: public AbstractDescriptor {
    public:
        // DSMCCSubgroupAssociationDescriptor public members:
        uint64_t subgroup_tag = 0;  //!< 40 bits, subgroup tag

        //!
        //! Default constructor.
        //!
        DSMCCSubgroupAssociationDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DSMCCSubgroupAssociationDescriptor(DuckContext& duck, const Descriptor& bin);

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
