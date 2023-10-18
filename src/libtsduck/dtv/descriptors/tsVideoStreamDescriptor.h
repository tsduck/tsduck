//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an video_stream_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an video_stream_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL VideoStreamDescriptor : public AbstractDescriptor
    {
    public:
        // VideoStreamDescriptor public members:
        bool    multiple_frame_rate = false;       //!< Has multiple frame rates.
        uint8_t frame_rate_code = 0;               //!< 4 bits, frame rate code, one of FPS_*.
        bool    MPEG_1_only = false;               //!< No MPEG-2 parameter when true.
        bool    constrained_parameter = false;     //!< Has contrained parameter.
        bool    still_picture = false;             //!< Contains still pictures.
        uint8_t profile_and_level_indication = 0;  //!< Profile (MPEG-2 only).
        uint8_t chroma_format = 0;                 //!< 2 bits, chroma formzt value, one of CHROMA_*.
        bool    frame_rate_extension = false;      //!< Extended frame rate format.

        //!
        //! Default constructor.
        //!
        VideoStreamDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        VideoStreamDescriptor(DuckContext& duck, const Descriptor& bin);

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
