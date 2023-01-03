//-----------------------------------------------------------------------------
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
    version.format(u"libvatek version %d.%02d", {VATEK_VERSION / 10000, (VATEK_VERSION / 100) % 100});
#if (VATEK_VERSION % 100) != 0
    version.format(u".%02d", {VATEK_VERSION % 100});
#endif
    return version;
#endif
}
