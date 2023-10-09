//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAVCVUIParameters.h"


//----------------------------------------------------------------------------
// Constructor from a binary area
//----------------------------------------------------------------------------

ts::AVCVUIParameters::AVCVUIParameters(const uint8_t* data, size_t size)
{
    AVCVUIParameters::parse(data, size);
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::AVCVUIParameters::clear()
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
    timing_info_present_flag = 0;
    num_units_in_tick = 0;
    time_scale = 0;
    fixed_frame_rate_flag = 0;
    nal_hrd_parameters_present_flag = 0;
    nal_hrd.clear();
    vcl_hrd_parameters_present_flag = 0;
    vcl_hrd.clear();
    low_delay_hrd_flag = 0;
    pic_struct_present_flag = 0;
    bitstream_restriction_flag = 0;
    motion_vectors_over_pic_boundaries_flag = 0;
    max_bytes_per_pic_denom = 0;
    max_bits_per_mb_denom = 0;
    log2_max_mv_length_horizontal = 0;
    log2_max_mv_length_vertical = 0;
    num_reorder_frames = 0;
    max_dec_frame_buffering = 0;
}


//----------------------------------------------------------------------------
// Parse a memory area. Return the "valid" flag.
//----------------------------------------------------------------------------

bool ts::AVCVUIParameters::parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> params)
{
    return SuperClass::parse(data, size, params);
}

bool ts::AVCVUIParameters::parse(AVCParser& parser, std::initializer_list<uint32_t>)
{
    clear();

    valid = parser.u(aspect_ratio_info_present_flag, 1);

    if (valid && aspect_ratio_info_present_flag == 1) {
        valid = parser.u(aspect_ratio_idc, 8);
        if (valid && aspect_ratio_idc == 255) {  // Extended_SAR
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
        valid =
            parser.u(video_format, 3) &&
            parser.u(video_full_range_flag, 1) &&
            parser.u(colour_description_present_flag, 1);
        if (valid && colour_description_present_flag == 1) {
            valid =
                parser.u(colour_primaries, 8) &&
                parser.u(transfer_characteristics, 8) &&
                parser.u(matrix_coefficients, 8);
        }
    }

    valid = valid && parser.u(chroma_loc_info_present_flag, 1);

    if (valid && chroma_loc_info_present_flag == 1) {
        valid =
            parser.ue(chroma_sample_loc_type_top_field) &&
            parser.ue(chroma_sample_loc_type_bottom_field);
    }

    valid = valid && parser.u(timing_info_present_flag, 1);

    if (valid && timing_info_present_flag == 1) {
        valid =
            parser.u(num_units_in_tick, 32) &&
            parser.u(time_scale, 32) &&
            parser.u(fixed_frame_rate_flag, 1);
    }

    valid = valid && parser.u(nal_hrd_parameters_present_flag, 1);

    if (valid && nal_hrd_parameters_present_flag == 1) {
        valid = nal_hrd.parse(parser);
    }

    valid = valid && parser.u(vcl_hrd_parameters_present_flag, 1);

    if (valid && vcl_hrd_parameters_present_flag == 1) {
        valid = vcl_hrd.parse(parser);
    }

    if (valid && (nal_hrd_parameters_present_flag == 1 || vcl_hrd_parameters_present_flag == 1)) {
        valid = parser.u(low_delay_hrd_flag, 1);
    }

    valid = valid &&
        parser.u(pic_struct_present_flag, 1) &&
        parser.u(bitstream_restriction_flag, 1);

    if (valid && bitstream_restriction_flag == 1) {
        valid =
            parser.u(motion_vectors_over_pic_boundaries_flag, 1) &&
            parser.ue(max_bytes_per_pic_denom) &&
            parser.ue(max_bits_per_mb_denom) &&
            parser.ue(log2_max_mv_length_horizontal) &&
            parser.ue(log2_max_mv_length_vertical) &&
            parser.ue(num_reorder_frames) &&
            parser.ue(max_dec_frame_buffering);
    }

    return valid;
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::AVCVUIParameters::display(std::ostream& out, const UString& margin, int level) const
{
    if (!valid) {
        return out;
    }

#define DISP(n) disp(out, margin, u ## #n, n)

    DISP(aspect_ratio_info_present_flag);
    if (aspect_ratio_info_present_flag) {
        DISP(aspect_ratio_idc);
        if (aspect_ratio_idc == 255) {  // Extended_SAR
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
    DISP(timing_info_present_flag);
    if (timing_info_present_flag) {
        DISP(num_units_in_tick);
        DISP(time_scale);
        DISP(fixed_frame_rate_flag);
    }
    DISP(nal_hrd_parameters_present_flag);
    if (nal_hrd_parameters_present_flag) {
        nal_hrd.display(out, margin + u"nal_hrd.");
    }
    DISP(vcl_hrd_parameters_present_flag);
    if (vcl_hrd_parameters_present_flag) {
        vcl_hrd.display(out, margin + u"vcl_hrd.");
    }
    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
        DISP (low_delay_hrd_flag);
    }
    DISP(pic_struct_present_flag);
    DISP(bitstream_restriction_flag);
    if (bitstream_restriction_flag) {
        DISP(motion_vectors_over_pic_boundaries_flag);
        DISP(max_bytes_per_pic_denom);
        DISP(max_bits_per_mb_denom);
        DISP(log2_max_mv_length_horizontal);
        DISP(log2_max_mv_length_vertical);
        DISP(num_reorder_frames);
        DISP(max_dec_frame_buffering);
    }

#undef DISP

    return out;
}
