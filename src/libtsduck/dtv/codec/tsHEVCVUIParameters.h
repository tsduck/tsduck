
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
//!  HEVC VUI (Video Usability Information) parameters.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoStructure.h"
#include "tsHEVCHRDParameters.h"

namespace ts {
    //!
    //! HEVC VUI (Video Usability Information) parameters.
    //! @ingroup mpeg
    //! @see ITU-T Rec. H.265, E.2.1
    //!
    class TSDUCKDLL HEVCVUIParameters: public AbstractVideoStructure
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractVideoStructure SuperClass;

        //!
        //! Constructor from a binary area.
        //! Note: the parameter @a sps_max_sub_layers_minus1 must be passed in the initializer list of the parse() methods.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //! @param [in] sps_max_sub_layers_minus1 Number of sub-layers minus 1 (depends on parent structure).
        //!
        HEVCVUIParameters(const uint8_t* data = nullptr, size_t size = 0, size_t sps_max_sub_layers_minus1 = 0);

        // Inherited methods
        virtual void clear() override;
        virtual bool parse(const uint8_t*, size_t, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual bool parse(AVCParser&, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual std::ostream& display(std::ostream& strm, const UString& margin = UString(), int level = Severity::Info) const override;

        // VUI parameters fields.
        // See ITU-T Rec. H.265 section E.2.1.

        uint8_t aspect_ratio_info_present_flag;                //!< aspect_ratio_info_present_flag
        // if (aspect_ratio_info_present_flag) {
            uint8_t aspect_ratio_idc;                          //!< aspect_ratio_idc
            // if (aspect_ratio_idc == EXTENDED_SAR) {         // EXTENDED_SAR = 255
                uint16_t sar_width;                            //!< sar_width
                uint16_t sar_height;                           //!< sar_height
            // }
        // }
        uint8_t overscan_info_present_flag;                    //!< overscan_info_present_flag
        // if (overscan_info_present_flag) {
            uint8_t overscan_appropriate_flag;                 //!< overscan_appropriate_flag
        // }
        uint8_t video_signal_type_present_flag;                //!< video_signal_type_present_flag
        // if (video_signal_type_present_flag) {
            uint8_t video_format;                              //!< video_format
            uint8_t video_full_range_flag;                     //!< video_full_range_flag
            uint8_t colour_description_present_flag;           //!< colour_description_present_flag
            // if (colour_description_present_flag) {
                uint8_t colour_primaries;                      //!< colour_primaries
                uint8_t transfer_characteristics;              //!< transfer_characteristics
                uint8_t matrix_coefficients;                   //!< matrix_coefficients
            // }
        // }
        uint8_t chroma_loc_info_present_flag;                  //!< chroma_loc_info_present_flag
        // if (chroma_loc_info_present_flag) {
            uint32_t chroma_sample_loc_type_top_field;         //!< chroma_sample_loc_type_top_field
            uint32_t chroma_sample_loc_type_bottom_field;      //!< chroma_sample_loc_type_bottom_field
        // }
        uint8_t neutral_chroma_indication_flag;                //!< neutral_chroma_indication_flag
        uint8_t field_seq_flag;                                //!< field_seq_flag
        uint8_t frame_field_info_present_flag;                 //!< frame_field_info_present_flag
        uint8_t default_display_window_flag;                   //!< default_display_window_flag
        // if (default_display_window_flag) {
            uint32_t def_disp_win_left_offset;                 //!< def_disp_win_left_offset
            uint32_t def_disp_win_right_offset;                //!< def_disp_win_right_offset
            uint32_t def_disp_win_top_offset;                  //!< def_disp_win_top_offset
            uint32_t def_disp_win_bottom_offset;               //!< def_disp_win_bottom_offset
        // }
        uint8_t vui_timing_info_present_flag;                  //!< vui_timing_info_present_flag
        // if (vui_timing_info_present_flag) {
            uint32_t vui_num_units_in_tick;                    //!< vui_num_units_in_tick
            uint32_t vui_time_scale;                           //!< vui_time_scale
            uint8_t  vui_poc_proportional_to_timing_flag;      //!< vui_poc_proportional_to_timing_flag
            // if (vui_poc_proportional_to_timing_flag) {
                uint32_t vui_num_ticks_poc_diff_one_minus1;    //!< vui_num_ticks_poc_diff_one_minus1
            // }
            uint8_t vui_hrd_parameters_present_flag;           //!< vui_hrd_parameters_present_flag
            // if (vui_hrd_parameters_present_flag) {
                HEVCHRDParameters hrd_parameters;              //!< hrd_parameters
            // }
        // }
        uint8_t bitstream_restriction_flag;                    //!< bitstream_restriction_flag
        // if (bitstream_restriction_flag ) {
            uint8_t  tiles_fixed_structure_flag;               //!< tiles_fixed_structure_flag
            uint8_t  motion_vectors_over_pic_boundaries_flag;  //!< motion_vectors_over_pic_boundaries_flag
            uint8_t  restricted_ref_pic_lists_flag;            //!< restricted_ref_pic_lists_flag
            uint32_t min_spatial_segmentation_idc;             //!< min_spatial_segmentation_idc
            uint32_t max_bytes_per_pic_denom;                  //!< max_bytes_per_pic_denom
            uint32_t max_bits_per_min_cu_denom;                //!< max_bits_per_min_cu_denom
            uint32_t log2_max_mv_length_horizontal;            //!< log2_max_mv_length_horizontal
            uint32_t log2_max_mv_length_vertical;              //!< log2_max_mv_length_vertical
        // }
    };
}
