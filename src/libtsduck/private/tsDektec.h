//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!
//!  @file
//!  Provide a safe way to include the DTAPI definition.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

#if defined(DOXYGEN)
    //! Externally defined when the DTAPI is not available.
    #define TS_NO_DTAPI
#elif defined(TS_NO_DTAPI)
    // An error message to display.
    #define TS_NO_DTAPI_MESSAGE u"This version of TSDuck was compiled without Dektec support"
    #define DTAPI_VERSION_MAJOR 0
    #define DTAPI_VERSION_MINOR 0
#else
    #define _NO_USING_NAMESPACE_DTAPI
    #include "DTAPI.h"
#endif

//!
//! Define a synthetic major/minor version number for DTAPI
//!
#define TS_DTAPI_VERSION ((DTAPI_VERSION_MAJOR * 100) + (DTAPI_VERSION_MINOR % 100))
