//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Conditional Access Systems general definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Known Conditional Access Systems families.
    //! @ingroup mpeg
    //!
    enum CASFamily : uint8_t {
        CAS_OTHER       = 0,  //!< Unknown CAS.
        CAS_MEDIAGUARD  = 1,  //!< MediaGuard (Canal+ Technologies).
        CAS_NAGRA       = 2,  //!< Nagravision.
        CAS_VIACCESS    = 3,  //!< Viaccess.
        CAS_THALESCRYPT = 4,  //!< ThalesCrypt (for TPS).
        CAS_SAFEACCESS  = 5,  //!< SafeAccess (Logiways).
        CAS_WIDEVINE    = 6,  //!< Widevine CAS (Google).
        CAS_NDS         = 7,  //!< Synamedia, formerly NDS, formerly Cisco Video Solutions.
        CAS_IRDETO      = 8,  //!< Irdeto.
        CAS_CONAX       = 9,  //!< Conax, now part of Nagravision.
    };

    //!
    //! Return a CAS family from a CA system id.
    //! Useful to analyze CA descriptors.
    //! @param [in] ca_system_id DVB-allocated CA system id.
    //! @return A CAS family enumeration value.
    //!
    TSDUCKDLL CASFamily CASFamilyOf(uint16_t ca_system_id);

    //!
    //! Name of Conditional Access Families.
    //! @param [in] cas CAS family
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString CASFamilyName(CASFamily cas);

    //!
    //! Selected DVB-assigned CA System Identifier values
    //!
    enum : uint16_t {
        CASID_NULL            = 0x0000,  //!< Null/reserved/invalid CAS Id. Can be used to indicated "unspecified".
        CASID_MEDIAGUARD_MIN  = 0x0100,  //!< Minimum CAS Id value for MediaGuard.
        CASID_MEDIAGUARD_MAX  = 0x01FF,  //!< Maximum CAS Id value for MediaGuard.
        CASID_VIACCESS_MIN    = 0x0500,  //!< Minimum CAS Id value for Viaccess.
        CASID_VIACCESS_MAX    = 0x05FF,  //!< Maximum CAS Id value for Viaccess.
        CASID_IRDETO_MIN      = 0x0600,  //!< Minimum CAS Id value for Irdeto.
        CASID_IRDETO_MAX      = 0x06FF,  //!< Maximum CAS Id value for Irdeto.
        CASID_NDS_MIN         = 0x0900,  //!< Minimum CAS Id value for NDS.
        CASID_NDS_MAX         = 0x09FF,  //!< Maximum CAS Id value for NDS.
        CASID_CONAX_MIN       = 0x0B00,  //!< Minimum CAS Id value for Conax.
        CASID_CONAX_MAX       = 0x0BFF,  //!< Maximum CAS Id value for Conax.
        CASID_CRYPTOWORKS_MIN = 0x0D00,  //!< Minimum CAS Id value for CryptoWorks (Irdeto).
        CASID_CRYPTOWORKS_MAX = 0x0DFF,  //!< Maximum CAS Id value for CryptoWorks (Irdeto).
        CASID_NAGRA_MIN       = 0x1800,  //!< Minimum CAS Id value for Nagravision.
        CASID_NAGRA_MAX       = 0x18FF,  //!< Maximum CAS Id value for Nagravision.
        CASID_THALESCRYPT_MIN = 0x4A80,  //!< Minimum CAS Id value for ThalesCrypt.
        CASID_THALESCRYPT_MAX = 0x4A8F,  //!< Maximum CAS Id value for ThalesCrypt.
        CASID_WIDEVINE_MIN    = 0x4AD4,  //!< Minimum CAS Id value for Widevine CAS (Google).
        CASID_WIDEVINE_MAX    = 0x4AD5,  //!< Maximum CAS Id value for Widevine CAS (Google).
        CASID_SAFEACCESS      = 0x4ADC,  //!< CAS Id value for SafeAccess.
    };
}
