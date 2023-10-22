//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractVideoStructure.h"

bool ts::AbstractVideoStructure::parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> params)
{
    clear();
    if (data == nullptr) {
        valid = false;
    }
    else {
        AVCParser parser(data, size);
        valid = parse(parser, params);
    }
    return valid;
}
