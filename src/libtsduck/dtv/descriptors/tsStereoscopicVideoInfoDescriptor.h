//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a stereoscopic_video_info_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a stereoscopic_video_info_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.88.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL StereoscopicVideoInfoDescriptor : public AbstractDescriptor
    {
    public:
        // StereoscopicVideoInfoDescriptor public members:
        bool    base_video = false;                //!< Base video stream
        bool    leftview = false;                  //!< True if left view video stream (when base_video is true).
        bool    usable_as_2D = false;              //!< Can be sued as a 2D video stream (when base_video is false).
        uint8_t horizontal_upsampling_factor = 0;  //!< 4 bits, horizontal upsampling factor code (when base_video is false).
        uint8_t vertical_upsampling_factor = 0;    //!< 4 bits, vertical upsampling factor code (when base_video is false).

        //!
        //! Default constructor.
        //!
        StereoscopicVideoInfoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        StereoscopicVideoInfoDescriptor(DuckContext& duck, const Descriptor& bin);

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
