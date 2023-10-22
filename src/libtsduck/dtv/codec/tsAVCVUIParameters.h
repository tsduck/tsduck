//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  AVC VUI (Video Usability Information) parameters.
//!  AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoStructure.h"
#include "tsAVCHRDParameters.h"

namespace ts {
    //!
    //! AVC VUI (Video Usability Information) parameters.
    //! @ingroup mpeg
    //!
    //! AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
    //!
    class TSDUCKDLL AVCVUIParameters: public AbstractVideoStructure
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractVideoStructure SuperClass;

        //!
        //! Constructor from a binary area.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //!
        AVCVUIParameters(const uint8_t* data = nullptr, size_t size = 0);

        // Inherited methods
        virtual void clear() override;
        virtual bool parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual bool parse(AVCParser& parser, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual std::ostream& display(std::ostream& strm = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

        // VUI parameters fields.
        // See ISO/IEC 14496-10 sections E.1.1 and E.2.1.

        uint8_t aspect_ratio_info_present_flag = 0;                //!< aspect_ratio_info_present_flag
        // if (aspect_ratio_info_present_flag) {
            uint8_t aspect_ratio_idc = 0;                          //!< aspect_ratio_idc
            // if (aspect_ratio_idc == 255) {                      // Extended_SAR
                uint16_t sar_width = 0;                            //!< sar_width
                uint16_t sar_height = 0;                           //!< sar_height
            // }
        // }
        uint8_t overscan_info_present_flag = 0;                    //!< overscan_info_present_flag
        // if (overscan_info_present_flag) {
            uint8_t overscan_appropriate_flag = 0;                 //!< overscan_appropriate_flag
        // }
        uint8_t video_signal_type_present_flag = 0;                //!< video_signal_type_present_flag
        // if (video_signal_type_present_flag) {
            uint8_t video_format = 0;                              //!< video_format
            uint8_t video_full_range_flag = 0;                     //!< video_full_range_flag
            uint8_t colour_description_present_flag = 0;           //!< colour_description_present_flag
            // if (colour_description_present_flag) {
                uint8_t colour_primaries = 0;                      //!< colour_primaries
                uint8_t transfer_characteristics = 0;              //!< transfer_characteristics
                uint8_t matrix_coefficients = 0;                   //!< matrix_coefficients
            // }
        // }
        uint8_t chroma_loc_info_present_flag = 0;                  //!< chroma_loc_info_present_flag
        // if (chroma_loc_info_present_flag) {
            uint32_t chroma_sample_loc_type_top_field = 0;         //!< chroma_sample_loc_type_top_field
            uint32_t chroma_sample_loc_type_bottom_field = 0;      //!< chroma_sample_loc_type_bottom_field
        // }
        uint8_t timing_info_present_flag = 0;                      //!< timing_info_present_flag
        // if (timing_info_present_flag) {
            uint32_t num_units_in_tick = 0;                        //!< num_units_in_tick
            uint32_t time_scale = 0;                               //!< time_scale
            uint8_t  fixed_frame_rate_flag = 0;                    //!< fixed_frame_rate_flag
        // }
        uint8_t nal_hrd_parameters_present_flag = 0;               //!< nal_hrd_parameters_present_flag
        // if (nal_hrd_parameters_present_flag) {
        AVCHRDParameters nal_hrd {};                               //!< nal_hrd
        // }
        uint8_t vcl_hrd_parameters_present_flag = 0;               //!< vcl_hrd_parameters_present_flag
        // if (vcl_hrd_parameters_present_flag) {
        AVCHRDParameters vcl_hrd {};                               //!< vcl_hrd
        // }
        // if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
            uint8_t low_delay_hrd_flag = 0;                        //!< low_delay_hrd_flag
        // }
        uint8_t pic_struct_present_flag = 0;                       //!< pic_struct_present_flag
        uint8_t bitstream_restriction_flag = 0;                    //!< bitstream_restriction_flag
        // if (bitstream_restriction_flag) {
            uint8_t  motion_vectors_over_pic_boundaries_flag = 0;  //!< motion_vectors_over_pic_boundaries_flag
            uint32_t max_bytes_per_pic_denom = 0;                  //!< max_bytes_per_pic_denom
            uint32_t max_bits_per_mb_denom = 0;                    //!< max_bits_per_mb_denom
            uint32_t log2_max_mv_length_horizontal = 0;            //!< log2_max_mv_length_horizontal
            uint32_t log2_max_mv_length_vertical = 0;              //!< log2_max_mv_length_vertical
            uint32_t num_reorder_frames = 0;                       //!< num_reorder_frames
            uint32_t max_dec_frame_buffering = 0;                  //!< max_dec_frame_buffering
        // }
    };
}
