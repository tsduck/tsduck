//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsMPEG.h"

namespace ts {

    class TSDUCKDLL AVCSequenceParameterSet: public AbstractAVCAccessUnit
    {
    public:
        typedef AbstractAVCAccessUnit SuperClass;

        // Constructor from a binary access unit
        AVCSequenceParameterSet (const void* data = 0, size_t size = 0);

        // Clear all values
        virtual void clear();

        // Display structure content
        virtual std::ostream& display (std::ostream& = std::cout, const std::string& margin = "") const;

        // Get chroma_format_idc, applying default value (see H.264 7.4.2.1.1)
        uint8_t chroma() const {return extension1() ? chroma_format_idc : uint8_t (CHROMA_420);}

        // Get separate_colour_plane_flag, applying default value (see H.264 7.4.2.1.1)
        uint8_t separateColourPlaneFlag() const {return extension1() && chroma_format_idc == 3 ? separate_colour_plane_flag : 0;}

        // ChromaArrayType variable (see H.264 7.4.2.1.1)
        uint8_t chromaArrayType() const {return separateColourPlaneFlag() == 0 ? chroma() : 0;}

        // SubWidthC and SubHeightC variables (see H.264 6.2)
        size_t subWidthC() const;
        size_t subHeightC() const;

        // CropUnitX and CropUnitY variables (see H.264 7.4.2.1.1)
        size_t cropUnitX() const;
        size_t cropUnitY() const;

        // Frame size in pixels
        size_t frameWidth() const;
        size_t frameHeight() const;

        // Check validity of extension fields 1 (see below)
        bool extension1() const;

        // Sequence parameter set fields
        // See ISO/IEC 14496-10 sections 7.3.2.1 and 7.4.2.1
        uint8_t  profile_idc;
        uint8_t  constraint_set0_flag;
        uint8_t  constraint_set1_flag;
        uint8_t  constraint_set2_flag;
        uint8_t  constraint_set3_flag;
        uint8_t  reserved_zero_4bits;
        uint8_t  level_idc;
        uint32_t seq_parameter_set_id;
        // if (extension1()) {
            uint8_t chroma_format_idc;
            // if (chroma_format_idc == 3) {
                uint8_t separate_colour_plane_flag;
            // }
            uint32_t bit_depth_luma_minus8;
            uint32_t bit_depth_chroma_minus8;
            uint8_t  qpprime_y_zero_transform_bypass_flag;
            uint8_t  seq_scaling_matrix_present_flag;
            // scaling lists not stored in class AVCSequenceParameterSet
        // }
        uint32_t log2_max_frame_num_minus4;
        uint32_t pic_order_cnt_type;
        // if (pic_order_cnt_type == 0) {
            uint32_t log2_max_pic_order_cnt_lsb_minus4;
        // }
        // else if (pic_order_cnt_type == 1) {
            uint8_t  delta_pic_order_always_zero_flag;
            int32_t  offset_for_non_ref_pic;
            int32_t  offset_for_top_to_bottom_field;
            uint32_t num_ref_frames_in_pic_order_cnt_cycle;
            std::vector<int32_t> offset_for_ref_frame;
        // }
        uint32_t num_ref_frames;
        uint8_t  gaps_in_frame_num_value_allowed_flag;
        uint32_t pic_width_in_mbs_minus1;
        uint32_t pic_height_in_map_units_minus1;
        uint8_t  frame_mbs_only_flag;
        // if (!frame_mbs_only_flag) {
            uint8_t  mb_adaptive_frame_field_flag;
        // }
        uint8_t  direct_8x8_inference_flag;
        uint8_t  frame_cropping_flag;
        // if (frame_cropping_flag) {
            uint32_t frame_crop_left_offset;
            uint32_t frame_crop_right_offset;
            uint32_t frame_crop_top_offset;
            uint32_t frame_crop_bottom_offset;
        // }
        uint8_t  vui_parameters_present_flag;
        // if (vui_parameters_present_flag) {
            AVCVUIParameters vui;
        // }

        // Validity of RBSP trailing bits
        bool   rbsp_trailing_bits_valid;
        size_t rbsp_trailing_bits_count;

    protected:
        // Parse the body of the binary access unit. Return the "valid" flag.
        virtual bool parseBody (AVCParser&);
    };
}
