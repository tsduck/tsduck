//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SCTE 164 EAS_metadata_descriptor
//!  (specific to a Cable Emergency Alert Table).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an SCTE 164 EAS_metadata_descriptor (specific to a Cable Emergency Alert Table).
    //!
    //! This descriptor cannot be present in other tables than a Cable Emergency Alert Table).
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see SCTE 164, 5.0
    //! @ingroup descriptor
    //!
    class TSDUCKDLL EASMetadataDescriptor : public AbstractDescriptor
    {
    public:
        // EASMetadataDescriptor public members:
        uint8_t fragment_number = 1;  //!< XML text fragment number, 1 to 255.
        UString XML_fragment {};      //!< XML text fragment.

        //!
        //! Default constructor.
        //!
        EASMetadataDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EASMetadataDescriptor(DuckContext& duck, const Descriptor& bin);

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
