//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SCTE 35 DTMF_descriptor (SIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsSCTE35.h"

namespace ts {
    //!
    //! Representation of an SCTE 35 DTMF_descriptor (SIT specific).
    //!
    //! This descriptor cannot be present in other tables than an Splice
    //! Information Table (SIT) because its tag reuses an MPEG-defined one.
    //!
    //! @see SCTE 35, 10.3.2
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SpliceDTMFDescriptor : public AbstractDescriptor
    {
    public:
        // SpliceDTMFDescriptor public members:
        uint32_t identifier = SPLICE_ID_CUEI;  //!< Descriptor owner, 0x43554549 ("CUEI").
        uint8_t  preroll = 0;                  //!< Pre-roll time in tenths of seconds.
        UString  DTMF {};                      //!< Dial string (only '*', '#' and '0'-'9' are allowed).

        //!
        //! Maximum size of the DTMF character string.
        //! The DTMF size is stored in 3 bits in the descriptor.
        //!
        static constexpr size_t DTMF_MAX_SIZE = 7;

        //!
        //! Default constructor.
        //!
        SpliceDTMFDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SpliceDTMFDescriptor(DuckContext& duck, const Descriptor& bin);

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
