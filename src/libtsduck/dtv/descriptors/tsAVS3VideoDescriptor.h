//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an AVS3_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an AVS3_video_descriptor.
    //!
    //! @see T/AI 109-6.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AVS3VideoDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t   profile_id;                        //!< Profile of the video stream.
        uint8_t   level_id;                          //!< Level of the video stream.
        bool      multiple_frame_rate_flag;          //!< Indicates multiple frame rates may be present in the video stream.
        uint8_t   frame_rate_code;                   //!< 4 bits. Code for the frame rate according to T/AI 109.2
        uint8_t   sample_precision;                  //!< 3 bits. Precision of the luma and chroma samples.
        uint8_t   chroma_format;                     //!< 2 bits. Format of the chroma component.
        bool      temporal_id_flag;                  //!< Indicates whether the video stream is allows to use the temporal_id.
        bool      td_mode_flag;                      //!< Indicates whether the video stream is monocular or multi-view.
        bool      library_stream_flag;               //!< Indicates whether the elementary stream is a library stream.
        bool      library_picture_enable_flag;       //!< Indicates whether there is an inter prediction picture using the library picture as a reference picture in the sequence stream.
        uint8_t   colour_primaries;                  //!< Chromaticity coordinates of the three primary colours of the source pictre in the video stream.
        uint8_t   transfer_characteristics;          //!< Photoelectric transfer characteristics of the source pictre in the video stream.
        uint8_t   matrix_coefficients;               //!< Conversion matrix used to convert from red, green and blue to luminance anc chromanance signals.

        //!
        //! Default constructor.
        //!
        AVS3VideoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AVS3VideoDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        static const std::vector<uint8_t> valid_profile_ids;
        static const std::vector<uint8_t> valid_level_ids;
    };
}
