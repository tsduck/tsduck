//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a short_smoothing_buffer_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a short_smoothing_buffer_descriptor.
    //! @see ETSI EN 300 468, 6.2.38.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ShortSmoothingBufferDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t   sb_size = 0;       //!< 2 bits, smoothing buffer size code.
        uint8_t   sb_leak_rate = 0;  //!< 6 bits, smoothing buffer leak rate code.
        ByteBlock DVB_reserved {};   //!< Additional data.

        //!
        //! Default constructor.
        //!
        ShortSmoothingBufferDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ShortSmoothingBufferDescriptor(DuckContext& duck, const Descriptor& bin);

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
