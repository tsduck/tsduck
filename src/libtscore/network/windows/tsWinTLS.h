//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  SSL/TLS - Windows specific headers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsWinUtils.h"

#if !defined(DOXYGEN)
    #define SECURITY_WIN32 1           // used by sspi.h (versus SECURITY_KERNEL)
    #define SCHANNEL_USE_BLACKLISTS 1  // for SCH_CREDENTIALS
#endif

#include "tsBeforeStandardHeaders.h"
#include <subauth.h>
#include <sspi.h>
#include <schannel.h>
#include "tsAfterStandardHeaders.h"

#if defined(TS_MSC)
    #pragma comment(lib, "crypt32.lib")
    #pragma comment(lib, "secur32.lib")
    #pragma comment(lib, "ncrypt.lib")
#endif
