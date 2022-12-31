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
//!  @ingroup mpeg
//!  Basic definitions for Versatile Video Coding (VVC, H.266) standard.
//!  @see ITU-T Rec. H.266
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! VVC access unit types
    //! @see H.266, 7.4.2.2
    //!
    enum {
        VVC_AUT_TRAIL_NUT      =  0,   //!< Coded slice of a trailing picture or subpicture.
        VVC_AUT_STSA_NUT       =  1,   //!< Coded slice of an STSA picture or subpicture.
        VVC_AUT_RADL_NUT       =  2,   //!< Coded slice of a RADL picture or subpicture.
        VVC_AUT_RASL_NUT       =  3,   //!< Coded slice of a RASL picture or subpicture.
        VVC_AUT_RSV_VCL_4      =  4,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        VVC_AUT_RSV_VCL_5      =  5,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        VVC_AUT_RSV_VCL_6      =  6,   //!< Reserved non-IRAP VCL NAL unit types VCL.
        VVC_AUT_IDR_W_RADL     =  7,   //!< Coded slice of an IDR picture or subpicture.
        VVC_AUT_IDR_N_LP       =  8,   //!< Coded slice of an IDR picture or subpicture.
        VVC_AUT_CRA_NUT        =  9,   //!< Coded slice of a CRA picture or subpicture.
        VVC_AUT_GDR_NUT        = 10,   //!< Coded slice of a GDR picture or subpicture.
        VVC_AUT_RSV_IRAP_11    = 11,   //!< Reserved IRAP VCL NAL unit type VCL.
        VVC_AUT_OPI_NUT        = 12,   //!< Operating point information.
        VVC_AUT_DCI_NUT        = 13,   //!< Decoding capability information.
        VVC_AUT_VPS_NUT        = 14,   //!< Video parameter set.
        VVC_AUT_SPS_NUT        = 15,   //!< Sequence parameter set.
        VVC_AUT_PPS_NUT        = 16,   //!< Picture parameter set.
        VVC_AUT_PREFIX_APS_NUT = 17,   //!< Adaptation parameter set.
        VVC_AUT_SUFFIX_APS_NUT = 18,   //!< Adaptation parameter set.
        VVC_AUT_PH_NUT         = 19,   //!< Picture header.
        VVC_AUT_AUD_NUT        = 20,   //!< Access Unit delimiter.
        VVC_AUT_EOS_NUT        = 21,   //!< End of sequence.
        VVC_AUT_EOB_NUT        = 22,   //!< End of bitstream.
        VVC_AUT_PREFIX_SEI_NUT = 23,   //!< Supplemental enhancement information.
        VVC_AUT_SUFFIX_SEI_NUT = 24,   //!< Supplemental enhancement information.
        VVC_AUT_FD_NUT         = 25,   //!< Filler data.
        VVC_AUT_RSV_NVCL_26    = 26,   //!< Reserved non-VCL NAL unit types non-VCL.
        VVC_AUT_RSV_NVCL_27    = 27,   //!< Reserved non-VCL NAL unit types non-VCL.
        VVC_AUT_UNSPEC_28      = 28,   //!< Unspecified non-VCL NAL unit types.
        VVC_AUT_UNSPEC_31      = 31,   //!< Unspecified non-VCL NAL unit types.
        VVC_AUT_INVALID        = 0xFF, //!< Invalid value, can be used as error indicator.
    };

    //!
    //! VVC picture types
    //! @see H.266, 7.4.3.10
    //!
    enum {
        VVC_PIC_TYPE_I   = 0,  //!< VVC picture with slice types I
        VVC_PIC_TYPE_IP  = 1,  //!< VVC picture with slice types I or P
        VVC_PIC_TYPE_IPB = 2,  //!< VVC picture with slice types I, P or B
    };
}
