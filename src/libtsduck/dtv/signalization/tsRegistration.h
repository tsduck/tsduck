//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup mpeg
//!  Registration id in MPEG-defined registration_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Registration id..
    //!
    using REGID = uint32_t;

    //!
    //! Registration id (a.k.a "format identifier") values in MPEG-defined registration_descriptor.
    //!
    enum : REGID {
        REGID_AC3  = 0x41432D33, //!< "AC-3" registration identifier.
        REGID_CUEI = 0x43554549, //!< "CUEI" registration identifier (SCTE-35 splice information).
        REGID_DTG1 = 0x44544731, //!< "DTG1" registration identifier.
        REGID_EAC3 = 0x45414333, //!< "EAC3" registration identifier.
        REGID_GA94 = 0x47413934, //!< "GA94" registration identifier (ATSC).
        REGID_HDMV = 0x48444D56, //!< "HDMV" registration identifier (BluRay disks).
        REGID_HEVC = 0x48455643, //!< "HEVC" registration identifier.
        REGID_KLVA = 0x4B4C5641, //!< "KLVA" registration identifier.
        REGID_SCTE = 0x53435445, //!< "SCTE" registration identifier.
        REGID_AVSA = 0x4A565341, //!< AVS "AVSA" registration identifier.
        REGID_AVSV = 0x4A565356, //!< AVS "AVSV" registration identifier.
        REGID_NULL = 0xFFFFFFFF, //!< Unassigned registration identifier.
    };
}
