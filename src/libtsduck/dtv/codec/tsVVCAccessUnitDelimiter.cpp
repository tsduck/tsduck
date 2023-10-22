//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVVCAccessUnitDelimiter.h"
#include "tsVVC.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::VVCAccessUnitDelimiter::VVCAccessUnitDelimiter(const uint8_t* data, size_t size)
{
    VVCAccessUnitDelimiter::parse(data, size);
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::VVCAccessUnitDelimiter::clear()
{
    SuperClass::clear();
    aud_irap_or_gdr_flag = 0;
    aud_pic_type = 0;
}


//----------------------------------------------------------------------------
// Parse the body of the binary access unit. Return the "valid" flag.
//----------------------------------------------------------------------------

bool ts::VVCAccessUnitDelimiter::parseBody(AVCParser& parser, std::initializer_list<uint32_t>)
{
    return nal_unit_type == VVC_AUT_AUD_NUT &&
           parser.u(aud_irap_or_gdr_flag, 1) &&
           parser.u(aud_pic_type, 3);
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::VVCAccessUnitDelimiter::display(std::ostream& out, const UString& margin, int level) const
{
#define DISP(n) disp(out, margin, u ## #n, n)

    if (valid) {
        DISP(forbidden_zero_bit);
        DISP(nuh_reserved_zero_bit);
        DISP(nuh_layer_id);
        DISP(nal_unit_type);
        DISP(nuh_temporal_id_plus1);
        DISP(aud_irap_or_gdr_flag);
        DISP(aud_pic_type);
        DISP(rbsp_trailing_bits_valid);
        DISP(rbsp_trailing_bits_count);
    }
    return out;

#undef DISP
}
