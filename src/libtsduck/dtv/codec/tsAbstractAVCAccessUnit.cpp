//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractAVCAccessUnit.h"


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
// Parse the AVC access unit header.
//----------------------------------------------------------------------------

bool ts::AbstractAVCAccessUnit::parseHeader(const uint8_t*& data, size_t& size, std::initializer_list<uint32_t> )
{
    if (data == nullptr || size < 1) {
        return false;
    }
    else {
        forbidden_zero_bit = (data[0] >> 7) & 0x01;
        nal_ref_idc = (data[0] >> 5) & 0x03;
        nal_unit_type = data[0] & 0x1F;
        data++;
        size--;
        return true;
    }
}
