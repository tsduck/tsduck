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

#include "tsHEVCHRDParameters.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::HEVCHRDParameters::HEVCHRDParameters(const uint8_t* data, size_t size, bool commonInfPresentFlag, size_t maxNumSubLayersMinus1) :
    SuperClass(),
    common_inf_present_flag(commonInfPresentFlag),
    nal_hrd_parameters_present_flag(0),
    vcl_hrd_parameters_present_flag(0),
    sub_pic_hrd_params_present_flag(0),
    tick_divisor_minus2(0),
    du_cpb_removal_delay_increment_length_minus1(0),
    sub_pic_cpb_params_in_pic_timing_sei_flag(0),
    dpb_output_delay_du_length_minus1(0),
    bit_rate_scale(0),
    cpb_size_scale(0),
    cpb_size_du_scale(0),
    initial_cpb_removal_delay_length_minus1(0),
    au_cpb_removal_delay_length_minus1(0),
    dpb_output_delay_length_minus1(0),
    sub_layers()
{
    parse(data, size, {uint32_t(commonInfPresentFlag), uint32_t(maxNumSubLayersMinus1)});
}

ts::HEVCHRDParameters::SubLayerParams::SubLayerParams() :
    fixed_pic_rate_general_flag(0),
    fixed_pic_rate_within_cvs_flag(0),
    elemental_duration_in_tc_minus1(0),
    low_delay_hrd_flag(0),
    cpb_cnt_minus1(0),
    nal_hrd_parameters(),
    vcl_hrd_parameters()
{
}

ts::HEVCHRDParameters::CPBParams::CPBParams() :
    bit_rate_value_minus1(0),
    cpb_size_value_minus1(0),
    cpb_size_du_value_minus1(0),
    bit_rate_du_value_minus1(0),
    cbr_flag(0)
{
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::HEVCHRDParameters::clear()
{
    SuperClass::clear();
    common_inf_present_flag = false;
    nal_hrd_parameters_present_flag = 0;
    vcl_hrd_parameters_present_flag = 0;
    sub_pic_hrd_params_present_flag = 0;
    tick_divisor_minus2 = 0;
    du_cpb_removal_delay_increment_length_minus1 = 0;
    sub_pic_cpb_params_in_pic_timing_sei_flag = 0;
    dpb_output_delay_du_length_minus1 = 0;
    bit_rate_scale = 0;
    cpb_size_scale = 0;
    cpb_size_du_scale = 0;
    initial_cpb_removal_delay_length_minus1 = 0;
    au_cpb_removal_delay_length_minus1 = 0;
    dpb_output_delay_length_minus1 = 0;
    sub_layers.clear();
}


//----------------------------------------------------------------------------
// Parse a memory area.
//----------------------------------------------------------------------------

bool ts::HEVCHRDParameters::parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> params)
{
    return SuperClass::parse(data, size, params);
}

bool ts::HEVCHRDParameters::parse(AVCParser& parser, std::initializer_list<uint32_t> params)
{
    clear();

    // The two parameters commonInfPresentFlag and maxNumSubLayersMinus1 must be passed in the initializer list of the parse() methods.
    valid = params.size() >= 2;
    if (valid) {
        auto iparams = params.begin();
        common_inf_present_flag = bool(*iparams++);
        sub_layers.resize(size_t(*iparams) + 1);
    }
    HEVC_TRACE(u"----- HEVCHRDParameters::parse(), common_inf_present_flag=%d, sub_layers.size()=%d", common_inf_present_flag, sub_layers.size());

    if (valid && common_inf_present_flag) {
        valid = parser.u(nal_hrd_parameters_present_flag, 1) &&
                parser.u(vcl_hrd_parameters_present_flag, 1);
        if (valid && (nal_hrd_parameters_present_flag == 1 || vcl_hrd_parameters_present_flag == 1)) {
            valid = parser.u(sub_pic_hrd_params_present_flag, 1);
            if (valid && sub_pic_hrd_params_present_flag == 1) {
                valid = parser.u(tick_divisor_minus2, 8) &&
                        parser.u(du_cpb_removal_delay_increment_length_minus1, 5) &&
                        parser.u(sub_pic_cpb_params_in_pic_timing_sei_flag, 1) &&
                        parser.u(dpb_output_delay_du_length_minus1, 5);
            }
            valid = valid &&
                    parser.u(bit_rate_scale, 4) &&
                    parser.u(cpb_size_scale, 4);
            if (valid && sub_pic_hrd_params_present_flag == 1) {
                valid = parser.u(cpb_size_du_scale, 4);
            }
            valid = valid &&
                    parser.u(initial_cpb_removal_delay_length_minus1, 5) &&
                    parser.u(au_cpb_removal_delay_length_minus1, 5) &&
                    parser.u(dpb_output_delay_length_minus1, 5);
        }
        HEVC_TRACE(u"valid=%d, bit_rate_scale=%d, cpb_size_scale=%d, initial_cpb_removal_delay_length_minus1=%d", valid, bit_rate_scale, cpb_size_scale, initial_cpb_removal_delay_length_minus1);
        HEVC_TRACE(u"au_cpb_removal_delay_length_minus1=%d, dpb_output_delay_length_minus1=%d", au_cpb_removal_delay_length_minus1, dpb_output_delay_length_minus1);
    }

    for (size_t i = 0; valid && i < sub_layers.size(); i++) {
        SubLayerParams& sl(sub_layers[i]);
        valid = parser.u(sl.fixed_pic_rate_general_flag, 1);
        if (valid && sl.fixed_pic_rate_general_flag == 0) {
            valid = parser.u(sl.fixed_pic_rate_within_cvs_flag, 1);
        }
        else {
            // When fixed_pic_rate_general_flag is 1, fixed_pic_rate_within_cvs_flag is inferred to be 1.
            sl.fixed_pic_rate_within_cvs_flag = 1;
        }
        if (valid && sl.fixed_pic_rate_within_cvs_flag == 1) {
            valid = parser.ue(sl.elemental_duration_in_tc_minus1);
        }
        else {
            valid = parser.u(sl.low_delay_hrd_flag, 1);
        }
        if (valid && sl.low_delay_hrd_flag == 0) {
            valid = parser.ue(sl.cpb_cnt_minus1);
        }
        else {
            sl.cpb_cnt_minus1 = 0;
        }
        if (valid && nal_hrd_parameters_present_flag == 1) {
            sl.nal_hrd_parameters.resize(sl.cpb_cnt_minus1 + 1);
            valid = parse_sub_layer_hrd_parameters(parser, sl.nal_hrd_parameters);
        }
        if (valid && vcl_hrd_parameters_present_flag == 1) {
            sl.vcl_hrd_parameters.resize(sl.cpb_cnt_minus1 + 1);
            valid = parse_sub_layer_hrd_parameters(parser, sl.vcl_hrd_parameters);
        }
        HEVC_TRACE(u"valid=%d, end of sub_layer[%d]", valid, i);
    }

    return valid;
}

bool ts::HEVCHRDParameters::parse_sub_layer_hrd_parameters(AVCParser& parser, std::vector<CPBParams>& hrd_parameters)
{
    for (size_t i = 0; valid && i < hrd_parameters.size(); i++) {
        CPBParams& cpb(hrd_parameters[i]);
        valid = parser.ue(cpb.bit_rate_value_minus1) &&
                parser.ue(cpb.cpb_size_value_minus1);
        if (valid && sub_pic_hrd_params_present_flag == 1) {
            valid = parser.ue(cpb.cpb_size_du_value_minus1) &&
                    parser.ue(cpb.bit_rate_du_value_minus1);
        }
        valid = valid && parser.u(cpb.cbr_flag, 1);
    }
    return valid;
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::HEVCHRDParameters::display(std::ostream& out, const UString& margin, int level) const
{
#define DISP(n) disp(out, margin, u ## #n, n)

    if (valid) {
        DISP(common_inf_present_flag);
        if (common_inf_present_flag) {
            DISP(nal_hrd_parameters_present_flag);
            DISP(nal_hrd_parameters_present_flag);
            if (nal_hrd_parameters_present_flag == 1 || vcl_hrd_parameters_present_flag == 1) {
                DISP(sub_pic_hrd_params_present_flag);
                if (sub_pic_hrd_params_present_flag == 1) {
                    DISP(tick_divisor_minus2);
                    DISP(du_cpb_removal_delay_increment_length_minus1);
                    DISP(sub_pic_cpb_params_in_pic_timing_sei_flag);
                    DISP(dpb_output_delay_du_length_minus1);
                }
                DISP(bit_rate_scale);
                DISP(cpb_size_scale);
                if (sub_pic_hrd_params_present_flag == 1) {
                    DISP(cpb_size_du_scale);
                }
                DISP(initial_cpb_removal_delay_length_minus1);
                DISP(au_cpb_removal_delay_length_minus1);
                DISP(dpb_output_delay_length_minus1);
            }
        }

        for (size_t i = 0; valid && i < sub_layers.size(); i++) {
            const SubLayerParams& sl(sub_layers[i]);
            DISP(sl.fixed_pic_rate_general_flag);
            if (sl.fixed_pic_rate_general_flag == 0) {
                DISP(sl.fixed_pic_rate_within_cvs_flag);
            }
            if (sl.fixed_pic_rate_within_cvs_flag == 1) {
                DISP(sl.elemental_duration_in_tc_minus1);
            }
            else {
                DISP(sl.low_delay_hrd_flag);
            }
            if (sl.low_delay_hrd_flag == 0) {
                DISP(sl.cpb_cnt_minus1);
            }
            if (nal_hrd_parameters_present_flag == 1) {
                display_sub_layer_hrd_parameters(out, margin + u"nal_hrd_parameters", sl.nal_hrd_parameters);
            }
            if (vcl_hrd_parameters_present_flag == 1) {
                display_sub_layer_hrd_parameters(out, margin + u"vcl_hrd_parameters", sl.vcl_hrd_parameters);
            }
        }
    }
    return out;

#undef DISP
}

void ts::HEVCHRDParameters::display_sub_layer_hrd_parameters(std::ostream& out, const UString& margin, const std::vector<CPBParams>& hrd_parameters) const
{
    for (size_t i = 0; i < hrd_parameters.size(); i++) {
        const CPBParams& cpb(hrd_parameters[i]);
#define DISP(n) out << margin << "[" << i << "]." #n " = " << int64_t(n) << std::endl
        DISP(cpb.bit_rate_value_minus1);
        DISP(cpb.cpb_size_value_minus1);
        if (sub_pic_hrd_params_present_flag == 1) {
            DISP(cpb.cpb_size_du_value_minus1);
            DISP(cpb.bit_rate_du_value_minus1);
        }
        DISP(cpb.cbr_flag);
#undef DISP
    }
}
