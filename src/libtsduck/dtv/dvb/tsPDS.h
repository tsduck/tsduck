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
//!  DVB-registered private data specifier (PDS) values.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"

namespace ts {
    //!
    //! Private data specifier.
    //!
    using PDS = uint32_t;

    //!
    //! Selected DVB-registered private data specifier (PDS) values.
    //!
    enum : PDS {
        PDS_ASTRA     = 0x00000001,  //!< Private data specifier for SES Astra.
        PDS_BSKYB     = 0x00000002,  //!< Private data specifier for BskyB (1).
        PDS_BSKYB_2   = 0x00000003,  //!< Private data specifier for BskyB (2).
        PDS_BSKYB_3   = 0x00000004,  //!< Private data specifier for BskyB (3).
        PDS_NAGRA     = 0x00000009,  //!< Private data specifier for Nagra (1).
        PDS_NAGRA_2   = 0x0000000A,  //!< Private data specifier for Nagra (2).
        PDS_NAGRA_3   = 0x0000000B,  //!< Private data specifier for Nagra (3).
        PDS_NAGRA_4   = 0x0000000C,  //!< Private data specifier for Nagra (4).
        PDS_NAGRA_5   = 0x0000000D,  //!< Private data specifier for Nagra (5).
        PDS_TPS       = 0x00000010,  //!< Private data specifier for TPS.
        PDS_EACEM     = 0x00000028,  //!< Private data specifier for EACEM / EICTA.
        PDS_EICTA     = PDS_EACEM,   //!< Private data specifier for EACEM / EICTA.
        PDS_NORDIG    = 0x00000029,  //!< Private data specifier for NorDig (Northern Europe and Ireland).
        PDS_LOGIWAYS  = 0x000000A2,  //!< Private data specifier for Logiways.
        PDS_CANALPLUS = 0x000000C0,  //!< Private data specifier for Canal+.
        PDS_EUTELSAT  = 0x0000055F,  //!< Private data specifier for EutelSat.
        PDS_OFCOM     = 0x0000233A,  //!< Private data specifier for DTT UK (OFCOM, formerly ITC).
        PDS_AUSTRALIA = 0x00003200,  //!< Private data specifier for Free TV Australia.
        PDS_AOM       = 0x414F4D53,  //!< Private data specifier for the Alliance for Open Media (AOM) (value is "AOMS" in ASCII).
        PDS_AVSAudio  = 0x41565341,  //!< Private data specifier for AVS Working Group of China (value is "AVSA" in ASCII).
        PDS_AVSVideo  = 0x41565356,  //!< Private data specifier for AVS Working Group of China (value is "AVSV" in ASCII).
        PDS_NULL      = 0xFFFFFFFF,  //!< An invalid private data specifier, can be used as placeholder.
    };

    //!
    //! Enumeration description of PDS values.
    //! Typically used to implement PDS-related command line options.
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& PrivateDataSpecifierEnum();

    //!
    //! Name of a Private Data Specifier.
    //! @param [in] pds Private Data Specifier.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString PDSName(PDS pds, NamesFlags flags = NamesFlags::NAME);
}
