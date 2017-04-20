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
//!  AVC VUI (Video Usability Information) parameters
//!  (AVC, Advanced Video Coding, ISO 14496-10, ITU H.264)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractAVCStructure.h"
#include "tsAVCHRDParameters.h"

namespace ts {

    class TSDUCKDLL AVCVUIParameters: public AbstractAVCStructure
    {
    public:
        typedef AbstractAVCStructure SuperClass;

        // Constructor from a binary area
        AVCVUIParameters (const void* data = 0, size_t size = 0);

        // Clear all values
        virtual void clear();

        // Parse a memory area. Return the "valid" flag.
        virtual bool parse (const void* data, size_t size) {return AbstractAVCStructure::parse (data, size);}
        virtual bool parse (AVCParser&);

        // Display structure content
        virtual std::ostream& display (std::ostream& = std::cout, const std::string& margin = "") const;

        // VUI parameters fields
        // See ISO/IEC 14496-10 sections E.1.1 and E.2.1
        uint8_t aspect_ratio_info_present_flag;
        // if (aspect_ratio_info_present_flag) {
            uint8_t aspect_ratio_idc;
            // if (aspect_ratio_idc == 255) {  // Extended_SAR
                uint16_t sar_width;
                uint16_t sar_height;
            // }
        // }
        uint8_t overscan_info_present_flag;
        // if (overscan_info_present_flag) {
            uint8_t overscan_appropriate_flag;
        // }
        uint8_t video_signal_type_present_flag;
        // if (video_signal_type_present_flag) {
            uint8_t video_format;
            uint8_t video_full_range_flag;
            uint8_t colour_description_present_flag;
            // if (colour_description_present_flag) {
                uint8_t colour_primaries;
                uint8_t transfer_characteristics;
                uint8_t matrix_coefficients;
            // }
        // }
        uint8_t chroma_loc_info_present_flag;
        // if (chroma_loc_info_present_flag) {
            uint32_t chroma_sample_loc_type_top_field;
            uint32_t chroma_sample_loc_type_bottom_field;
        // }
        uint8_t timing_info_present_flag;
        // if (timing_info_present_flag) {
            uint32_t num_units_in_tick;
            uint32_t time_scale;
            uint8_t  fixed_frame_rate_flag;
        // }
        uint8_t nal_hrd_parameters_present_flag;
        // if (nal_hrd_parameters_present_flag) {
            AVCHRDParameters nal_hrd;
        // }
        uint8_t vcl_hrd_parameters_present_flag;
        // if (vcl_hrd_parameters_present_flag) {
            AVCHRDParameters vcl_hrd;
        // }
        // if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
            uint8_t low_delay_hrd_flag;
        // }
        uint8_t pic_struct_present_flag;
        uint8_t bitstream_restriction_flag;
        // if (bitstream_restriction_flag) {
            uint8_t  motion_vectors_over_pic_boundaries_flag;
            uint32_t max_bytes_per_pic_denom;
            uint32_t max_bits_per_mb_denom;
            uint32_t log2_max_mv_length_horizontal;
            uint32_t log2_max_mv_length_vertical;
            uint32_t num_reorder_frames;
            uint32_t max_dec_frame_buffering;
        // }
    };
}
