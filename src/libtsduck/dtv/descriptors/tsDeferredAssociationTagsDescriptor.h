//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a deferred_association_tags_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a deferred_association_tags_descriptor.
    //! @see ISO/IEC 13818-6 (DSM-CC), 11.4.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DeferredAssociationTagsDescriptor : public AbstractDescriptor
    {
    public:
        // DeferredAssociationTagsDescriptor public members:
        std::vector<uint16_t> association_tags {};      //!< Association tags.
        uint16_t              transport_stream_id = 0;  //!< Transport stream id.
        uint16_t              program_number = 0;       //!< Program number.
        ByteBlock             private_data {};          //!< Private data.

        //!
        //! Default constructor.
        //!
        DeferredAssociationTagsDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DeferredAssociationTagsDescriptor(DuckContext& duck, const Descriptor& bin);

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
