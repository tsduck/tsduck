//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup mpeg
//!  Basic definitions for Advanced Video Coding (AVC, H.264) standard.
//!  @see ISO/IEC 14496-10, ITU-T Rec. H.264
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! AVC access unit types
    //! @see ISO/IEC 14496-10, H.264, 7.4.1
    //!
    enum {
        AVC_AUT_UNSPECIFIED  =  0,   //!< Unspecified NALunit type.
        AVC_AUT_NON_IDR      =  1,   //!< Coded slice of a non-IDR picture (NALunit type).
        AVC_AUT_SLICE_A      =  2,   //!< Coded slice data partition A (NALunit type).
        AVC_AUT_SLICE_B      =  3,   //!< Coded slice data partition B (NALunit type).
        AVC_AUT_SLICE_C      =  4,   //!< Coded slice data partition C (NALunit type).
        AVC_AUT_IDR          =  5,   //!< Coded slice of an IDR picture (NALunit type).
        AVC_AUT_SEI          =  6,   //!< Supplemental enhancement information (SEI) (NALunit type).
        AVC_AUT_SEQPARAMS    =  7,   //!< Sequence parameter set (NALunit type).
        AVC_AUT_PICPARAMS    =  8,   //!< Picture parameter set (NALunit type).
        AVC_AUT_DELIMITER    =  9,   //!< Access unit delimiter (NALunit type).
        AVC_AUT_END_SEQUENCE = 10,   //!< End of sequence (NALunit type).
        AVC_AUT_END_STREAM   = 11,   //!< End of stream (NALunit type).
        AVC_AUT_FILLER       = 12,   //!< Filler data (NALunit type).
        AVC_AUT_SEQPARAMSEXT = 13,   //!< Sequence parameter set extension (NALunit type).
        AVC_AUT_PREFIX       = 14,   //!< Prefix NAL unit in scalable extension (NALunit type).
        AVC_AUT_SUBSETPARAMS = 15,   //!< Subset sequence parameter set (NALunit type).
        AVC_AUT_DEPTHPARAMS  = 16,   //!< Depth parameter set (NALunit type).
        AVC_AUT_SLICE_NOPART = 19,   //!< Coded slice without partitioning (NALunit type).
        AVC_AUT_SLICE_SCALE  = 20,   //!< Coded slice in scalable extension (NALunit type).
        AVC_AUT_SLICE_EXTEND = 21,   //!< Coded slice extension or 3D-AVC texture view (NALunit type).
        AVC_AUT_INVALID      = 0xFF, //!< Invalid value, can be used as error indicator.
    };

    //!
    //! AVC SEI types
    //! @see ISO/IEC 14496-10, H.264, D.1.1
    //!
    enum {
        AVC_SEI_BUF_PERIOD                =   0,  //!< SEI type: buffering_period
        AVC_SEI_PIC_TIMING                =   1,  //!< SEI type: pic_timing
        AVC_SEI_PAN_SCAN_RECT             =   2,  //!< SEI type: pan_scan_rect
        AVC_SEI_FILLER_PAYLOAD            =   3,  //!< SEI type: filler_payload
        AVC_SEI_USER_DATA_REG             =   4,  //!< SEI type: user_data_registered_itu_t_t35
        AVC_SEI_USER_DATA_UNREG           =   5,  //!< SEI type: user_data_unregistered
        AVC_SEI_RECOVERY_POINT            =   6,  //!< SEI type: recovery_point
        AVC_SEI_DEC_REF_PIC_MAR_REP       =   7,  //!< SEI type: dec_ref_pic_marking_repetition
        AVC_SEI_SPARE_PIC                 =   8,  //!< SEI type: spare_pic
        AVC_SEI_SCENE_INFO                =   9,  //!< SEI type: scene_info
        AVC_SEI_SUB_SEQ_INFO              =  10,  //!< SEI type: sub_seq_info
        AVC_SEI_SUB_SEQ_LAYER_CHARS       =  11,  //!< SEI type: sub_seq_layer_characteristics
        AVC_SEI_SUB_SEQ_CHARS             =  12,  //!< SEI type: sub_seq_characteristics
        AVC_SEI_FFRAME_FREEZE             =  13,  //!< SEI type: full_frame_freeze
        AVC_SEI_FFRAME_FREEZE_RELEASE     =  14,  //!< SEI type: full_frame_freeze_release
        AVC_SEI_FFRAME_SNAPSHOT           =  15,  //!< SEI type: full_frame_snapshot
        AVC_SEI_PROG_REF_SEG_START        =  16,  //!< SEI type: progressive_refinement_segment_start
        AVC_SEI_PROG_REF_SEG_END          =  17,  //!< SEI type: progressive_refinement_segment_end
        AVC_SEI_MOTION_CSLICE_GROUP_SET   =  18,  //!< SEI type: motion_constrained_slice_group_set
        AVC_SEI_FILM_GRAIN_CHARS          =  19,  //!< SEI type: film_grain_characteristics
        AVC_SEI_DEBLOCK_FILTER_DISP_PREF  =  20,  //!< SEI type: deblocking_filter_display_preference
        AVC_SEI_STEREO_VIDEO_INFO         =  21,  //!< SEI type: stereo_video_info
        AVC_SEI_POST_FILTER_HINT          =  22,  //!< SEI type: post_filter_hint
        AVC_SEI_TONE_MAPPING_INFO         =  23,  //!< SEI type: tone_mapping_info
        AVC_SEI_SCALABILITY_INFO          =  24,  //!< SEI type: scalability_info
        AVC_SEI_SUB_PIC_SCALABLE_LAYER    =  25,  //!< SEI type: sub_pic_scalable_layer
        AVC_SEI_NON_REQUIRED_LAYER_REP    =  26,  //!< SEI type: non_required_layer_rep
        AVC_SEI_PRIORITY_LAYER_INFO       =  27,  //!< SEI type: priority_layer_info
        AVC_SEI_LAYERS_NOT_PRESENT        =  28,  //!< SEI type: layers_not_present
        AVC_SEI_LAYER_DEP_CHANGE          =  29,  //!< SEI type: layer_dependency_change
        AVC_SEI_SCALABLE_NESTING          =  30,  //!< SEI type: scalable_nesting
        AVC_SEI_BASE_LAYER_TEMPORAL_HRD   =  31,  //!< SEI type: base_layer_temporal_hrd
        AVC_SEI_QUALITY_LAYER_INTEG_CHECK =  32,  //!< SEI type: quality_layer_integrity_check
        AVC_SEI_REDUNDANT_PIC_PROPERTY    =  33,  //!< SEI type: redundant_pic_property
        AVC_SEI_TL0_PICTURE_INDEX         =  34,  //!< SEI type: tl0_picture_index
        AVC_SEI_TL_SWITCHING_POINT        =  35,  //!< SEI type: tl_switching_point
        AVC_SEI_PARALLEL_DECODING_INFO    =  36,  //!< SEI type: parallel_decoding_info
        AVC_SEI_MVC_SCALABLE_NESTING      =  37,  //!< SEI type: mvc_scalable_nesting
        AVC_SEI_VIEW_SCALABILITY_INFO     =  38,  //!< SEI type: view_scalability_info
        AVC_SEI_MULTIVIEW_SCENE_INFO      =  39,  //!< SEI type: multiview_scene_info
        AVC_SEI_MULTIVIEW_ACQUISITION     =  40,  //!< SEI type: multiview_acquisition_info
        AVC_SEI_NON_REQUIRED_VIEW_COMP    =  41,  //!< SEI type: non_required_view_component
        AVC_SEI_VIEW_DEPENDENCY_CHANGE    =  42,  //!< SEI type: view_dependency_change
        AVC_SEI_OP_POINTS_NOT_PRESENT     =  43,  //!< SEI type: operation_points_not_present
        AVC_SEI_BASE_VIEW_TEMPORAL_HRD    =  44,  //!< SEI type: base_view_temporal_hrd
        AVC_SEI_FRAME_PACKING_ARRANG      =  45,  //!< SEI type: frame_packing_arrangement
        AVC_SEI_MULTIVIEW_VIEW_POSITION   =  46,  //!< SEI type: multiview_view_position
        AVC_SEI_DISPLAY_ORIENTATION       =  47,  //!< SEI type: display_orientation
        AVC_SEI_MVCD_SCALABLE_NESTING     =  48,  //!< SEI type: mvcd_scalable_nesting
        AVC_SEI_MVCD_VIEW_SCALABILITY     =  49,  //!< SEI type: mvcd_view_scalability_info
        AVC_SEI_DEPTH_REPRESENTATION      =  50,  //!< SEI type: depth_representation_info
        AVC_SEI_3D_REF_DISPLAYS_INFO      =  51,  //!< SEI type: three_dimensional_reference_displays_info
        AVC_SEI_DEPTH_TIMING              =  52,  //!< SEI type: depth_timing
        AVC_SEI_DEPTH_SAMPLING_INFO       =  53,  //!< SEI type: depth_sampling_info
        AVC_SEI_CONSTR_DEPTH_PARAMSET_ID  =  54,  //!< SEI type: constrained_depth_parameter_set_identifier
        AVC_SEI_GREEN_METADATA            =  56,  //!< SEI type: green_metadata
        AVC_SEI_MASTER_DISP_COLOUR_VOLUME = 137,  //!< SEI type: mastering_display_colour_volume
        AVC_SEI_COLOUR_REMAPPING_INFO     = 142,  //!< SEI type: colour_remapping_info
        AVC_SEI_CONTENT_LIGHT_LEVEL_INFO  = 144,  //!< SEI type: content_light_level_info
        AVC_SEI_ALT_TRANSFER_CHARS        = 147,  //!< SEI type: alternative_transfer_characteristics
        AVC_SEI_AMBIENT_VIEWING_ENV       = 148,  //!< SEI type: ambient_viewing_environment
        AVC_SEI_CONTENT_COLOUR_VOLUME     = 149,  //!< SEI type: content_colour_volume
        AVC_SEI_EQUIRECTANGULAR_PROJECT   = 150,  //!< SEI type: equirectangular_projection
        AVC_SEI_CUBEMAP_PROJECTION        = 151,  //!< SEI type: cubemap_projection
        AVC_SEI_SPHERE_ROTATION           = 154,  //!< SEI type: sphere_rotation
        AVC_SEI_REGIONWISE_PACKING        = 155,  //!< SEI type: regionwise_packing
        AVC_SEI_OMNI_VIEWPORT             = 156,  //!< SEI type: omni_viewport
        AVC_SEI_ALTERNATIVE_DEPTH_INFO    = 181,  //!< SEI type: alternative_depth_info
        AVC_SEI_MANIFEST                  = 200,  //!< SEI type: manifest
        AVC_SEI_PREFIX_INDICATION         = 201,  //!< SEI type: prefix_indication
    };

    //!
    //! Size in bytes of a UUID in AVC SEI's.
    //!
    constexpr size_t AVC_SEI_UUID_SIZE = 16;

    //!
    //! AVC primary picture types
    //! @see ISO/IEC 14496-10, H.264, 7.4.2.4
    //!
    enum {
        AVC_PIC_TYPE_I      = 0,  //!< AVC picture with slice types 2, 7
        AVC_PIC_TYPE_IP     = 1,  //!< AVC picture with slice types 0, 2, 5, 7
        AVC_PIC_TYPE_IPB    = 2,  //!< AVC picture with slice types 0, 1, 2, 5, 6, 7
        AVC_PIC_TYPE_SI     = 3,  //!< AVC picture with slice types 4, 9
        AVC_PIC_TYPE_SIP    = 4,  //!< AVC picture with slice types 3, 4, 8, 9
        AVC_PIC_TYPE_I_SI   = 5,  //!< AVC picture with slice types 2, 4, 7, 9
        AVC_PIC_TYPE_IP_SIP = 6,  //!< AVC picture with slice types 0, 2, 3, 4, 5, 7, 8, 9
        AVC_PIC_TYPE_ANY    = 7,  //!< AVC picture with slice types 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    };

    //!
    //! AVC slice types
    //! @see ISO/IEC 14496-10, H.264, 7.4.3
    //!
    enum {
        AVC_SLICE_P      = 0,  //!< AVC P slice.
        AVC_SLICE_B      = 1,  //!< AVC B slice.
        AVC_SLICE_I      = 2,  //!< AVC I slice.
        AVC_SLICE_SP     = 3,  //!< AVC SP slice.
        AVC_SLICE_SI     = 4,  //!< AVC SI slice.
        AVC_SLICE_ALL_P  = 5,  //!< AVC all P slices.
        AVC_SLICE_ALL_B  = 6,  //!< AVC all B slices.
        AVC_SLICE_ALL_I  = 7,  //!< AVC all I slices.
        AVC_SLICE_ALL_SP = 8,  //!< AVC all SP slices.
        AVC_SLICE_ALL_SI = 9,  //!< AVC all SI slices.
    };
}
