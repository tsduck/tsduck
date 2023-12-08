//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a multiplex_buffer_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a multiplex_buffer_descriptor
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.52.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MultiplexBufferDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint32_t MB_buffer_size = 0;  //!< 24 bits, in bytes
        uint32_t TB_leak_rate = 0;    //!< 24 bits, in units of 400 bits/s

        //!
        //! Default constructor.
        //!
        MultiplexBufferDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MultiplexBufferDescriptor(DuckContext& duck, const Descriptor& bin);

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
