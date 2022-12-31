//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------

#include "tsAbstractHEVCAccessUnit.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AbstractHEVCAccessUnit::AbstractHEVCAccessUnit() :
    SuperClass(),
    forbidden_zero_bit(0),
    nal_unit_type(0),
    nuh_layer_id(0),
    nuh_temporal_id_plus1(0)
{
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::AbstractHEVCAccessUnit::clear()
{
    SuperClass::clear();
    forbidden_zero_bit = 0;
    nal_unit_type = 0;
    nuh_layer_id = 0;
    nuh_temporal_id_plus1 = 0;
}


//----------------------------------------------------------------------------
// Parse the HEVC access unit header.
//----------------------------------------------------------------------------

bool ts::AbstractHEVCAccessUnit::parseHeader(const uint8_t*& data, size_t& size, std::initializer_list<uint32_t>)
{
    if (data == nullptr || size < 2) {
        return false;
    }
    else {
        forbidden_zero_bit = (data[0] >> 7) & 0x01;
        nal_unit_type = (data[0] >> 1) & 0x3F;
        nuh_layer_id = uint8_t((GetUInt16(data) >> 3) & 0x3F);
        nuh_temporal_id_plus1 = data[1] & 0x07;
        data += 2;
        size -= 2;
        return true;
    }
}
