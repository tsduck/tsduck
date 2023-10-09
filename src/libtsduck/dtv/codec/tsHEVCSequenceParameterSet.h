//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an HEVC sequence parameter set access unit.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractHEVCAccessUnit.h"
#include "tsHEVCVUIParameters.h"
#include "tsHEVCScalingListData.h"
#include "tsHEVCProfileTierLevel.h"
#include "tsHEVCShortTermReferencePictureSetList.h"

namespace ts {
    //!
    //! Representation of an HEVC sequence parameter set access unit.
    //! @ingroup mpeg
    //! @see ITU H.265, sections 7.3.2.2 and 7.4.3.2
    //!
    class TSDUCKDLL HEVCSequenceParameterSet: public AbstractHEVCAccessUnit
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractHEVCAccessUnit SuperClass;

        //!
        //! Constructor from a binary area.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //!
        HEVCSequenceParameterSet(const uint8_t* data = nullptr, size_t size = 0);

        // Inherited methods
        virtual void clear() override;
        virtual std::ostream& display(std::ostream& strm = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

        //!
        //! Get chroma_format_idc, applying default value (see H.264 7.4.2.1.1).
        //! @return The chroma_format_idc, applying default value.
        //!
        uint8_t chroma() const;

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

        class TSDUCKDLL SPSMax                              //!< An entry for sps_max values
        {
        public:
            SPSMax() = default;                             //!< Constructor
            uint32_t sps_max_dec_pic_buffering_minus1 = 0;  //!< sps_max_dec_pic_buffering_minus1
            uint32_t sps_max_num_reorder_pics = 0;          //!< sps_max_num_reorder_pics
            uint32_t sps_max_latency_increase_plus1 = 0;    //!< sps_max_latency_increase_plus1
        };

        class TSDUCKDLL LongTermRef                         //!< An entry for lt_ref values
        {
        public:
            LongTermRef() = default;                        //!< Constructor
            uint32_t lt_ref_pic_poc_lsb_sps = 0;            //!< lt_ref_pic_poc_lsb_sps
            uint8_t  used_by_curr_pic_lt_sps_flag = 0;      //!< used_by_curr_pic_lt_sps_flag
        };

        // Sequence parameter set fields.
        // See ITU H.265, sections 7.3.2.2 and 7.4.3.2

        uint8_t  sps_video_parameter_set_id = 0;                        //!< sps_video_parameter_set_id
        uint8_t  sps_max_sub_layers_minus1 = 0;                         //!< sps_max_sub_layers_minus1
        uint8_t  sps_temporal_id_nesting_flag = 0;                      //!< sps_temporal_id_nesting_flag
        HEVCProfileTierLevel profile_tier_level {};                     //!< profile_tier_level
        uint32_t sps_seq_parameter_set_id = 0;                          //!< sps_seq_parameter_set_id
        uint32_t chroma_format_idc = 0;                                 //!< chroma_format_idc
        // if (chroma_format_idc == 3) {
            uint8_t separate_colour_plane_flag = 0;                     //!< separate_colour_plane_flag
        // }
        uint32_t pic_width_in_luma_samples = 0;                         //!< pic_width_in_luma_samples
        uint32_t pic_height_in_luma_samples = 0;                        //!< pic_height_in_luma_samples
        uint8_t  conformance_window_flag = 0;                           //!< conformance_window_flag
        // if (conformance_window_flag) {
            uint32_t conf_win_left_offset = 0;                          //!< conf_win_left_offset
            uint32_t conf_win_right_offset = 0;                         //!< conf_win_right_offset
            uint32_t conf_win_top_offset = 0;                           //!< conf_win_top_offset
            uint32_t conf_win_bottom_offset = 0;                        //!< conf_win_bottom_offset
        // }
        uint32_t bit_depth_luma_minus8 = 0;                             //!< bit_depth_luma_minus8
        uint32_t bit_depth_chroma_minus8 = 0;                           //!< bit_depth_chroma_minus8
        uint32_t log2_max_pic_order_cnt_lsb_minus4 = 0;                 //!< log2_max_pic_order_cnt_lsb_minus4
        uint8_t  sps_sub_layer_ordering_info_present_flag = 0;          //!< sps_sub_layer_ordering_info_present_flag
        // for (i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1); i <= sps_max_sub_layers_minus1; i++) {}
        std::vector<SPSMax> sps_max {};                                 //!< sps_max values
        uint32_t log2_min_luma_coding_block_size_minus3 = 0;            //!< log2_min_luma_coding_block_size_minus3
        uint32_t log2_diff_max_min_luma_coding_block_size = 0;          //!< log2_diff_max_min_luma_coding_block_size
        uint32_t log2_min_luma_transform_block_size_minus2 = 0;         //!< log2_min_luma_transform_block_size_minus2
        uint32_t log2_diff_max_min_luma_transform_block_size = 0;       //!< log2_diff_max_min_luma_transform_block_size
        uint32_t max_transform_hierarchy_depth_inter = 0;               //!< max_transform_hierarchy_depth_inter
        uint32_t max_transform_hierarchy_depth_intra = 0;               //!< max_transform_hierarchy_depth_intra
        uint8_t  scaling_list_enabled_flag = 0;                         //!< scaling_list_enabled_flag
        // if (scaling_list_enabled_flag) {
            uint8_t sps_scaling_list_data_present_flag = 0;             //!< sps_scaling_list_data_present_flag
            // if (sps_scaling_list_data_present_flag) {
                HEVCScalingListData scaling_list_data {};               //!< scaling_list_data
            // }
        // }
        uint8_t amp_enabled_flag = 0;                                   //!< amp_enabled_flag
        uint8_t sample_adaptive_offset_enabled_flag = 0;                //!< sample_adaptive_offset_enabled_flag
        uint8_t pcm_enabled_flag = 0;                                   //!< pcm_enabled_flag
        // if (pcm_enabled_flag) {
            uint8_t  pcm_sample_bit_depth_luma_minus1 = 0;              //!< pcm_sample_bit_depth_luma_minus1
            uint8_t  pcm_sample_bit_depth_chroma_minus1 = 0;            //!< pcm_sample_bit_depth_chroma_minus1
            uint32_t log2_min_pcm_luma_coding_block_size_minus3 = 0;    //!< log2_min_pcm_luma_coding_block_size_minus3
            uint32_t log2_diff_max_min_pcm_luma_coding_block_size = 0;  //!< log2_diff_max_min_pcm_luma_coding_block_size
            uint8_t  pcm_loop_filter_disabled_flag = 0;                 //!< pcm_loop_filter_disabled_flag
        // }
        uint32_t num_short_term_ref_pic_sets = 0;                       //!< num_short_term_ref_pic_sets
        // for (i = 0; i < num_short_term_ref_pic_sets; i++) {}
        HEVCShortTermReferencePictureSetList st_ref_pic_set {};         //!< st_ref_pic_set
        uint8_t long_term_ref_pics_present_flag = 0;                    //!< long_term_ref_pics_present_flag
        // if (long_term_ref_pics_present_flag) {
            uint32_t num_long_term_ref_pics_sps = 0;                    //!< num_long_term_ref_pics_sps
            // for (i = 0; i < num_long_term_ref_pics_sps; i++) {}
            std::vector<LongTermRef> lt_ref {};                         //!< lt_ref values
        // }
        uint8_t sps_temporal_mvp_enabled_flag = 0;                      //!< sps_temporal_mvp_enabled_flag
        uint8_t strong_intra_smoothing_enabled_flag = 0;                //!< strong_intra_smoothing_enabled_flag
        uint8_t vui_parameters_present_flag = 0;                        //!< vui_parameters_present_flag
        // if (vui_parameters_present_flag) {
            HEVCVUIParameters vui {};                                   //!< vui
        // }
        uint8_t sps_extension_present_flag = 0;                         //!< sps_extension_present_flag
        // if (sps_extension_present_flag) {
            uint8_t sps_range_extension_flag = 0;                       //!< sps_range_extension_flag
            uint8_t sps_multilayer_extension_flag = 0;                  //!< sps_multilayer_extension_flag
            uint8_t sps_3d_extension_flag = 0;                          //!< sps_3d_extension_flag
            uint8_t sps_scc_extension_flag = 0;                         //!< sps_scc_extension_flag
            uint8_t sps_extension_4bits = 0;                            //!< sps_extension_4bits
        // }
        //
        // Warning: we currently do not deserialize extensions.
        //
        // if (sps_range_extension_flag) {
            // sps_range_extension()
        // }
        // if (sps_multilayer_extension_flag) {
            // sps_multilayer_extension()
        // }
        // if (sps_3d_extension_flag) {
            // sps_3d_extension()
        // }
        // if (sps_scc_extension_flag) {
            // sps_scc_extension()
        // if (sps_extension_4bits) {
            // while (more_rbsp_data()) {
                // sps_extension_data_flag
            // }
        // }

    protected:
        // Inherited methods
        virtual bool parseBody(AVCParser&, std::initializer_list<uint32_t>) override;
    };
}
