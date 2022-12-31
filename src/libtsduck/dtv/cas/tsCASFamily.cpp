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

#include "tsCASFamily.h"
#include "tsNamesFile.h"
#include "tsPSI.h"


//----------------------------------------------------------------------------
// Return a CAS family from a CA system id.
// Useful to analyze CA descriptors.
//----------------------------------------------------------------------------

ts::CASFamily ts::CASFamilyOf(uint16_t casid)
{
    struct CASDesc {
        CASFamily family;
        uint16_t  min;
        uint16_t  max;
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
