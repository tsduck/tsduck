//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAVCAccessUnitDelimiter.h"
#include "tsAVC.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AVCAccessUnitDelimiter::AVCAccessUnitDelimiter(const uint8_t* data, size_t size)
{
    parse(data, size);
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::AVCAccessUnitDelimiter::clear()
{
    SuperClass::clear();
    primary_pic_type = 0;
}


//----------------------------------------------------------------------------
// Parse the body of the binary access unit. Return the "valid" flag.
//----------------------------------------------------------------------------

bool ts::AVCAccessUnitDelimiter::parseBody(AVCParser& parser, std::initializer_list<uint32_t>)
{
    return nal_unit_type == AVC_AUT_DELIMITER && parser.u(primary_pic_type, 3);
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::AVCAccessUnitDelimiter::display(std::ostream& out, const UString& margin, int level) const
{
#define DISP(n) disp(out, margin, u ## #n, n)

    if (valid) {
        DISP(forbidden_zero_bit);
        DISP(nal_ref_idc);
        DISP(nal_unit_type);
        DISP(primary_pic_type);
        DISP(rbsp_trailing_bits_valid);
        DISP(rbsp_trailing_bits_count);
    }
    return out;

#undef DISP
}
