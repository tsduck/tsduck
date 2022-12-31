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

#include "tsAbstractVideoAccessUnit.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AbstractVideoAccessUnit::AbstractVideoAccessUnit() :
    SuperClass(),
    rbsp_trailing_bits_valid(false),
    rbsp_trailing_bits_count(0)
{
}


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
