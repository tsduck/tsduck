//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck mpeg
//!  Registration id in MPEG-defined registration_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"

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
    //! Statically build a 32-bit registration id from a 4-character string.
    //! @param [in] id A 4-character string.
    //! @return The corresponding 32-bit registration id.
    //!
    constexpr REGID MakeREGID(const char id[5])
    {
        return ((id[0] << 24) + (id[1] << 16) + (id[2] << 8) + id[3]);
    };

    //!
    //! Registration id (a.k.a "format identifier") values in MPEG-defined registration_descriptor.
    //!
    //! Should be found in the list maintained by the SMPTE Registration Authority, LLC
    //! https://www.smpte-ra.org/registered-mpeg-ts-ids
    //!
    enum : REGID {
        REGID_AC3      = MakeREGID("AC-3"),  //!< Advanced Television Systems Committee.
        REGID_AOM      = MakeREGID("AV01"),  //!< Alliance for Open Media.
        REGID_AVSAudio = MakeREGID("AVSA"),  //!< Audio Video Coding Standard Working Group of China.
        REGID_AVSVideo = MakeREGID("AVSV"),  //!< Audio Video Coding Standard Working Group of China.
        REGID_BSSD     = MakeREGID("BSSD"),  //!< Society of Motion Picture and Television Engineers
        REGID_CUEI     = MakeREGID("CUEI"),  //!< Society of Cable Telecommunications Engineers (SCTE-35 splice information).
        REGID_DTG1     = MakeREGID("DTG1"),  //!< Digital TV Group.
        REGID_EAC3     = MakeREGID("EAC3"),  //!< Dolby Laboratories, Inc.
        REGID_GA94     = MakeREGID("GA94"),  //!< Advanced Television Systems Committee (ATSC).
        REGID_HDMV     = MakeREGID("HDMV"),  //!< Sony Corporation (BluRay disks).
        REGID_HEVC     = MakeREGID("HEVC"),  //!< "HEVC" registration identifier (unofficial, used in legacy streams).
        REGID_KLVA     = MakeREGID("KLVA"),  //!< Society of Motion Picture and Television Engineers.
        REGID_SCTE     = MakeREGID("SCTE"),  //!< Society of Cable Telecommunications Engineers.
        REGID_VANC     = MakeREGID("VANC"),  //!< SMPTE ST 2038 Carriage of Ancillary Data Packets (unofficial)
        REGID_VC1      = MakeREGID("VC-1"),  //!< VC-1 video coding (SMPTE 421).
        REGID_VC4      = MakeREGID("VC-4"),  //!< VC-4 video coding (SMPTE 2058).
        REGID_CUVV     = MakeREGID("cuvv"),  //!< UHD World Association ("cuvv") registration identifier (registration procedure in progress).
        REGID_NULL     = 0xFFFFFFFF,         //!< Unassigned registration identifier.
    };

    //!
    //! Name of a Registration id from an MPEG registration_descriptor.
    //! @param [in] regid Registration id.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString REGIDName(REGID regid, NamesFlags flags = NamesFlags::NAME);
}
