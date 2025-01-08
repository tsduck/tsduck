//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a label_descriptor (DSM-CC U-N Message DII specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Representation of a label_descriptor.
    //! @see ETSI TS 102 727 V1.1.1 (2010-01), B.2.2.4.1
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DSMCCLabelDescriptor: public AbstractDescriptor {
    public:
        // DSMCCLabelDescriptor public members:
        UString label {};  //!< Label of the module.

        //!
        //! Default constructor.
        //!
        DSMCCLabelDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DSMCCLabelDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer& buf) const override;
        virtual void deserializePayload(PSIBuffer& buf) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}  // namespace ts
