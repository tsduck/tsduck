//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a IBP_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a IBP_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.34.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL IBPDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        bool     closed_gop = false;      //!< A GOP header is encoded before every I-frame.
        bool     identical_gop = false;   //!< Number of P- and B-frames between I-frames is the same throughout the sequence
        uint16_t max_gop_length = 0;      //!< 14 bits, 0 forbidden, maximum number of pictures between any two consecutive I-pictures

        //!
        //! Default constructor.
        //!
        IBPDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        IBPDescriptor(DuckContext& duck, const Descriptor& bin);

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
