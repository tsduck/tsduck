//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SCTE 35 time_descriptor (SIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsSCTE35.h"

namespace ts {
    //!
    //! Representation of an SCTE 35 time_descriptor (SIT specific).
    //!
    //! This descriptor cannot be present in other tables than an Splice
    //! Information Table (SIT) because its tag reuses an MPEG-defined one.
    //!
    //! @see SCTE 35, 10.3.4
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SpliceTimeDescriptor : public AbstractDescriptor
    {
    public:
        // SpliceTimeDescriptor public members:
        uint32_t identifier = SPLICE_ID_CUEI;  //!< Descriptor owner, 0x43554549 ("CUEI").
        uint64_t TAI_seconds = 0;              //!< 48 bits, International Atomic Time (TAI) seconds.
        uint32_t TAI_ns = 0;                   //!< TAI nano-seconds.
        uint16_t UTC_offset = 0;               //!< Offset in seconds from UTC time.

        //!
        //! Default constructor.
        //!
        SpliceTimeDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SpliceTimeDescriptor(DuckContext& duck, const Descriptor& bin);

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
