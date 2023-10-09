//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractVVCAccessUnit.h"


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::AbstractVVCAccessUnit::clear()
{
    SuperClass::clear();
    forbidden_zero_bit = 0;
    nuh_reserved_zero_bit = 0;
    nuh_layer_id = 0;
    nal_unit_type = 0;
    nuh_temporal_id_plus1 = 0;
}


//----------------------------------------------------------------------------
// Parse the VVC access unit header.
//----------------------------------------------------------------------------

bool ts::AbstractVVCAccessUnit::parseHeader(const uint8_t*& data, size_t& size, std::initializer_list<uint32_t>)
{
    if (data == nullptr || size < 2) {
        return false;
    }
    else {
        forbidden_zero_bit = (data[0] >> 7) & 0x01;
        nuh_reserved_zero_bit= (data[0] >> 6) & 0x01;
        nuh_layer_id = data[0] & 0x3F;
        nal_unit_type = (data[1] >> 3) & 0x1F;
        nuh_temporal_id_plus1 = data[1] & 0x07;
        data += 2;
        size -= 2;
        return true;
    }
}
