//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DSM-CC content_type_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a DSM-CC content_type_descriptor (DSM-CC U-N Message DII specific).
    //! This descriptor cannot be present in other tables than a DII (0x3B)
    //!
    //! @see ETSI TS 102 809, B.2.3.4
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL DSMCCContentTypeDescriptor: public AbstractDescriptor
    {
    public:
        // DSMCCContentTypeDescriptor public members:
        UString content_type {};  //!< MIME type.

        //!
        //! Default constructor.
        //!
        DSMCCContentTypeDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DSMCCContentTypeDescriptor(DuckContext& duck, const Descriptor& bin);

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
