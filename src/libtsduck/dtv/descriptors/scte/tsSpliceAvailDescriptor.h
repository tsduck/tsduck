//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SCTE 35 avail_descriptor (SIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsSCTE35.h"

namespace ts {
    //!
    //! Representation of an SCTE 35 avail_descriptor (SIT specific).
    //!
    //! This descriptor cannot be present in other tables than an Splice
    //! Information Table (SIT) because its tag reuses an MPEG-defined one.
    //!
    //! @see SCTE 35, 10.3.1
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SpliceAvailDescriptor : public AbstractDescriptor
    {
    public:
        // SpliceAvailDescriptor public members:
        uint32_t identifier = SPLICE_ID_CUEI;  //!< Descriptor owner, 0x43554549 ("CUEI").
        uint32_t provider_avail_id = 0;        //!< Provider id.

        //!
        //! Default constructor.
        //!
        SpliceAvailDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SpliceAvailDescriptor(DuckContext& duck, const Descriptor& bin);

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
