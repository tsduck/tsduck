//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a location_descriptor (DSM-CC U-N Message DSI/DII specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a SSU_module_type_descriptor (DSM-CC U-N Message DII specific).
    //! This descriptor cannot be present in other tables than a DII (0x3B)
    //!
    //! @see ETSI TS 102 006 V1.4.1 (2015-06), 8.2.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DSMCCSSUModuleTypeDescriptor: public AbstractDescriptor
    {
    public:
        // DSMCCSSUModuleTypeDescriptor public members:
        uint8_t ssu_module_type = 0;  //!< SSU Module Type

        //!
        //! Default constructor.
        //!
        DSMCCSSUModuleTypeDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DSMCCSSUModuleTypeDescriptor(DuckContext& duck, const Descriptor& bin);

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
