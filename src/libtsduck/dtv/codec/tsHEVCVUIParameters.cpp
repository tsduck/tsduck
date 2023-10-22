//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCVUIParameters.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::HEVCVUIParameters::HEVCVUIParameters(const uint8_t* data, size_t size, size_t sps_max_sub_layers_minus1)
{
    HEVCVUIParameters::parse(data, size, {uint32_t(sps_max_sub_layers_minus1)});
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::HEVCVUIParameters::clear()
{
    SuperClass::clear();
    aspect_ratio_info_present_flag = 0;
    aspect_ratio_idc = 0;
    sar_width = 0;
    sar_height = 0;
    overscan_info_present_flag = 0;
    overscan_appropriate_flag = 0;
    video_signal_type_present_flag = 0;
    video_format = 0;
    video_full_range_flag = 0;
    colour_description_present_flag = 0;
    colour_primaries = 0;
    transfer_characteristics = 0;
    matrix_coefficients = 0;
    chroma_loc_info_present_flag = 0;
    chroma_sample_loc_type_top_field = 0;
    chroma_sample_loc_type_bottom_field = 0;
    neutral_chroma_indication_flag = 0;
    field_seq_flag = 0;
    frame_field_info_present_flag = 0;
    default_display_window_flag = 0;
    def_disp_win_left_offset = 0;
    def_disp_win_right_offset = 0;
    def_disp_win_top_offset = 0;
    def_disp_win_bottom_offset = 0;
    vui_timing_info_present_flag = 0;
    vui_num_units_in_tick = 0;
    vui_time_scale = 0;
    vui_poc_proportional_to_timing_flag = 0;
    vui_num_ticks_poc_diff_one_minus1 = 0;
    vui_hrd_parameters_present_flag = 0;
    hrd_parameters.clear();
    bitstream_restriction_flag = 0;
    tiles_fixed_structure_flag = 0;
    motion_vectors_over_pic_boundaries_flag = 0;
    restricted_ref_pic_lists_flag = 0;
    min_spatial_segmentation_idc = 0;
    max_bytes_per_pic_denom = 0;
    max_bits_per_min_cu_denom = 0;
    log2_max_mv_length_horizontal = 0;
    log2_max_mv_length_vertical = 0;
}


//----------------------------------------------------------------------------
// Parse a memory area.
//----------------------------------------------------------------------------

bool ts::HEVCVUIParameters::parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> params)
{
    return SuperClass::parse(data, size, params);
}

bool ts::HEVCVUIParameters::parse(AVCParser& parser, std::initializer_list<uint32_t> params)
{
    clear();

    // The parameter sps_max_sub_layers_minus1 must be passed in the initializer list of the parse() methods.
    valid = params.size() >= 1;
    const uint32_t sps_max_sub_layers_minus1 = valid ? *params.begin() : 0;

    valid = valid && parser.u(aspect_ratio_info_present_flag, 1);
    if (valid && aspect_ratio_info_present_flag == 1) {
        valid = parser.u(aspect_ratio_idc, 8);
        if (valid && aspect_ratio_idc == 255) {  // EXTENDED_SAR
            valid = valid &&
                    parser.u(sar_width, 16) &&
                    parser.u(sar_height, 16);
        }
    }

    valid = valid && parser.u(overscan_info_present_flag, 1);
    if (valid && overscan_info_present_flag == 1) {
        valid = parser.u(overscan_appropriate_flag, 1);
    }

    valid = valid && parser.u(video_signal_type_present_flag, 1);
    if (valid && video_signal_type_present_flag == 1) {
        valid = parser.u(video_format, 3) &&
                parser.u(video_full_range_flag, 1) &&
                parser.u(colour_description_present_flag, 1);
        if (valid && colour_description_present_flag == 1) {
            valid = parser.u(colour_primaries, 8) &&
                    parser.u(transfer_characteristics, 8) &&
                    parser.u(matrix_coefficients, 8);
        }
    }

    valid = valid && parser.u(chroma_loc_info_present_flag, 1);
    if (valid && chroma_loc_info_present_flag == 1) {
        valid = parser.ue(chroma_sample_loc_type_top_field) &&
                parser.ue(chroma_sample_loc_type_bottom_field);
    }

    valid = valid &&
            parser.u(neutral_chroma_indication_flag, 1) &&
            parser.u(field_seq_flag, 1) &&
            parser.u(frame_field_info_present_flag, 1) &&
            parser.u(default_display_window_flag, 1);

    if (valid && default_display_window_flag == 1) {
        valid = parser.ue(def_disp_win_left_offset) &&
                parser.ue(def_disp_win_right_offset) &&
                parser.ue(def_disp_win_top_offset) &&
                parser.ue(def_disp_win_bottom_offset);
    }

    valid = valid && parser.u(vui_timing_info_present_flag, 1);
    if (valid && vui_timing_info_present_flag == 1) {
        valid = parser.u(vui_num_units_in_tick, 32) &&
                parser.u(vui_time_scale, 32) &&
                parser.u(vui_poc_proportional_to_timing_flag, 1);
        if (valid && vui_poc_proportional_to_timing_flag == 1) {
            valid = parser.ue(vui_num_ticks_poc_diff_one_minus1);
        }
        valid = valid && parser.u(vui_hrd_parameters_present_flag, 1);
        HEVC_TRACE(u"----- valid=%d, vui_hrd_parameters_present_flag=%d", valid, vui_hrd_parameters_present_flag);
        if (valid && vui_hrd_parameters_present_flag == 1) {
            valid = hrd_parameters.parse(parser, {1, sps_max_sub_layers_minus1});
            HEVC_TRACE(u"----- after hrd_parameters.parse(), valid=%d", valid);
        }
    }

    valid = valid && parser.u(bitstream_restriction_flag, 1);
    if (valid && bitstream_restriction_flag == 1) {
        valid = parser.u(tiles_fixed_structure_flag, 1) &&
                parser.u(motion_vectors_over_pic_boundaries_flag, 1) &&
                parser.u(restricted_ref_pic_lists_flag, 1) &&
                parser.ue(min_spatial_segmentation_idc) &&
                parser.ue(max_bytes_per_pic_denom) &&
                parser.ue(max_bits_per_min_cu_denom) &&
                parser.ue(log2_max_mv_length_horizontal) &&
                parser.ue(log2_max_mv_length_vertical);
    }

    return valid;
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::HEVCVUIParameters::display(std::ostream& out, const UString& margin, int level) const
{
#define DISP(n) disp(out, margin, u ## #n, n)

    if (valid) {
        DISP(aspect_ratio_info_present_flag);
        if (aspect_ratio_info_present_flag) {
            DISP(aspect_ratio_idc);
            if (aspect_ratio_idc == 255) {  // EXTENDED_SAR
                DISP(sar_width);
                DISP(sar_height);
            }
        }
        DISP(overscan_info_present_flag);
        if (overscan_info_present_flag) {
            DISP(overscan_appropriate_flag);
        }
        DISP(video_signal_type_present_flag);
        if (video_signal_type_present_flag) {
            DISP(video_format);
            DISP(video_full_range_flag);
            DISP(colour_description_present_flag);
            if (colour_description_present_flag) {
                DISP(colour_primaries);
                DISP(transfer_characteristics);
                DISP(matrix_coefficients);
            }
        }
        DISP(chroma_loc_info_present_flag);
        if (chroma_loc_info_present_flag) {
            DISP(chroma_sample_loc_type_top_field);
            DISP(chroma_sample_loc_type_bottom_field);
        }
        DISP(neutral_chroma_indication_flag);
        DISP(field_seq_flag);
        DISP(frame_field_info_present_flag);
        DISP(default_display_window_flag);
        if (default_display_window_flag) {
            DISP(def_disp_win_left_offset);
            DISP(def_disp_win_right_offset);
            DISP(def_disp_win_top_offset);
            DISP(def_disp_win_bottom_offset);
        }
        DISP(vui_timing_info_present_flag);
        if (vui_timing_info_present_flag) {
            DISP(vui_num_units_in_tick);
            DISP(vui_time_scale);
            DISP(vui_poc_proportional_to_timing_flag);
            if (vui_poc_proportional_to_timing_flag) {
                DISP(vui_num_ticks_poc_diff_one_minus1);
            }
            DISP(vui_hrd_parameters_present_flag);
            if (vui_hrd_parameters_present_flag) {
                hrd_parameters.display(out, margin + u"hrd.");
            }
        }
        DISP(bitstream_restriction_flag);
        if (bitstream_restriction_flag) {
            DISP(tiles_fixed_structure_flag);
            DISP(motion_vectors_over_pic_boundaries_flag);
            DISP(restricted_ref_pic_lists_flag);
            DISP(min_spatial_segmentation_idc);
            DISP(max_bytes_per_pic_denom);
            DISP(max_bits_per_min_cu_denom);
            DISP(log2_max_mv_length_horizontal);
            DISP(log2_max_mv_length_vertical);
        }
    }
    return out;

#undef DISP
}
