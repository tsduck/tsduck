//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a target_background_grid_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a target_background_grid_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.12.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TargetBackgroundGridDescriptor : public AbstractDescriptor
    {
    public:
        // TargetBackgroundGridDescriptor public members:
        uint16_t horizontal_size = 0;           //!< 14 bits, horizontal size.
        uint16_t vertical_size = 0;             //!< 14 bits, vertical size.
        uint8_t  aspect_ratio_information = 0;  //!< 4 bits, aspect ration code, one of AR_*.

        //!
        //! Default constructor.
        //!
        TargetBackgroundGridDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TargetBackgroundGridDescriptor(DuckContext& duck, const Descriptor& bin);

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
