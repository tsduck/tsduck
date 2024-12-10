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
// CAS families and ranges.
//----------------------------------------------------------------------------

namespace {

    struct CASDesc {
        ts::CASFamily family;
        ts::CASID     min;
        ts::CASID     max;
    };

    static const CASDesc table[] = {
        {ts::CAS_MEDIAGUARD,  ts::CASID_MEDIAGUARD_MIN,  ts::CASID_MEDIAGUARD_MAX},
        {ts::CAS_NAGRA,       ts::CASID_NAGRA_MIN,       ts::CASID_NAGRA_MAX},
        {ts::CAS_VIACCESS,    ts::CASID_VIACCESS_MIN,    ts::CASID_VIACCESS_MAX},
        {ts::CAS_THALESCRYPT, ts::CASID_THALESCRYPT_MIN, ts::CASID_THALESCRYPT_MAX},
        {ts::CAS_SAFEACCESS,  ts::CASID_SAFEACCESS,      ts::CASID_SAFEACCESS},
        {ts::CAS_WIDEVINE,    ts::CASID_WIDEVINE_MIN,    ts::CASID_WIDEVINE_MAX},
        {ts::CAS_NDS,         ts::CASID_NDS_MIN,         ts::CASID_NDS_MAX},
        {ts::CAS_IRDETO,      ts::CASID_IRDETO_MIN,      ts::CASID_IRDETO_MAX},
        {ts::CAS_IRDETO,      ts::CASID_CRYPTOWORKS_MIN, ts::CASID_CRYPTOWORKS_MAX},
        {ts::CAS_CONAX,       ts::CASID_CONAX_MIN,       ts::CASID_CONAX_MAX},
        {ts::CAS_OTHER,       0x0000,                    0xFFFF},
    };
}


//----------------------------------------------------------------------------
// Return a CAS family from a CA system id.
//----------------------------------------------------------------------------

ts::CASFamily ts::CASFamilyOf(CASID casid)
{
    for (const CASDesc* it = table; ; ++it) {
        if (casid >= it->min && casid <= it->max) {
            return it->family;
        }
    }
}


//----------------------------------------------------------------------------
// Get the minimum and maximum CA system id in a CAS family.
//----------------------------------------------------------------------------

bool ts::GetCASIdRange(CASFamily cas, CASID& min, CASID& max)
{
    for (const CASDesc* it = table; it->family != CAS_OTHER; ++it) {
        if (cas == it->family) {
            min = it->min;
            max = it->max;
            return true;
        }
    }
    min = max = CASID_NULL;
    return false;
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
