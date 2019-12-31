//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  @ingroup mpeg
//!  MPEG Program Clock Reference (PCR) utilities
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"

namespace ts {

    //!
    //! Size in bytes of a Program Clock Reference (PCR).
    //!
    const size_t PCR_SIZE = 6;

    //!
    //! This routine extracts a PCR from a stream.
    //! @param [in] b Address of a 6-byte memory area containing a PCR binary value.
    //! @return A 42-bit PCR value.
    //!
    TSDUCKDLL uint64_t GetPCR(const uint8_t* b);

    //!
    //! This routine inserts a PCR in a stream.
    //! @param [out] b Address of a 6-byte memory area to write the PCR binary value.
    //! @param [in] pcr A 42-bit PCR value.
    //!
    TSDUCKDLL void PutPCR(uint8_t* b, const uint64_t& pcr);
}
