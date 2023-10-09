//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAVCHRDParameters.h"


//----------------------------------------------------------------------------
// Constructor from a binary area
//----------------------------------------------------------------------------

ts::AVCHRDParameters::AVCHRDParameters(const uint8_t* data, size_t size)
{
    parse(data, size);
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::AVCHRDParameters::clear()
{
    SuperClass::clear();
    cpb_cnt_minus1 = 0;
    bit_rate_scale = 0;
    cpb_size_scale = 0;
    bit_rate_value_minus1.clear();
    cpb_size_value_minus1.clear();
    cbr_flag.clear();
    initial_cpb_removal_delay_length_minus1 = 0;
    cpb_removal_delay_length_minus1 = 0;
    dpb_output_delay_length_minus1 = 0;
    time_offset_length = 0;
}


//----------------------------------------------------------------------------
// Parse a memory area.
//----------------------------------------------------------------------------

bool ts::AVCHRDParameters::parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> params)
{
    return SuperClass::parse(data, size, params);
}

bool ts::AVCHRDParameters::parse(AVCParser& parser, std::initializer_list<uint32_t>)
{
    clear();

    valid =
        parser.ue(cpb_cnt_minus1) &&
        parser.u(bit_rate_scale, 4) &&
        parser.u(cpb_size_scale, 4);

    for (uint32_t i = 0; valid && i <= cpb_cnt_minus1; i++) {
        uint32_t x_bit_rate_value_minus1;
        uint32_t x_cpb_size_value_minus1;
        uint8_t  x_cbr_flag;
        valid = valid &&
            parser.ue(x_bit_rate_value_minus1) &&
            parser.ue(x_cpb_size_value_minus1) &&
            parser.u(x_cbr_flag, 1);
        if (valid) {
            bit_rate_value_minus1.push_back(x_bit_rate_value_minus1);
            cpb_size_value_minus1.push_back(x_cpb_size_value_minus1);
            cbr_flag.push_back(x_cbr_flag);
        }
    }

    valid = valid &&
        parser.u(initial_cpb_removal_delay_length_minus1, 5) &&
        parser.u(cpb_removal_delay_length_minus1, 5) &&
        parser.u(dpb_output_delay_length_minus1, 5) &&
        parser.u(time_offset_length, 5);

    return valid;
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::AVCHRDParameters::display(std::ostream& out, const UString& margin, int level) const
{
    if (valid) {
#define DISP(n) disp(out, margin, u ## #n, n)
        DISP(cpb_cnt_minus1);
        DISP(bit_rate_scale);
        DISP(cpb_size_scale);
        DISP(bit_rate_value_minus1);
        DISP(cpb_size_value_minus1);
        DISP(cbr_flag);
        DISP(initial_cpb_removal_delay_length_minus1);
        DISP(cpb_removal_delay_length_minus1);
        DISP(dpb_output_delay_length_minus1);
        DISP(time_offset_length);
#undef DISP
    }
    return out;
}
