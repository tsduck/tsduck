//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Conditional Access Systems general definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"

namespace ts {

    class DuckContext;

    //!
    //! Conditional Access System Id.
    //!
    using CASID = uint16_t;

    //!
    //! Maximum number of CASID values.
    //!
    constexpr size_t CASID_MAX = 0x10000;

    //!
    //! Known Conditional Access Systems families.
    //! These symbols be be used in the TSDuck C++ code.
    //! More CAS families can be defined in '.names' files in extensions, in the [CASFamily] entry.
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
    TSDUCKDLL CASFamily CASFamilyOf(CASID ca_system_id);

    //!
    //! Get the minimum and maximum CA system id in a CAS family.
    //! @param [in] cas CAS family.
    //! @param [out] min First DVB-allocated CA system id for @a cas or CASID_NULL if unknown.
    //! @param [out] max Last DVB-allocated CA system id for @a cas or CASID_NULL if unknown.
    //! @return True if @a cas was found, false otherwise.
    //!
    TSDUCKDLL bool GetCASIdRange(CASFamily cas, CASID& min, CASID& max);

    //!
    //! Get the lowest CA system id in a CAS family.
    //! @param [in] cas CAS family.
    //! @return First DVB-allocated CA system id for @a cas or CASID_NULL if unknown.
    //!
    TSDUCKDLL CASID FirstCASId(CASFamily cas);

    //!
    //! Get the highest CA system id in a CAS family.
    //! @param [in] cas CAS family.
    //! @return Last DVB-allocated CA system id for @a cas or CASID_NULL if unknown.
    //!
    TSDUCKDLL CASID LastCASId(CASFamily cas);

    //!
    //! Name of Conditional Access Families.
    //! @param [in] cas CAS family
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString CASFamilyName(CASFamily cas);

    //!
    //! Get the set of all defined Conditional Access Families.
    //! This may include CAS families from extensions.
    //! @param [out] cas Set of all CAS families.
    //!
    TSDUCKDLL void GetAllCASFamilies(std::set<CASFamily>& cas);

    //!
    //! Selected DVB-assigned CA System Identifier values
    //!
    enum : CASID {
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

    //!
    //! Name of a Conditional Access System Id (as in CA Descriptor).
    //! @param [in] duck TSDuck execution context (used to select from other standards).
    //! @param [in] casid Conditional Access System Id.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString CASIdName(const DuckContext& duck, CASID casid, NamesFlags flags = NamesFlags::NAME);
}
