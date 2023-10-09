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
//!  Basic definitions for High Efficiency Video Coding (HEVC, H.265) standard.
//!  @see ITU-T Rec. H.265
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! HEVC access unit types
    //! @see H.265, 7.4.2.2
    //!
    enum {
        HEVC_AUT_TRAIL_N        =  0,   //!< Coded slice segment of a non-TSA, non-STSA trailing picture.
        HEVC_AUT_TRAIL_R        =  1,   //!< Coded slice segment of a non-TSA, non-STSA trailing picture.
        HEVC_AUT_TSA_N          =  2,   //!< Coded slice segment of a TSA picture.
        HEVC_AUT_TSA_R          =  3,   //!< Coded slice segment of a TSA picture.
        HEVC_AUT_STSA_N         =  4,   //!< Coded slice segment of an STSA picture.
        HEVC_AUT_STSA_R         =  5,   //!< Coded slice segment of an STSA picture.
        HEVC_AUT_RADL_N         =  6,   //!< Coded slice segment of a RADL picture.
        HEVC_AUT_RADL_R         =  7,   //!< Coded slice segment of a RADL picture.
        HEVC_AUT_RASL_N         =  8,   //!< Coded slice segment of a RASL picture.
        HEVC_AUT_RASL_R         =  9,   //!< Coded slice segment of a RASL picture.
        HEVC_AUT_RSV_VCL_N10    = 10,   //!< Reserved non-IRAP SLNR VCL NAL unit types.
        HEVC_AUT_RSV_VCL_R11    = 11,   //!< Reserved non-IRAP sub-layer reference VCL NAL unit types.
        HEVC_AUT_RSV_VCL_N12    = 12,   //!< Reserved non-IRAP SLNR VCL NAL unit types.
        HEVC_AUT_RSV_VCL_R13    = 13,   //!< Reserved non-IRAP sub-layer reference VCL NAL unit types.
        HEVC_AUT_RSV_VCL_N14    = 14,   //!< Reserved non-IRAP SLNR VCL NAL unit types.
        HEVC_AUT_RSV_VCL_R15    = 15,   //!< Reserved non-IRAP sub-layer reference VCL NAL unit types.
        HEVC_AUT_BLA_W_LP       = 16,   //!< Coded slice segment of a BLA picture.
        HEVC_AUT_BLA_W_RADL     = 17,   //!< Coded slice segment of a BLA picture.
        HEVC_AUT_BLA_N_LP       = 18,   //!< Coded slice segment of a BLA picture.
        HEVC_AUT_IDR_W_RADL     = 19,   //!< Coded slice segment of an IDR picture.
        HEVC_AUT_IDR_N_LP       = 20,   //!< Coded slice segment of an IDR picture.
        HEVC_AUT_CRA_NUT        = 21,   //!< Coded slice segment of a CRA picture.
        HEVC_AUT_RSV_IRAP_VCL22 = 22,   //!< Reserved IRAP VCL NAL unit types.
        HEVC_AUT_RSV_IRAP_VCL23 = 23,   //!< Reserved IRAP VCL NAL unit types.
        HEVC_AUT_RSV_VCL24      = 24,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        HEVC_AUT_RSV_VCL25      = 25,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        HEVC_AUT_RSV_VCL26      = 26,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        HEVC_AUT_RSV_VCL27      = 27,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        HEVC_AUT_RSV_VCL28      = 28,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        HEVC_AUT_RSV_VCL29      = 29,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        HEVC_AUT_RSV_VCL30      = 30,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        HEVC_AUT_RSV_VCL31      = 31,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        HEVC_AUT_VPS_NUT        = 32,   //!< Video parameter set.
        HEVC_AUT_SPS_NUT        = 33,   //!< Sequence parameter set.
        HEVC_AUT_PPS_NUT        = 34,   //!< Picture parameter set.
        HEVC_AUT_AUD_NUT        = 35,   //!< Access unit delimiter.
        HEVC_AUT_EOS_NUT        = 36,   //!< End of sequence.
        HEVC_AUT_EOB_NUT        = 37,   //!< End of bitstream.
        HEVC_AUT_FD_NUT         = 38,   //!< Filler data.
        HEVC_AUT_PREFIX_SEI_NUT = 39,   //!< Supplemental enhancement information.
        HEVC_AUT_SUFFIX_SEI_NUT = 40,   //!< Supplemental enhancement information.
        HEVC_AUT_RSV_NVCL41     = 41,   //!< Reserved.
        HEVC_AUT_RSV_NVCL42     = 42,   //!< Reserved.
        HEVC_AUT_RSV_NVCL43     = 43,   //!< Reserved.
        HEVC_AUT_RSV_NVCL44     = 44,   //!< Reserved.
        HEVC_AUT_RSV_NVCL45     = 45,   //!< Reserved.
        HEVC_AUT_RSV_NVCL46     = 46,   //!< Reserved.
        HEVC_AUT_RSV_NVCL47     = 47,   //!< Reserved.
        HEVC_AUT_UNSPEC48       = 48,   //!< Unspecified.
        HEVC_AUT_UNSPEC63       = 63,   //!< Unspecified.
        HEVC_AUT_INVALID        = 0xFF, //!< Invalid value, can be used as error indicator.
    };

    //!
    //! HEVC picture types
    //! @see H.265, 7.4.3.5
    //!
    enum {
        HEVC_PIC_TYPE_I   = 0,  //!< HEVC picture with slice types I
        HEVC_PIC_TYPE_IP  = 1,  //!< HEVC picture with slice types I or P
        HEVC_PIC_TYPE_IPB = 2,  //!< HEVC picture with slice types I, P or B
    };
}
