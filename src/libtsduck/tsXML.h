//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!
//!  @file
//!  Encapsulation of TinyXML-2 header.
//!  All applications should use this header file instead of tinyxml.h.
//!
//!  - TinXML-2 home page: http://leethomason.github.io/tinyxml2/
//!  - TinXML-2 repository: https://github.com/leethomason/tinyxml2
//!  - TinXML-2 documentation: http://leethomason.github.io/tinyxml2/annotated.html
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

// Definitions which are used by TinyXML-2.
#if defined(__windows) && defined(_TSDUCKDLL_IMPL) && !defined(TINYXML2_EXPORT)
    #define TINYXML2_EXPORT 1
#elif defined(__windows) && defined(_TSDUCKDLL_USE) && !defined(TINYXML2_IMPORT)
    #define TINYXML2_IMPORT 1
#endif

#include "tinyxml2.h"
