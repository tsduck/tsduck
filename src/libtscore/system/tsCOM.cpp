//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCOM.h"

#if defined(TS_WINDOWS)
    #include "tsWinUtils.h"
    #include "tsBeforeStandardHeaders.h"
    #include <comutil.h>
    #include "tsAfterStandardHeaders.h"
#endif

// Constructor, initialize COM.
ts::COM::COM(Report& report)
{
#if defined(TS_WINDOWS)
    _is_init = ComSuccess(::CoInitializeEx(nullptr, ::COINIT_MULTITHREADED), u"COM initialization", report);
#else
    _is_init = true;
#endif
}

// Destructor, deinitialize COM.
ts::COM::~COM()
{
    uninitialize();
}

// Perform an early COM uninitialize (before destructor).
void ts::COM::uninitialize()
{
#if defined(TS_WINDOWS)
    if (_is_init) {
        ::CoUninitialize();
    }
#endif
    _is_init = false;
}
