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
//!  Utility routines for memory operations.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

//!
//! Zeroing an plain memory variable.
//! Do not use with instances of C++ classes.
//! @param [out] var Name of a variable.
//!
#define TS_ZERO(var) ts::Zero(&(var), sizeof(var))

namespace ts {

    //!
    //! Zeroing a memory area.
    //! @param [out] addr Address of a memory area to fill with zeroes.
    //! @param [in] size Size in bytes of the memory area.
    //!
    TSDUCKDLL inline void Zero(void* addr, size_t size) {
#if defined (__windows)
        ::SecureZeroMemory(addr, size);
#else
        ::memset(addr, 0, size);
#endif
    }

    //!
    //! Check if a memory area starts with the specified prefix.
    //! @param [in] area Address of a memory area to check.
    //! @param [in] area_size Size in bytes of the memory area.
    //! @param [in] prefix Address of the content of the prefix to check.
    //! @param [in] prefix_size Size in bytes of the prefix.
    //! @return True if @a area starts with @a prefix.
    //!
    TSDUCKDLL bool StartsWith(const void* area, size_t area_size, const void* prefix, size_t prefix_size);

    //!
    //! Locate a pattern into a memory area.
    //! @param [in] area Address of a memory area to check.
    //! @param [in] area_size Size in bytes of the memory area.
    //! @param [in] pattern Address of the content of the pattern to check.
    //! @param [in] pattern_size Size in bytes of the pattern.
    //! @return Address of the first occurence of @a pattern in @a area or zero if not found.
    //!
    TSDUCKDLL const void* LocatePattern(const void* area, size_t area_size, const void* pattern, size_t pattern_size);
}
