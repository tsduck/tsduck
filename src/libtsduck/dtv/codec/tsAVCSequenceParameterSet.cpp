//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAVCSequenceParameterSet.h"
#include "tsAVC.h"
#include "tsMPEG2.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AVCSequenceParameterSet::AVCSequenceParameterSet(const uint8_t* data, size_t size)
{
    AVCSequenceParameterSet::parse(data, size);
}


//----------------------------------------------------------------------------
// The various chroma information.
//----------------------------------------------------------------------------

uint8_t ts::AVCSequenceParameterSet::chroma() const
{
    return extension1() ? chroma_format_idc : uint8_t(CHROMA_420);
}

uint8_t ts::AVCSequenceParameterSet::separateColourPlaneFlag() const
{
    return extension1() && chroma_format_idc == 3 ? separate_colour_plane_flag : 0;
}

uint8_t ts::AVCSequenceParameterSet::chromaArrayType() const
{
    return separateColourPlaneFlag() == 0 ? chroma() : 0;
}


//----------------------------------------------------------------------------
// SubWidthC and SubHeightC variables (see H.264 6.2)
//----------------------------------------------------------------------------

uint32_t ts::AVCSequenceParameterSet::subWidthC() const
{
    switch (chroma()) {
        case 1:  return 2;
        case 2:  return 2;
        case 3:  return separateColourPlaneFlag() ? 0 : 1;
        default: return 0;
    }
}

uint32_t ts::AVCSequenceParameterSet::subHeightC() const
{
    switch (chroma()) {
        case 1:  return 2;
        case 2:  return 1;
        case 3:  return separateColourPlaneFlag() ? 0 : 1;
        default: return 0;
    }
}


//----------------------------------------------------------------------------
// CropUnitX and CropUnitY variables (see H.264 7.4.2.1.1)
//----------------------------------------------------------------------------

uint32_t ts::AVCSequenceParameterSet::cropUnitX() const
{
    return chromaArrayType() == 0 ? 1 : subWidthC();
}

uint32_t ts::AVCSequenceParameterSet::cropUnitY() const
{
    assert(frame_mbs_only_flag < 2);
    return (chromaArrayType() == 0 ? uint32_t(1) : subHeightC()) * (2 - frame_mbs_only_flag);
}


//----------------------------------------------------------------------------
// Frame size in pixels
//----------------------------------------------------------------------------

uint32_t ts::AVCSequenceParameterSet::frameWidth() const
{
    if (!valid) {
        return 0;
    }
    uint32_t width = uint32_t(MACROBLOCK_WIDTH * (pic_width_in_mbs_minus1 + 1));
    if (frame_cropping_flag) {
        width -= cropUnitX() * (frame_crop_left_offset + frame_crop_right_offset);
    }
    return width;
}

uint32_t ts::AVCSequenceParameterSet::frameHeight() const
{
    if (!valid) {
        return 0;
    }
    assert(frame_mbs_only_flag < 2);
    uint32_t height = uint32_t(MACROBLOCK_HEIGHT * (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1));
    if (frame_cropping_flag) {
        height -= cropUnitY() * (frame_crop_top_offset + frame_crop_bottom_offset);
    }
    return height;
}


//----------------------------------------------------------------------------
// Check validity of extension fields 1
//----------------------------------------------------------------------------

bool ts::AVCSequenceParameterSet::extension1() const
{
    return valid && (profile_idc == 100 ||
                     profile_idc == 110 ||
                     profile_idc == 122 ||
                     profile_idc == 244 ||
                     profile_idc == 44 ||
                     profile_idc == 83 ||
                     profile_idc == 86);
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::AVCSequenceParameterSet::clear()
{
    SuperClass::clear();
    profile_idc = 0;
    constraint_set0_flag = 0;
    constraint_set1_flag = 0;
    constraint_set2_flag = 0;
    constraint_set3_flag = 0;
    reserved_zero_4bits = 0;
    level_idc = 0;
    seq_parameter_set_id = 0;
    chroma_format_idc = 0;
    separate_colour_plane_flag = 0;
    bit_depth_luma_minus8 = 0;
    bit_depth_chroma_minus8 = 0;
    qpprime_y_zero_transform_bypass_flag = 0;
    seq_scaling_matrix_present_flag = 0;
    log2_max_frame_num_minus4 = 0;
    pic_order_cnt_type = 0;
    log2_max_pic_order_cnt_lsb_minus4 = 0;
    delta_pic_order_always_zero_flag = 0;
    offset_for_non_ref_pic = 0;
    offset_for_top_to_bottom_field = 0;
    num_ref_frames_in_pic_order_cnt_cycle = 0;
    offset_for_ref_frame.clear();
    num_ref_frames = 0;
    gaps_in_frame_num_value_allowed_flag = 0;
    pic_width_in_mbs_minus1 = 0;
    pic_height_in_map_units_minus1 = 0;
    frame_mbs_only_flag = 0;
    mb_adaptive_frame_field_flag = 0;
    direct_8x8_inference_flag = 0;
    frame_cropping_flag = 0;
    frame_crop_left_offset = 0;
    frame_crop_right_offset = 0;
    frame_crop_top_offset = 0;
    frame_crop_bottom_offset = 0;
    vui_parameters_present_flag = 0;
    vui.clear();
    rbsp_trailing_bits_valid = false;
    rbsp_trailing_bits_count = 0;
}


//----------------------------------------------------------------------------
// Parse the body of the binary access unit. Return the "valid" flag.
//----------------------------------------------------------------------------

bool ts::AVCSequenceParameterSet::parseBody(AVCParser& parser, std::initializer_list<uint32_t>)
{
    valid =
        nal_unit_type == AVC_AUT_SEQPARAMS &&
        parser.u(profile_idc, 8) &&
        parser.u(constraint_set0_flag, 1) &&
        parser.u(constraint_set1_flag, 1) &&
        parser.u(constraint_set2_flag, 1) &&
        parser.u(constraint_set3_flag, 1) &&
        parser.u(reserved_zero_4bits, 4) &&
        parser.u(level_idc, 8) &&
        parser.ue(seq_parameter_set_id);

    if (extension1()) {
        valid = parser.ue(chroma_format_idc);
        if (valid && chroma_format_idc == 3) {
            valid = parser.u(separate_colour_plane_flag, 1);
        }
        valid = valid &&
            parser.ue(bit_depth_luma_minus8) &&
            parser.ue(bit_depth_chroma_minus8) &&
            parser.u(qpprime_y_zero_transform_bypass_flag, 1) &&
            parser.u(seq_scaling_matrix_present_flag, 1);
        if (valid && seq_scaling_matrix_present_flag) {
            // Parse scaling lists but do not store them in this object
            int maxi = chroma_format_idc != 3 ? 8 : 12;
            for (int i = 0; valid && i < maxi; i++) {
                uint8_t seq_scaling_list_present_flag;
                valid = parser.u(seq_scaling_list_present_flag, 1);
                if (valid && seq_scaling_list_present_flag) {
                    int size_of_scaling_list = i < 6 ? 16 : 64;
                    int last_scale = 8;
                    int next_scale = 8;
                    int delta_scale;
                    for (int j = 0; valid && j < size_of_scaling_list; j++) {
                        if (next_scale != 0) {
                            valid = parser.se(delta_scale);
                            next_scale = (last_scale + delta_scale + 256) % 256;
                        }
                        last_scale = next_scale == 0 ? last_scale : next_scale;
                    }
                }
            }
        }
    }

    valid = valid &&
        parser.ue(log2_max_frame_num_minus4) &&
        parser.ue(pic_order_cnt_type);

    if (valid && pic_order_cnt_type == 0) {
        valid = parser.ue(log2_max_pic_order_cnt_lsb_minus4);
    }
    else if (valid && pic_order_cnt_type == 1) {
        valid =
            parser.u(delta_pic_order_always_zero_flag, 1) &&
            parser.se(offset_for_non_ref_pic) &&
            parser.se(offset_for_top_to_bottom_field) &&
            parser.ue(num_ref_frames_in_pic_order_cnt_cycle);
        for (uint32_t i = 0; valid && i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            int32_t n;
            valid = parser.se(n);
            if (valid) {
                offset_for_ref_frame.push_back(n);
            }
        }
    }

    valid = valid &&
        parser.ue(num_ref_frames) &&
        parser.u(gaps_in_frame_num_value_allowed_flag, 1) &&
        parser.ue(pic_width_in_mbs_minus1) &&
        parser.ue(pic_height_in_map_units_minus1) &&
        parser.u(frame_mbs_only_flag, 1);

    if (valid && frame_mbs_only_flag == 0) {
        valid = parser.u (mb_adaptive_frame_field_flag, 1);
    }

    valid = valid &&
        parser.u(direct_8x8_inference_flag, 1) &&
        parser.u(frame_cropping_flag, 1);

    if (valid && frame_cropping_flag == 1) {
        valid = valid &&
            parser.ue(frame_crop_left_offset) &&
            parser.ue(frame_crop_right_offset) &&
            parser.ue(frame_crop_top_offset) &&
            parser.ue(frame_crop_bottom_offset);
    }

    valid = valid && parser.u(vui_parameters_present_flag, 1);

    if (valid && vui_parameters_present_flag == 1) {
        valid = vui.parse(parser);
    }

    return valid;
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::AVCSequenceParameterSet::display(std::ostream& out, const UString& margin, int level) const
{
    if (!valid) {
        return out;
    }

#define DISP(n) disp(out, margin, u ## #n, n)

    DISP(forbidden_zero_bit);
    DISP(nal_ref_idc);
    DISP(nal_unit_type);
    DISP(profile_idc);
    DISP(constraint_set0_flag);
    DISP(constraint_set1_flag);
    DISP(constraint_set2_flag);
    DISP(constraint_set3_flag);
    DISP(reserved_zero_4bits);
    DISP(level_idc);
    DISP(seq_parameter_set_id);
    if (extension1()) {
        DISP(chroma_format_idc);
        if (chroma_format_idc == 3) {
            DISP(separate_colour_plane_flag);
        }
        DISP(bit_depth_luma_minus8);
        DISP(bit_depth_chroma_minus8);
        DISP(qpprime_y_zero_transform_bypass_flag);
        DISP(seq_scaling_matrix_present_flag);
    }
    DISP(log2_max_frame_num_minus4);
    DISP(pic_order_cnt_type);
    if (pic_order_cnt_type == 0) {
        DISP(log2_max_pic_order_cnt_lsb_minus4);
    }
    else if (pic_order_cnt_type == 1) {
        DISP(delta_pic_order_always_zero_flag);
        DISP(offset_for_non_ref_pic);
        DISP(offset_for_top_to_bottom_field);
        DISP(num_ref_frames_in_pic_order_cnt_cycle);
        DISP(offset_for_ref_frame);
    }
    DISP(num_ref_frames);
    DISP(gaps_in_frame_num_value_allowed_flag);
    DISP(pic_width_in_mbs_minus1);
    DISP(pic_height_in_map_units_minus1);
    DISP(frame_mbs_only_flag);
    if (!frame_mbs_only_flag) {
        DISP(mb_adaptive_frame_field_flag);
    }
    DISP(direct_8x8_inference_flag);
    DISP(frame_cropping_flag);
    if (frame_cropping_flag) {
        DISP(frame_crop_left_offset);
        DISP(frame_crop_right_offset);
        DISP(frame_crop_top_offset);
        DISP(frame_crop_bottom_offset);
    }
    DISP(vui_parameters_present_flag);
    if (vui_parameters_present_flag) {
        vui.display(out, margin + u"vui.");
    }
    DISP(rbsp_trailing_bits_valid);
    DISP(rbsp_trailing_bits_count);

    disp(out, margin, u"ChromaArrayType", chromaArrayType());
    disp(out, margin, u"SubWidthC", subWidthC());
    disp(out, margin, u"SubHeightC", subHeightC());
    disp(out, margin, u"CropUnitX", cropUnitX());
    disp(out, margin, u"CropUnitY", cropUnitY());
    disp(out, margin, u"frame width", frameWidth());
    disp(out, margin, u"frame height", frameHeight());

#undef DISP

    return out;
}
