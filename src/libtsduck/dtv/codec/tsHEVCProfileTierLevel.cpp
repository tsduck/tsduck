//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCProfileTierLevel.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::HEVCProfileTierLevel::HEVCProfileTierLevel(const uint8_t* data, size_t size, bool profilePresentFlag, size_t maxNumSubLayersMinus1) :
    profile_present_flag(profilePresentFlag)
{
    HEVCProfileTierLevel::parse(data, size, {uint32_t(profilePresentFlag), uint32_t(maxNumSubLayersMinus1)});
}


//----------------------------------------------------------------------------
// Clear all values
//----------------------------------------------------------------------------

void ts::HEVCProfileTierLevel::clear()
{
    SuperClass::clear();
    profile_present_flag = false;
    general_profile_space = 0;
    general_tier_flag = 0;
    general_profile_idc = 0;
    general_profile_compatibility_flag = 0;
    general_progressive_source_flag = 0;
    general_interlaced_source_flag = 0;
    general_non_packed_constraint_flag = 0;
    general_frame_only_constraint_flag = 0;
    general_max_12bit_constraint_flag = 0;
    general_max_10bit_constraint_flag = 0;
    general_max_8bit_constraint_flag = 0;
    general_max_422chroma_constraint_flag = 0;
    general_max_420chroma_constraint_flag = 0;
    general_max_monochrome_constraint_flag = 0;
    general_intra_constraint_flag = 0;
    general_one_picture_only_constraint_flag = 0;
    general_lower_bit_rate_constraint_flag = 0;
    general_max_14bit_constraint_flag = 0;
    general_inbld_flag = 0;
    general_level_idc = 0;
    sub_layers.clear();
}


//----------------------------------------------------------------------------
// Get the profile value.
//----------------------------------------------------------------------------

uint8_t ts::HEVCProfileTierLevel::profile() const
{
    if (profile_present_flag) {
        // Use general profile as base (0 to 31, read from 5-bit field).
        uint8_t prof = general_profile_idc;
        // Look for higher compatibility.
        for (uint8_t comp = prof + 1; comp < general_profile_compatibility_flag.size(); ++comp) {
            if (general_profile_compatibility_flag.test(comp)) {
                prof = comp;
            }
        }
        return prof;
    }
    else {
        // Unknown.
        return 0;
    }
}


//----------------------------------------------------------------------------
// Parse a memory area.
//----------------------------------------------------------------------------

bool ts::HEVCProfileTierLevel::parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> params)
{
    return SuperClass::parse(data, size, params);
}

bool ts::HEVCProfileTierLevel::parse(AVCParser& parser, std::initializer_list<uint32_t> params)
{
    clear();
    uint32_t dummy = 0;
    uint32_t maxNumSubLayersMinus1 = 0;

    // The two parameters profilePresentFlag and maxNumSubLayersMinus1 must be passed in the initializer list of the parse() methods.
    valid = params.size() >= 2;
    if (valid) {
        auto iparams = params.begin();
        profile_present_flag = bool(*iparams++);
        maxNumSubLayersMinus1 = *iparams;
    }

    if (valid && profile_present_flag) {
        valid = parser.u(general_profile_space, 2) &&
                parser.u(general_tier_flag, 1) &&
                parser.u(general_profile_idc, 5);
        for (size_t j = 0; valid && j < 32; ++j) {
            valid = parser.u(dummy, 1);
            general_profile_compatibility_flag.set(j, dummy != 0);
        }
        valid = valid &&
                parser.u(general_progressive_source_flag, 1) &&
                parser.u(general_interlaced_source_flag, 1) &&
                parser.u(general_non_packed_constraint_flag, 1) &&
                parser.u(general_frame_only_constraint_flag, 1) &&
                parser.u(general_max_12bit_constraint_flag, 1) &&
                parser.u(general_max_10bit_constraint_flag, 1) &&
                parser.u(general_max_8bit_constraint_flag, 1) &&
                parser.u(general_max_422chroma_constraint_flag, 1) &&
                parser.u(general_max_420chroma_constraint_flag, 1) &&
                parser.u(general_max_monochrome_constraint_flag, 1) &&
                parser.u(general_intra_constraint_flag, 1) &&
                parser.u(general_one_picture_only_constraint_flag, 1) &&
                parser.u(general_lower_bit_rate_constraint_flag, 1) &&
                parser.u(general_max_14bit_constraint_flag, 1) &&
                parser.u(dummy, 33) &&
                parser.u(general_inbld_flag, 1);
    }

    valid = valid && parser.u(general_level_idc, 8);

    sub_layers.resize(maxNumSubLayersMinus1);
    for (size_t i = 0; valid && i < maxNumSubLayersMinus1; ++i) {
        valid = parser.u(sub_layers[i].sub_layer_profile_present_flag, 1) &&
                parser.u(sub_layers[i].sub_layer_level_present_flag, 1);
    }

    if (valid && maxNumSubLayersMinus1 > 0) {
        for (uint32_t i = maxNumSubLayersMinus1; i < 8; i++) {
            valid = parser.u(dummy, 2);
        }
    }

    for (size_t i = 0; valid && i < maxNumSubLayersMinus1; ++i) {
        SubLayerParams& sl(sub_layers[i]);
        if (sl.sub_layer_profile_present_flag) {
            valid = parser.u(sl.sub_layer_profile_space, 2) &&
                    parser.u(sl.sub_layer_tier_flag, 1) &&
                    parser.u(sl.sub_layer_profile_idc, 5);
            for (size_t j = 0; valid && j < 32; ++j) {
                valid = parser.u(dummy, 1);
                sl.sub_layer_profile_compatibility_flag.set(j, dummy != 0);
            }
            valid = valid &&
                    parser.u(sl.sub_layer_progressive_source_flag, 1) &&
                    parser.u(sl.sub_layer_interlaced_source_flag, 1) &&
                    parser.u(sl.sub_layer_non_packed_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_frame_only_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_max_12bit_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_max_10bit_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_max_8bit_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_max_422chroma_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_max_420chroma_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_max_monochrome_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_intra_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_one_picture_only_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_lower_bit_rate_constraint_flag, 1) &&
                    parser.u(sl.sub_layer_max_14bit_constraint_flag, 1) &&
                    parser.u(dummy, 33) &&
                    parser.u(sl.sub_layer_inbld_flag, 1);
        }
        if (valid && sl.sub_layer_level_present_flag) {
            valid = parser.u(sl.sub_layer_level_idc, 8);
        }
    }

    return valid;
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::HEVCProfileTierLevel::display(std::ostream& out, const UString& margin, int level) const
{
#define DISP(n) disp(out, margin, u ## #n, n)
#define DISPsl(n) out << margin << "sub_layer[" << i << "]." #n " = " << int64_t(sub_layers[i].n) << std::endl

    if (valid) {
        DISP(profile_present_flag);
        if (profile_present_flag) {
            DISP(general_profile_space);
            DISP(general_tier_flag);
            DISP(general_profile_idc);
            for (size_t j = 0; valid && j < 32; ++j) {
                out << margin << "general_profile_compatibility_flag[" << j << "] = " << int(general_profile_compatibility_flag[j]) << std::endl;
            }
            DISP(general_progressive_source_flag);
            DISP(general_interlaced_source_flag);
            DISP(general_non_packed_constraint_flag);
            DISP(general_frame_only_constraint_flag);
            DISP(general_max_12bit_constraint_flag);
            DISP(general_max_10bit_constraint_flag);
            DISP(general_max_8bit_constraint_flag);
            DISP(general_max_422chroma_constraint_flag);
            DISP(general_max_420chroma_constraint_flag);
            DISP(general_max_monochrome_constraint_flag);
            DISP(general_intra_constraint_flag);
            DISP(general_one_picture_only_constraint_flag);
            DISP(general_lower_bit_rate_constraint_flag);
            DISP(general_max_14bit_constraint_flag);
            DISP(general_inbld_flag);
        }
        DISP(general_level_idc);

        for (size_t i = 0; valid && i < sub_layers.size(); ++i) {
            DISPsl(sub_layer_profile_present_flag);
            if (sub_layers[i].sub_layer_profile_present_flag) {
                DISPsl(sub_layer_profile_space);
                DISPsl(sub_layer_tier_flag);
                DISPsl(sub_layer_profile_idc);
                for (size_t j = 0; valid && j < 32; ++j) {
                    out << margin << "sub_layer[" << i << "].sub_layer_profile_compatibility_flag[" << j << "] = "
                        << int(sub_layers[i].sub_layer_profile_compatibility_flag[j]) << std::endl;
                }
                DISPsl(sub_layer_progressive_source_flag);
                DISPsl(sub_layer_interlaced_source_flag);
                DISPsl(sub_layer_non_packed_constraint_flag);
                DISPsl(sub_layer_frame_only_constraint_flag);
                DISPsl(sub_layer_max_12bit_constraint_flag);
                DISPsl(sub_layer_max_10bit_constraint_flag);
                DISPsl(sub_layer_max_8bit_constraint_flag);
                DISPsl(sub_layer_max_422chroma_constraint_flag);
                DISPsl(sub_layer_max_420chroma_constraint_flag);
                DISPsl(sub_layer_max_monochrome_constraint_flag);
                DISPsl(sub_layer_intra_constraint_flag);
                DISPsl(sub_layer_one_picture_only_constraint_flag);
                DISPsl(sub_layer_lower_bit_rate_constraint_flag);
                DISPsl(sub_layer_max_14bit_constraint_flag);
                DISPsl(sub_layer_inbld_flag);
            }
            DISPsl(sub_layer_level_present_flag);
            if (sub_layers[i].sub_layer_level_present_flag) {
                DISPsl(sub_layer_level_idc);
            }
        }
    }
    return out;

#undef DISPsl
#undef DISP
}
