//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Conditional Access Systems families
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Known Conditional Access Systems families.
    //! @ingroup mpeg
    //!
    enum CASFamily {
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
}
