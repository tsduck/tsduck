//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCAccessUnitDelimiter.h"
#include "tsHEVC.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HEVCAccessUnitDelimiter::HEVCAccessUnitDelimiter(const uint8_t* data, size_t size)
{
    HEVCAccessUnitDelimiter::parse(data, size);
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::HEVCAccessUnitDelimiter::clear()
{
    SuperClass::clear();
    pic_type = 0;
}


//----------------------------------------------------------------------------
// Parse the body of the binary access unit. Return the "valid" flag.
//----------------------------------------------------------------------------

bool ts::HEVCAccessUnitDelimiter::parseBody(AVCParser& parser, std::initializer_list<uint32_t>)
{
    return nal_unit_type == HEVC_AUT_AUD_NUT && parser.u(pic_type, 3);
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::HEVCAccessUnitDelimiter::display(std::ostream& out, const UString& margin, int level) const
{
#define DISP(n) disp(out, margin, u ## #n, n)

    if (valid) {
        DISP(forbidden_zero_bit);
        DISP(nal_unit_type);
        DISP(nuh_layer_id);
        DISP(nuh_temporal_id_plus1);
        DISP(pic_type);
        DISP(rbsp_trailing_bits_valid);
        DISP(rbsp_trailing_bits_count);
    }
    return out;

#undef DISP
}
