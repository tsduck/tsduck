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
#include "tsNamesFile.h"

namespace ts {
    //!
    //! Registration id, as found in an MPEG registration descriptor.
    //! This kind of value is also named "format identifier".
    //!
    using REGID = uint32_t;

    //!
    //! Vector of registration ids.
    //!
    //! Registration ids have ambiguous usage rules. Sometimes, it seems that all registration ids from
    //! a descriptor list must be simultaneously used (to identify stream types in a PMT for instance).
    //! Some other times, only one registration id must be valid at a time (to identify an MPEG private
    //! descriptor for instance).
    //!
    //! TSDuck allows the two strategies, on a case by case basis. Inside a descriptor list, all REGID's
    //! are collected in their order of appearance in a vector. In use cases where several REGID's are
    //! simulatenously used, the application searches for them in the vector. In use cases where two
    //! REGID's can be contradictory, the application uses the one which appeared last in the vector.
    //!
    //! Duplications are possible if the same REGID is defined several times in a descriptor list.
    //!
    using REGIDVector = std::vector<REGID>;

    //!
    //! Registration id (a.k.a "format identifier") values in MPEG-defined registration_descriptor.
    //!
    enum : REGID {
        REGID_AC3      = 0x41432D33,  //!< "AC-3" registration identifier.
        REGID_AOM      = 0x41563031,  //!< "AV01", Alliance for Open Media
        REGID_AVSAudio = 0x41565341,  //!< "AVSA", Audio Video Coding Standard Working Group of China
        REGID_AVSVideo = 0x41565356,  //!< "AVSV", Audio Video Coding Standard Working Group of China
        REGID_CUEI     = 0x43554549,  //!< "CUEI" registration identifier (SCTE-35 splice information).
        REGID_DTG1     = 0x44544731,  //!< "DTG1" registration identifier.
        REGID_EAC3     = 0x45414333,  //!< "EAC3" registration identifier.
        REGID_GA94     = 0x47413934,  //!< "GA94" registration identifier (ATSC).
        REGID_HDMV     = 0x48444D56,  //!< "HDMV" registration identifier (BluRay disks).
        REGID_HEVC     = 0x48455643,  //!< "HEVC" registration identifier.
        REGID_KLVA     = 0x4B4C5641,  //!< "KLVA" registration identifier.
        REGID_SCTE     = 0x53435445,  //!< "SCTE" registration identifier.
        REGID_AVSA     = 0x4A565341,  //!< AVS "AVSA" registration identifier.
        REGID_AVSV     = 0x4A565356,  //!< AVS "AVSV" registration identifier.
        REGID_CUVV     = 0x63757676,  //!< UHD World Association ("cuvv") registration identifier (registration procedure in progress).
        REGID_NULL     = 0xFFFFFFFF,  //!< Unassigned registration identifier.
    };

    //!
    //! Name of a Registration id from an MPEG registration_descriptor.
    //! @param [in] regid Registration id.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString REGIDName(REGID regid, NamesFlags flags = NamesFlags::NAME);
}
