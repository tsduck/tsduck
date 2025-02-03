//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a info_descriptor (DSM-CC U-N Message DSI/DII specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a info_descriptor (DSM-CC U-N Message DSI/DII specific).
    //! @see ETSI EN 301 192 V1.7.1 (2021-08), 10.2.4
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL DSMCCInfoDescriptor: public AbstractDescriptor
    {
    public:
        // DSMCCInfoDescriptor public members:
        UString language_code {};  //!< ISO-639 language code, 3 chars.
        UString info {};           //!< Module or Group info.

        //!
        //! Default constructor.
        //!
        DSMCCInfoDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DSMCCInfoDescriptor(DuckContext& duck, const Descriptor& bin);

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
