//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an IOD_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an IOD_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.40.
    //! @ingroup libtsduck descriptor
    //!
    //! This is a limited implementation. The InitialObjectDescriptor() is represented
    //! as a binary blob. A future version may chose to describe the internal structure
    //! of this data block.
    //!
    class TSDUCKDLL IODDescriptor : public AbstractDescriptor
    {
    public:
        // IODDescriptor public members:
        uint8_t   Scope_of_IOD_label = 0;      //!< Scope of IOD label, either 0x10 or 0x11.
        uint8_t   IOD_label = 0;               //!< IOD label.
        ByteBlock InitialObjectDescriptor {};  //!< Binary structure as defined in 8.6.3.1 of ISO/IEC 14496-1.

        //!
        //! Default constructor.
        //!
        IODDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        IODDescriptor(DuckContext& duck, const Descriptor& bin);

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
