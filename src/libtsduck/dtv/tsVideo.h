//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  @ingroup mpeg
//!  Basic definitions for various video coding standards.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Video macroblock width in pixels.
    //! Valid for:
    //! - ISO 11172-2 (MPEG-1 video)
    //! - ISO 13818-2 (MPEG-2 video)
    //! - ISO 14496-10 (MPEG-4 Advanced Video Coding, AVC, ITU H.264)
    //!
    constexpr size_t MACROBLOCK_WIDTH = 16;

    //!
    //! Video macroblock height in pixels.
    //! @see MACROBLOCK_WIDTH
    //!
    constexpr size_t MACROBLOCK_HEIGHT = 16;


    //---------------------------------------------------------------------
    //! Frame rate values (in MPEG-1/2 video sequence).
    //---------------------------------------------------------------------

    enum {
        FPS_23_976 = 0x01,  //!< 23.976 fps (24000/1001)
        FPS_24     = 0x02,  //!< 24 fps
        FPS_25     = 0x03,  //!< 25 fps
        FPS_29_97  = 0x04,  //!< 29.97 fps (30000/1001)
        FPS_30     = 0x05,  //!< 30 fps
        FPS_50     = 0x06,  //!< 50 fps
        FPS_59_94  = 0x07,  //!< 59.94 fps (60000/1001)
        FPS_60     = 0x08,  //!< 60 fps
    };


    //---------------------------------------------------------------------
    //! Aspect ratio values (in MPEG-1/2 video sequence header).
    //---------------------------------------------------------------------

    enum {
        AR_SQUARE = 1,  //!< 1/1 MPEG video aspect ratio.
        AR_4_3    = 2,  //!< 4/3 MPEG video aspect ratio.
        AR_16_9   = 3,  //!< 16/9 MPEG video aspect ratio.
        AR_221    = 4,  //!< 2.21/1 MPEG video aspect ratio.
    };


    //---------------------------------------------------------------------
    //! Chroma format values (in MPEG-1/2 video sequence header).
    //---------------------------------------------------------------------

    enum {
        CHROMA_MONO = 0,  //!< Monochrome MPEG video.
        CHROMA_420  = 1,  //!< Chroma 4:2:0 MPEG video.
        CHROMA_422  = 2,  //!< Chroma 4:2:2 MPEG video.
        CHROMA_444  = 3,  //!< Chroma 4:4:4 MPEG video.
    };


    //---------------------------------------------------------------------
    //! AVC access unit types
    //---------------------------------------------------------------------

    enum {
        AVC_AUT_NON_IDR      =  1, //!< Coded slice of a non-IDR picture (NALunit type).
        AVC_AUT_SLICE_A      =  2, //!< Coded slice data partition A (NALunit type).
        AVC_AUT_SLICE_B      =  3, //!< Coded slice data partition B (NALunit type).
        AVC_AUT_SLICE_C      =  4, //!< Coded slice data partition C (NALunit type).
        AVC_AUT_IDR          =  5, //!< Coded slice of an IDR picture (NALunit type).
        AVC_AUT_SEI          =  6, //!< Supplemental enhancement information (SEI) (NALunit type).
        AVC_AUT_SEQPARAMS    =  7, //!< Sequence parameter set (NALunit type).
        AVC_AUT_PICPARAMS    =  8, //!< Picture parameter set (NALunit type).
        AVC_AUT_DELIMITER    =  9, //!< Access unit delimiter (NALunit type).
        AVC_AUT_END_SEQUENCE = 10, //!< End of sequence (NALunit type).
        AVC_AUT_END_STREAM   = 11, //!< End of stream (NALunit type).
        AVC_AUT_FILLER       = 12, //!< Filler data (NALunit type).
        AVC_AUT_SEQPARAMSEXT = 13, //!< Sequence parameter set extension (NALunit type).
        AVC_AUT_PREFIX       = 14, //!< Prefix NAL unit in scalable extension (NALunit type).
        AVC_AUT_SUBSETPARAMS = 15, //!< Subset sequence parameter set (NALunit type).
        AVC_AUT_SLICE_NOPART = 19, //!< Coded slice without partitioning (NALunit type).
        AVC_AUT_SLICE_SCALE  = 20, //!< Coded slice in scalable extension (NALunit type).
    };


    //---------------------------------------------------------------------
    //! AVC SEI types
    //---------------------------------------------------------------------

    enum {
        AVC_SEI_BUF_PERIOD = 0,                 //!< SEI type: buffering_period
        AVC_SEI_PIC_TIMING = 1,                 //!< SEI type: pic_timing
        AVC_SEI_PAN_SCAN_RECT = 2,              //!< SEI type: pan_scan_rect
        AVC_SEI_FILLER_PAYLOAD = 3,             //!< SEI type: filler_payload
        AVC_SEI_USER_DATA_REG = 4,              //!< SEI type: user_data_registered_itu_t_t35
        AVC_SEI_USER_DATA_UNREG = 5,            //!< SEI type: user_data_unregistered
        AVC_SEI_RECOVERY_POINT = 6,             //!< SEI type: recovery_point
        AVC_SEI_DEC_REF_PIC_MAR_REP = 7,        //!< SEI type: dec_ref_pic_marking_repetition
        AVC_SEI_SPARE_PIC = 8,                  //!< SEI type: spare_pic
        AVC_SEI_SCENE_INFO = 9,                 //!< SEI type: scene_info
        AVC_SEI_SUB_SEQ_INFO = 10,              //!< SEI type: sub_seq_info
        AVC_SEI_SUB_SEQ_LAYER_CHARS = 11,       //!< SEI type: sub_seq_layer_characteristics
        AVC_SEI_SUB_SEQ_CHARS = 12,             //!< SEI type: sub_seq_characteristics
        AVC_SEI_FFRAME_FREEZE = 13,             //!< SEI type: full_frame_freeze
        AVC_SEI_FFRAME_FREEZE_RELEASE = 14,     //!< SEI type: full_frame_freeze_release
        AVC_SEI_FFRAME_SNAPSHOT = 15,           //!< SEI type: full_frame_snapshot
        AVC_SEI_PROG_REF_SEG_START = 16,        //!< SEI type: progressive_refinement_segment_start
        AVC_SEI_PROG_REF_SEG_END = 17,          //!< SEI type: progressive_refinement_segment_end
        AVC_SEI_MOTION_CSLICE_GROUP_SET = 18,   //!< SEI type: motion_constrained_slice_group_set
        AVC_SEI_FILM_GRAIN_CHARS = 19,          //!< SEI type: film_grain_characteristics
        AVC_SEI_DEBLOCK_FILTER_DISP_PREF = 20,  //!< SEI type: deblocking_filter_display_preference
        AVC_SEI_STEREO_VIDEO_INFO = 21,         //!< SEI type: stereo_video_info
        AVC_SEI_POST_FILTER_HINT = 22,          //!< SEI type: post_filter_hint
        AVC_SEI_TONE_MAPPING_INFO = 23,         //!< SEI type: tone_mapping_info
        AVC_SEI_SCALABILITY_INFO = 24,          //!< SEI type: scalability_info
        AVC_SEI_SUB_PIC_SCALABLE_LAYER = 25,    //!< SEI type: sub_pic_scalable_layer
        AVC_SEI_NON_REQUIRED_LAYER_REP = 26,    //!< SEI type: non_required_layer_rep
        AVC_SEI_PRIORITY_LAYER_INFO = 27,       //!< SEI type: priority_layer_info
        AVC_SEI_LAYERS_NOT_PRESENT = 28,        //!< SEI type: layers_not_present
        AVC_SEI_LAYER_DEP_CHANGE = 29,          //!< SEI type: layer_dependency_change
        AVC_SEI_SCALABLE_NESTING = 30,          //!< SEI type: scalable_nesting
        AVC_SEI_BASE_LAYER_TEMPORAL_HRD = 31,   //!< SEI type: base_layer_temporal_hrd
        AVC_SEI_QUALITY_LAYER_INTEG_CHECK = 32, //!< SEI type: quality_layer_integrity_check
        AVC_SEI_REDUNDANT_PIC_PROPERTY = 33,    //!< SEI type: redundant_pic_property
        AVC_SEI_TL0_PICTURE_INDEX = 34,         //!< SEI type: tl0_picture_index
        AVC_SEI_TL_SWITCHING_POINT = 35,        //!< SEI type: tl_switching_point
    };

    //! Size in bytes of a UUID in AVC SEI's.
    constexpr size_t AVC_SEI_UUID_SIZE = 16;
}
