//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsVatekUtils.h"

#if !defined(TS_NO_VATEK)
#include "tsBeforeStandardHeaders.h"
#include <vatek_sdk_device.h>
#include "tsAfterStandardHeaders.h"
#endif


//-----------------------------------------------------------------------------
// Check if this version of TSDuck was built with VATek support.
//-----------------------------------------------------------------------------

bool ts::HasVatekSupport()
{
#if defined(TS_NO_VATEK)
    return false;
#else
    return true;
#endif
}


//-----------------------------------------------------------------------------
// Get the version of VATek library.
//-----------------------------------------------------------------------------

ts::UString ts::GetVatekVersion()
{
#if defined(TS_NO_VATEK)
    return u"This version of TSDuck was compiled without VATec support";
#elif !defined(VATEK_VERSION)
    return u"3.06 or lower";
#else
    UString version;
    version.format(u"libvatek version %d.%02d", VATEK_VERSION / 10000, (VATEK_VERSION / 100) % 100);
#if (VATEK_VERSION % 100) != 0
    version.format(u".%02d", VATEK_VERSION % 100);
#endif
    return version;
#endif
}
