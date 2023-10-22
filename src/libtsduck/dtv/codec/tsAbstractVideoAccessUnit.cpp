//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractVideoAccessUnit.h"


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::AbstractVideoAccessUnit::clear()
{
    SuperClass::clear();
    rbsp_trailing_bits_valid = false;
    rbsp_trailing_bits_count = 0;
}


//----------------------------------------------------------------------------
// Parse the binary access unit. Return the "valid" flag.
//----------------------------------------------------------------------------

bool ts::AbstractVideoAccessUnit::parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> params)
{
    clear();
    if (data == nullptr || !parseHeader(data, size, params)) {
        return false;
    }
    AVCParser parser(data, size);
    valid = parseBody(parser, params);
    if (valid) {
        rbsp_trailing_bits_valid = parser.rbspTrailingBits();
        rbsp_trailing_bits_count = parser.remainingBits();
    }
    return valid;
}
