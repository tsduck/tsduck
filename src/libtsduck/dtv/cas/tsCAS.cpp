//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCAS.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Return a CAS family from a CA system id.
// Useful to analyze CA descriptors.
//----------------------------------------------------------------------------

ts::CASFamily ts::CASFamilyOf(CASID casid)
{
    struct CASDesc {
        CASFamily family;
        CASID     min;
        CASID     max;
    };

    static const CASDesc table[] = {
        {CAS_MEDIAGUARD,  CASID_MEDIAGUARD_MIN,  CASID_MEDIAGUARD_MAX},
        {CAS_NAGRA,       CASID_NAGRA_MIN,       CASID_NAGRA_MAX},
        {CAS_VIACCESS,    CASID_VIACCESS_MIN,    CASID_VIACCESS_MAX},
        {CAS_THALESCRYPT, CASID_THALESCRYPT_MIN, CASID_THALESCRYPT_MAX},
        {CAS_SAFEACCESS,  CASID_SAFEACCESS,      CASID_SAFEACCESS},
        {CAS_WIDEVINE,    CASID_WIDEVINE_MIN,    CASID_WIDEVINE_MAX},
        {CAS_NDS,         CASID_NDS_MIN,         CASID_NDS_MAX},
        {CAS_IRDETO,      CASID_IRDETO_MIN,      CASID_IRDETO_MAX},
        {CAS_IRDETO,      CASID_CRYPTOWORKS_MIN, CASID_CRYPTOWORKS_MAX},
        {CAS_CONAX,       CASID_CONAX_MIN,       CASID_CONAX_MAX},
        {CAS_OTHER,       0x0000,                0xFFFF},
    };

    for (const CASDesc* it = table; ; ++it) {
        if (casid >= it->min && casid <= it->max) {
            return it->family;
        }
    }
}


//----------------------------------------------------------------------------
// Name of Conditional Access Families.
//----------------------------------------------------------------------------

ts::UString ts::CASFamilyName(CASFamily cas)
{
    return NameFromDTV(u"CASFamily", cas, NamesFlags::NAME | NamesFlags::DECIMAL);
}


//----------------------------------------------------------------------------
// Name of a Conditional Access System Id (as in CA Descriptor).
//----------------------------------------------------------------------------

ts::UString ts::CASIdName(const DuckContext& duck, uint16_t casid, NamesFlags flags)
{
    // In the case of ISDB, look into another table (but only known names).
    if (bool(duck.standards() & Standards::ISDB)) {
        const UString name(NameFromDTV(u"ARIBCASystemId", NamesFile::Value(casid), flags | NamesFlags::NO_UNKNOWN));
        if (!name.empty()) {
            return name;
        }
    }

    // Not ISDB or not found in ISDB, use standard CAS names.
    return NameFromDTV(u"CASystemId", NamesFile::Value(casid), flags);
}
