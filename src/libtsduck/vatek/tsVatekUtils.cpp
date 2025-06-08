//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsVatekUtils.h"
#include "tsFeatures.h"

#if !defined(TS_NO_VATEK)
#include "tsBeforeStandardHeaders.h"
#include <vatek_sdk_device.h>
#include "tsAfterStandardHeaders.h"
#endif


//----------------------------------------------------------------------------
// Register for options --version and --support.
//----------------------------------------------------------------------------

#if defined(TS_NO_VATEK)
    #define SUPPORT ts::Features::UNSUPPORTED
#else
    #define SUPPORT ts::Features::SUPPORTED
#endif

TS_REGISTER_FEATURE(u"vatek", u"VATek", SUPPORT, ts::GetVatekVersion);


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
    return u"This version of TSDuck was compiled without VATek support";
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
