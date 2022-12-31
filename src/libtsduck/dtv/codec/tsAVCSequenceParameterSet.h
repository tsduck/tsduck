//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an AVC sequence parameter set access unit.
//!  AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractAVCAccessUnit.h"
#include "tsAVCVUIParameters.h"

namespace ts {
    //!
    //! Representation of an AVC sequence parameter set access unit.
    //! @ingroup mpeg
    //!
    //! AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
    //!
    class TSDUCKDLL AVCSequenceParameterSet: public AbstractAVCAccessUnit
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractAVCAccessUnit SuperClass;

        //!
        //! Constructor from a binary area.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //!
        AVCSequenceParameterSet(const uint8_t* data = nullptr, size_t size = 0);

        // Inherited methods
        virtual void clear() override;
        virtual std::ostream& display(std::ostream& strm = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

        //!
        //! Get chroma_format_idc, applying default value (see H.264 7.4.2.1.1).
        //! @return The chroma_format_idc, applying default value.
        //!
        uint8_t chroma() const;

        //!
        //! Get separate_colour_plane_flag, applying default value (see H.264 7.4.2.1.1).
        //! @return The separate_colour_plane_flag, applying default value.
        //!
        uint8_t separateColourPlaneFlag() const;

        //!
        //! The ChromaArrayType variable (see H.264 7.4.2.1.1).
        //! @return The ChromaArrayType variable.
        //!
        uint8_t chromaArrayType() const;

        //!
        //! The SubWidthC variable (see H.264 6.2).
        //! @return The SubWidthC variable (see H.264 6.2).
        //!
        uint32_t subWidthC() const;

        //!
        //! The SubHeightC variable (see H.264 6.2).
        //! @return The SubHeightC variable (see H.264 6.2).
        //!
        uint32_t subHeightC() const;

        //!
        //! The CropUnitX variable (see H.264 7.4.2.1.1).
        //! @return The CropUnitX variable.
        //!
        uint32_t cropUnitX() const;

        //!
        //! The CropUnitY variable (see H.264 7.4.2.1.1).
        //! @return The CropUnitY variable.
        //!
        uint32_t cropUnitY() const;

        //!
        //! Frame width in pixels.
        //! @return The frame width in pixels.
        //!
        uint32_t frameWidth() const;

        //!
        //! Frame height in pixels.
        //! @return The frame height in pixels.
        //!
        uint32_t frameHeight() const;

        //!
        //! Check validity of extension fields 1.
        //! @return True if extension fields 1 are valid.
        //!
        bool extension1() const;

        // Sequence parameter set fields.
        // See ISO/IEC 14496-10 sections 7.3.2.1 and 7.4.2.1.

        uint8_t  profile_idc;                               //!< profile_idc
        uint8_t  constraint_set0_flag;                      //!< constraint_set0_flag
        uint8_t  constraint_set1_flag;                      //!< constraint_set1_flag
        uint8_t  constraint_set2_flag;                      //!< constraint_set2_flag
        uint8_t  constraint_set3_flag;                      //!< constraint_set3_flag
        uint8_t  reserved_zero_4bits;                       //!< reserved_zero_4bits
        uint8_t  level_idc;                                 //!< level_idc
        uint32_t seq_parameter_set_id;                      //!< seq_parameter_set_id
        // if (extension1()) {
            uint8_t chroma_format_idc;                      //!< chroma_format_idc
            // if (chroma_format_idc == 3) {
                uint8_t separate_colour_plane_flag;         //!< separate_colour_plane_flag
            // }
            uint32_t bit_depth_luma_minus8;                 //!< bit_depth_luma_minus8
            uint32_t bit_depth_chroma_minus8;               //!< bit_depth_chroma_minus8
            uint8_t  qpprime_y_zero_transform_bypass_flag;  //!< qpprime_y_zero_transform_bypass_flag
            uint8_t  seq_scaling_matrix_present_flag;       //!< seq_scaling_matrix_present_flag
            // scaling lists not stored in class AVCSequenceParameterSet
        // }
        uint32_t log2_max_frame_num_minus4;                 //!< log2_max_frame_num_minus4
        uint32_t pic_order_cnt_type;                        //!< pic_order_cnt_type
        // if (pic_order_cnt_type == 0) {
            uint32_t log2_max_pic_order_cnt_lsb_minus4;     //!< log2_max_pic_order_cnt_lsb_minus4
        // }
        // else if (pic_order_cnt_type == 1) {
            uint8_t  delta_pic_order_always_zero_flag;      //!< delta_pic_order_always_zero_flag
            int32_t  offset_for_non_ref_pic;                //!< offset_for_non_ref_pic
            int32_t  offset_for_top_to_bottom_field;        //!< offset_for_top_to_bottom_field
            uint32_t num_ref_frames_in_pic_order_cnt_cycle; //!< num_ref_frames_in_pic_order_cnt_cycle
            std::vector<int32_t> offset_for_ref_frame;      //!< offset_for_ref_frame
        // }
        uint32_t num_ref_frames;                            //!< num_ref_frames
        uint8_t  gaps_in_frame_num_value_allowed_flag;      //!< gaps_in_frame_num_value_allowed_flag
        uint32_t pic_width_in_mbs_minus1;                   //!< pic_width_in_mbs_minus1
        uint32_t pic_height_in_map_units_minus1;            //!< pic_height_in_map_units_minus1
        uint8_t  frame_mbs_only_flag;                       //!< frame_mbs_only_flag
        // if (!frame_mbs_only_flag) {
            uint8_t  mb_adaptive_frame_field_flag;          //!< mb_adaptive_frame_field_flag
        // }
        uint8_t  direct_8x8_inference_flag;                 //!< direct_8x8_inference_flag
        uint8_t  frame_cropping_flag;                       //!< frame_cropping_flag
        // if (frame_cropping_flag) {
            uint32_t frame_crop_left_offset;                //!< frame_crop_left_offset
            uint32_t frame_crop_right_offset;               //!< frame_crop_right_offset
            uint32_t frame_crop_top_offset;                 //!< frame_crop_top_offset
            uint32_t frame_crop_bottom_offset;              //!< frame_crop_bottom_offset
        // }
        uint8_t  vui_parameters_present_flag;               //!< vui_parameters_present_flag
        // if (vui_parameters_present_flag) {
            AVCVUIParameters vui;                           //!< vui
        // }

    protected:
        // Inherited methods
        virtual bool parseBody(AVCParser&, std::initializer_list<uint32_t>) override;
    };
}
