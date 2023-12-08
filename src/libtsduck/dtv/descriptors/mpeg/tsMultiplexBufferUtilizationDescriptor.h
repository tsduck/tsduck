//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a multiplex_buffer_utilization_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an multiplex_buffer_utilization_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.22.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MultiplexBufferUtilizationDescriptor : public AbstractDescriptor
    {
    public:
        // MultiplexBufferUtilizationDescriptor public members:
        std::optional<uint16_t> LTW_offset_lower_bound {};  //!< 15 bits, in units of (27 MHz/300) clock periods
        std::optional<uint16_t> LTW_offset_upper_bound {};  //!< 15 bits, in units of (27 MHz/300) clock periods

        //!
        //! Default constructor.
        //!
        MultiplexBufferUtilizationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MultiplexBufferUtilizationDescriptor(DuckContext& duck, const Descriptor& bin);

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
