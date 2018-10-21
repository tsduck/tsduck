//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  Base class for AVC access units, aka NALunits
//  (AVC, Advanced Video Coding, ISO 14496-10, ITU H.264)
//
//----------------------------------------------------------------------------

#include "tsAbstractAVCAccessUnit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AbstractAVCAccessUnit::AbstractAVCAccessUnit() :
    AbstractAVCData(),
    forbidden_zero_bit (0),
    nal_ref_idc (0),
    nal_unit_type (0)
{
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::AbstractAVCAccessUnit::clear()
{
    SuperClass::clear();
    forbidden_zero_bit = 0;
    nal_ref_idc = 0;
    nal_unit_type = 0;
}


//----------------------------------------------------------------------------
// Parse the binary access unit. Return the "valid" flag.
//----------------------------------------------------------------------------

bool ts::AbstractAVCAccessUnit::parse(const void* nalunit, size_t nalunit_size)
{
    if (nalunit == nullptr || nalunit_size < 1) {
        clear();
        return false;
    }
    else {
        // Parse AVC access unit. Skip first byte (NALunit type).
        // The RBSP (Raw Byte Sequence Payload) starts at second byte.
        const uint8_t* data = reinterpret_cast <const uint8_t*> (nalunit);

        valid = true;
        forbidden_zero_bit = (data[0] >> 7) & 0x01;
        nal_ref_idc = (data[0] >> 5) & 0x03;
        nal_unit_type = data[0] & 0x1F;

        AVCParser parser(data + 1, nalunit_size - 1);
        return parseBody(parser);
    }
}
