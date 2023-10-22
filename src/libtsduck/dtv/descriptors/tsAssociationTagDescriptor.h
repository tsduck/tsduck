//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an association_tag_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an association_tag_descriptor.
    //! @see ISO/IEC 13818-6 (DSM-CC), 11.4.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AssociationTagDescriptor : public AbstractDescriptor
    {
    public:
        // AssociationTagDescriptor public members:
        uint16_t  association_tag = 0;  //!< Association tag.
        uint16_t  use = 0;              //!< Usage of associated bitstream.
        ByteBlock selector_bytes {};    //!< Selector bytes, depend on @a use.
        ByteBlock private_data {};      //!< Private data.

        //!
        //! Default constructor.
        //!
        AssociationTagDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AssociationTagDescriptor(DuckContext& duck, const Descriptor& bin);

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
