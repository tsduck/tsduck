//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCSequenceParameterSet.h"
#include "tsHEVC.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HEVCSequenceParameterSet::HEVCSequenceParameterSet(const uint8_t* data, size_t size)
{
    HEVCSequenceParameterSet::parse(data, size);
}


//----------------------------------------------------------------------------
// The various picture information.
//----------------------------------------------------------------------------

uint8_t ts::HEVCSequenceParameterSet::chroma() const
{
    // Direct value, see H.265, section 6.2.
    return valid ? uint8_t(chroma_format_idc) : 0;
}

uint32_t ts::HEVCSequenceParameterSet::frameWidth() const
{
    return valid ? pic_width_in_luma_samples : 0;
}

uint32_t ts::HEVCSequenceParameterSet::frameHeight() const
{
    return valid ? pic_height_in_luma_samples : 0;
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::HEVCSequenceParameterSet::clear()
{
    SuperClass::clear();
    sps_video_parameter_set_id = 0;
    sps_max_sub_layers_minus1 = 0;
    sps_temporal_id_nesting_flag = 0;
    profile_tier_level.clear();
    sps_seq_parameter_set_id = 0;
    chroma_format_idc = 0;
    separate_colour_plane_flag = 0;
    pic_width_in_luma_samples = 0;
    pic_height_in_luma_samples = 0;
    conformance_window_flag = 0;
    conf_win_left_offset = 0;
    conf_win_right_offset = 0;
    conf_win_top_offset = 0;
    conf_win_bottom_offset = 0;
    bit_depth_luma_minus8 = 0;
    bit_depth_chroma_minus8 = 0;
    log2_max_pic_order_cnt_lsb_minus4 = 0;
    sps_sub_layer_ordering_info_present_flag = 0;
    sps_max.clear();
    log2_min_luma_coding_block_size_minus3 = 0;
    log2_diff_max_min_luma_coding_block_size = 0;
    log2_min_luma_transform_block_size_minus2 = 0;
    log2_diff_max_min_luma_transform_block_size = 0;
    max_transform_hierarchy_depth_inter = 0;
    max_transform_hierarchy_depth_intra = 0;
    scaling_list_enabled_flag = 0;
    sps_scaling_list_data_present_flag = 0;
    scaling_list_data.clear();
    amp_enabled_flag = 0;
    sample_adaptive_offset_enabled_flag = 0;
    pcm_enabled_flag = 0;
    pcm_sample_bit_depth_luma_minus1 = 0;
    pcm_sample_bit_depth_chroma_minus1 = 0;
    log2_min_pcm_luma_coding_block_size_minus3 = 0;
    log2_diff_max_min_pcm_luma_coding_block_size = 0;
    pcm_loop_filter_disabled_flag = 0;
    num_short_term_ref_pic_sets = 0;
    st_ref_pic_set.clear();
    long_term_ref_pics_present_flag = 0;
    num_long_term_ref_pics_sps = 0;
    lt_ref.clear();
    sps_temporal_mvp_enabled_flag = 0;
    strong_intra_smoothing_enabled_flag = 0;
    vui_parameters_present_flag = 0;
    vui.clear();
    sps_extension_present_flag = 0;
    sps_range_extension_flag = 0;
    sps_multilayer_extension_flag = 0;
    sps_3d_extension_flag = 0;
    sps_scc_extension_flag = 0;
    sps_extension_4bits = 0;
}


//----------------------------------------------------------------------------
// Parse the body of the binary access unit. Return the "valid" flag.
//----------------------------------------------------------------------------

bool ts::HEVCSequenceParameterSet::parseBody(AVCParser& parser, std::initializer_list<uint32_t>)
{
    valid = nal_unit_type == HEVC_AUT_SPS_NUT &&
            parser.u(sps_video_parameter_set_id, 4) &&
            parser.u(sps_max_sub_layers_minus1, 3) &&
            parser.u(sps_temporal_id_nesting_flag, 1) &&
            profile_tier_level.parse(parser, {1, sps_max_sub_layers_minus1}) &&
            parser.ue(sps_seq_parameter_set_id) &&
            parser.ue(chroma_format_idc);
    HEVC_TRACE(u"===== start of HEVCSequenceParameterSet::parseBody(), valid=%d", valid);

    if (valid && chroma_format_idc == 3) {
        valid = parser.u(separate_colour_plane_flag, 1);
    }

    valid = valid &&
            parser.ue(pic_width_in_luma_samples) &&
            parser.ue(pic_height_in_luma_samples) &&
            parser.u(conformance_window_flag, 1);

    if (valid && conformance_window_flag) {
        valid = parser.ue(conf_win_left_offset) &&
                parser.ue(conf_win_right_offset) &&
                parser.ue(conf_win_top_offset) &&
                parser.ue(conf_win_bottom_offset);
    }

    valid = valid &&
            parser.ue(bit_depth_luma_minus8) &&
            parser.ue(bit_depth_chroma_minus8) &&
            parser.ue(log2_max_pic_order_cnt_lsb_minus4) &&
            parser.u(sps_sub_layer_ordering_info_present_flag, 1);

    for (size_t i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1); valid && i <= sps_max_sub_layers_minus1; i++) {
        SPSMax sp;
        valid = parser.ue(sp.sps_max_dec_pic_buffering_minus1) &&
                parser.ue(sp.sps_max_num_reorder_pics) &&
                parser.ue(sp.sps_max_latency_increase_plus1);
        sps_max.push_back(sp);
    }

    valid = valid &&
            parser.ue(log2_min_luma_coding_block_size_minus3) &&
            parser.ue(log2_diff_max_min_luma_coding_block_size) &&
            parser.ue(log2_min_luma_transform_block_size_minus2) &&
            parser.ue(log2_diff_max_min_luma_transform_block_size) &&
            parser.ue(max_transform_hierarchy_depth_inter) &&
            parser.ue(max_transform_hierarchy_depth_intra) &&
            parser.u(scaling_list_enabled_flag, 1);

    if (valid && scaling_list_enabled_flag) {
        valid = parser.u(sps_scaling_list_data_present_flag, 1);
        if (valid && sps_scaling_list_data_present_flag) {
            valid = scaling_list_data.parse(parser);
        }
    }

    valid = valid &&
            parser.u(amp_enabled_flag, 1) &&
            parser.u(sample_adaptive_offset_enabled_flag, 1) &&
            parser.u(pcm_enabled_flag, 1);

    if (valid && pcm_enabled_flag) {
        valid = parser.u(pcm_sample_bit_depth_luma_minus1, 4) &&
                parser.u(pcm_sample_bit_depth_chroma_minus1, 4) &&
                parser.ue(log2_min_pcm_luma_coding_block_size_minus3) &&
                parser.ue(log2_diff_max_min_pcm_luma_coding_block_size) &&
                parser.u(pcm_loop_filter_disabled_flag, 1);
    }

    valid = valid && parser.ue(num_short_term_ref_pic_sets);
    st_ref_pic_set.reset(num_short_term_ref_pic_sets);
    for (uint32_t i = 0; valid && i < num_short_term_ref_pic_sets; i++) {
        valid = st_ref_pic_set.parse(parser, {i});
    }
    HEVC_TRACE(u"----- st_ref_pic_set.list.size()=%d, valid=%d", st_ref_pic_set.list.size(), valid);

    valid = valid && parser.u(long_term_ref_pics_present_flag, 1);
    if (valid && long_term_ref_pics_present_flag) {
        valid = parser.ue(num_long_term_ref_pics_sps);
        for (uint32_t i = 0; valid && i < num_long_term_ref_pics_sps; i++) {
            LongTermRef ltr;
            valid = parser.u(ltr.lt_ref_pic_poc_lsb_sps, log2_max_pic_order_cnt_lsb_minus4 + 4) &&
                    parser.u(ltr.used_by_curr_pic_lt_sps_flag, 1);
            lt_ref.push_back(ltr);
        }
    }

    valid = valid &&
            parser.u(sps_temporal_mvp_enabled_flag, 1) &&
            parser.u(strong_intra_smoothing_enabled_flag, 1) &&
            parser.u(vui_parameters_present_flag, 1);

    HEVC_TRACE(u"----- before vui.parse(), valid=%d, vui_parameters_present_flag=%d", valid, vui_parameters_present_flag);
    if (valid && vui_parameters_present_flag) {
        valid = vui.parse(parser, {sps_max_sub_layers_minus1});
    }
    HEVC_TRACE(u"----- after vui.parse(), valid=%d", valid);

    valid = valid && parser.u(sps_extension_present_flag, 1);
    if (valid && sps_extension_present_flag) {
        valid = parser.u(sps_range_extension_flag, 1) &&
                parser.u(sps_multilayer_extension_flag, 1) &&
                parser.u(sps_3d_extension_flag, 1) &&
                parser.u(sps_scc_extension_flag, 1) &&
                parser.u(sps_extension_4bits, 4);
    }

    HEVC_TRACE(u"===== end of HEVCSequenceParameterSet::parseBody(), valid=%d", valid);
    return valid;
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::HEVCSequenceParameterSet::display(std::ostream& out, const UString& margin, int level) const
{
    if (valid) {

#define DISP(n) disp(out, margin, u ## #n, n)

        DISP(forbidden_zero_bit);
        DISP(nal_unit_type);
        DISP(nuh_layer_id);
        DISP(nuh_temporal_id_plus1);

        DISP(sps_video_parameter_set_id);
        DISP(sps_max_sub_layers_minus1);
        DISP(sps_temporal_id_nesting_flag);
        profile_tier_level.display(out, margin + u"profile_tier_level.");
        DISP(sps_seq_parameter_set_id);
        DISP(chroma_format_idc);

        if (chroma_format_idc == 3) {
            DISP(separate_colour_plane_flag);
        }

        DISP(pic_width_in_luma_samples);
        DISP(pic_height_in_luma_samples);
        DISP(conformance_window_flag);

        if (conformance_window_flag) {
            DISP(conf_win_left_offset);
            DISP(conf_win_right_offset);
            DISP(conf_win_top_offset);
            DISP(conf_win_bottom_offset);
        }

        DISP(bit_depth_luma_minus8);
        DISP(bit_depth_chroma_minus8);
        DISP(log2_max_pic_order_cnt_lsb_minus4);
        DISP(sps_sub_layer_ordering_info_present_flag);

        for (size_t i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1); valid && i <= sps_max_sub_layers_minus1; i++) {
#define DISPsub(i,n) out << margin << #n << "[" << i << "] = " << int64_t(sps_max[i].n) << std::endl
            DISPsub(i, sps_max_dec_pic_buffering_minus1);
            DISPsub(i, sps_max_num_reorder_pics);
            DISPsub(i, sps_max_latency_increase_plus1);
#undef DISPsub
        }

        DISP(log2_min_luma_coding_block_size_minus3);
        DISP(log2_diff_max_min_luma_coding_block_size);
        DISP(log2_min_luma_transform_block_size_minus2);
        DISP(log2_diff_max_min_luma_transform_block_size);
        DISP(max_transform_hierarchy_depth_inter);
        DISP(max_transform_hierarchy_depth_intra);
        DISP(scaling_list_enabled_flag);

        if (scaling_list_enabled_flag) {
            DISP(sps_scaling_list_data_present_flag);
            if (sps_scaling_list_data_present_flag) {
                scaling_list_data.display(out, margin);
            }
        }

        DISP(amp_enabled_flag);
        DISP(sample_adaptive_offset_enabled_flag);
        DISP(pcm_enabled_flag);

        if (pcm_enabled_flag) {
            DISP(pcm_sample_bit_depth_luma_minus1);
            DISP(pcm_sample_bit_depth_chroma_minus1);
            DISP(log2_min_pcm_luma_coding_block_size_minus3);
            DISP(log2_diff_max_min_pcm_luma_coding_block_size);
            DISP(pcm_loop_filter_disabled_flag);
        }

        DISP(num_short_term_ref_pic_sets);
        st_ref_pic_set.display(out, margin);

        DISP(long_term_ref_pics_present_flag);
        if (long_term_ref_pics_present_flag) {
            DISP(num_long_term_ref_pics_sps);
            for (uint32_t i = 0; i < num_long_term_ref_pics_sps; i++) {
#define DISPsub(i,n) out << margin << #n << "[" << i << "] = " << int64_t(lt_ref[i].n) << std::endl
                DISPsub(i, lt_ref_pic_poc_lsb_sps);
                DISPsub(i, used_by_curr_pic_lt_sps_flag);
#undef DISPsub
            }
        }

        DISP(sps_temporal_mvp_enabled_flag);
        DISP(strong_intra_smoothing_enabled_flag);
        DISP(vui_parameters_present_flag);

        if (vui_parameters_present_flag) {
            vui.display(out, margin + u"vui.");
        }

        DISP(sps_extension_present_flag);
        if (sps_extension_present_flag) {
            DISP(sps_range_extension_flag);
            DISP(sps_multilayer_extension_flag);
            DISP(sps_3d_extension_flag);
            DISP(sps_scc_extension_flag);
            DISP(sps_extension_4bits);
        }

        disp(out, margin, u"chroma", chroma());
        disp(out, margin, u"frame width", frameWidth());
        disp(out, margin, u"frame height", frameHeight());

#undef DISP
    }
    return out;
}
