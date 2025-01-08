//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a module_link_descriptor (DSM-CC U-N Message DSI/DII specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a module_link_descriptor (DSM-CC U-N Message DII specific).
    //! This descriptor cannot be present in other tables than a DII (0x3B)
    //!
    //! @see ETSI EN 301 192 V1.7.1 (2021-08), 10.2.5
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DSMCCModuleLinkDescriptor: public AbstractDescriptor {
    public:
        // DSMCCModuleLinkDescriptor public members:
        uint8_t  position = 0;   //!< Position of the module.
        uint16_t module_id = 0;  //!< Module Id

        //!
        //! Default constructor.
        //!
        DSMCCModuleLinkDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DSMCCModuleLinkDescriptor(DuckContext& duck, const Descriptor& bin);

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
