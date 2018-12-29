//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//
//  MPEG Program Clock Reference (PCR) utilities
//
//----------------------------------------------------------------------------

#include "tsPCR.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// This routine extracts a PCR from a stream.
// Use 6 bytes at address b. Return a 42-bit value.
//----------------------------------------------------------------------------

uint64_t ts::GetPCR (const uint8_t* b)
{
    uint32_t v32 = GetUInt32 (b);
    uint16_t v16 = GetUInt16 (b + 4);
    uint64_t pcr_base = (uint64_t (v32) << 1) | uint64_t (v16 >> 15);
    uint64_t pcr_ext = uint64_t (v16 & 0x01FF);
    return pcr_base * 300 + pcr_ext;
}

//----------------------------------------------------------------------------
// This routine inserts a PCR in a stream.
// Writes 6 bytes at address b.
//----------------------------------------------------------------------------

void ts::PutPCR(uint8_t* b, const uint64_t& pcr)
{
    uint64_t pcr_base = pcr / 300;
    uint64_t pcr_ext = pcr % 300;
    PutUInt32(b, uint32_t(pcr_base >> 1));
    PutUInt16(b + 4, uint16_t(uint32_t((pcr_base << 15) | 0x7E00 | pcr_ext)));
}
