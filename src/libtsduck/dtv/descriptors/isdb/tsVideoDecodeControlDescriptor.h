//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB video_decode_control_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB video_decode_control_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.30
    //! @ingroup descriptor
    //!
    class TSDUCKDLL VideoDecodeControlDescriptor : public AbstractDescriptor
    {
    public:
        // VideoDecodeControlDescriptor public members:
        bool    still_picture = false;        //!< Presence of still pictures.
        bool    sequence_end_code = false;    //!< Has sequence end code.
        uint8_t video_encode_format = 0;      //!< 4 bits.
        uint8_t reserved_future_use = 3;      //!< 2 bits.

        //!
        //! Default constructor.
        //!
        VideoDecodeControlDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        VideoDecodeControlDescriptor(DuckContext& duck, const Descriptor& bin);

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
