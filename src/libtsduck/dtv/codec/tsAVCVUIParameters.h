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

        uint8_t aspect_ratio_info_present_flag;                //!< aspect_ratio_info_present_flag
        // if (aspect_ratio_info_present_flag) {
            uint8_t aspect_ratio_idc;                          //!< aspect_ratio_idc
            // if (aspect_ratio_idc == 255) {                  // Extended_SAR
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
        uint8_t timing_info_present_flag;                      //!< timing_info_present_flag
        // if (timing_info_present_flag) {
            uint32_t num_units_in_tick;                        //!< num_units_in_tick
            uint32_t time_scale;                               //!< time_scale
            uint8_t  fixed_frame_rate_flag;                    //!< fixed_frame_rate_flag
        // }
        uint8_t nal_hrd_parameters_present_flag;               //!< nal_hrd_parameters_present_flag
        // if (nal_hrd_parameters_present_flag) {
            AVCHRDParameters nal_hrd;                          //!< nal_hrd
        // }
        uint8_t vcl_hrd_parameters_present_flag;               //!< vcl_hrd_parameters_present_flag
        // if (vcl_hrd_parameters_present_flag) {
            AVCHRDParameters vcl_hrd;                          //!< vcl_hrd
        // }
        // if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
            uint8_t low_delay_hrd_flag;                        //!< low_delay_hrd_flag
        // }
        uint8_t pic_struct_present_flag;                       //!< pic_struct_present_flag
        uint8_t bitstream_restriction_flag;                    //!< bitstream_restriction_flag
        // if (bitstream_restriction_flag) {
            uint8_t  motion_vectors_over_pic_boundaries_flag;  //!< motion_vectors_over_pic_boundaries_flag
            uint32_t max_bytes_per_pic_denom;                  //!< max_bytes_per_pic_denom
            uint32_t max_bits_per_mb_denom;                    //!< max_bits_per_mb_denom
            uint32_t log2_max_mv_length_horizontal;            //!< log2_max_mv_length_horizontal
            uint32_t log2_max_mv_length_vertical;              //!< log2_max_mv_length_vertical
            uint32_t num_reorder_frames;                       //!< num_reorder_frames
            uint32_t max_dec_frame_buffering;                  //!< max_dec_frame_buffering
        // }
    };
}
