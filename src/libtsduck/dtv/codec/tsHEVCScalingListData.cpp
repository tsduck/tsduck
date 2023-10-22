//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCScalingListData.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::HEVCScalingListData::HEVCScalingListData(const uint8_t* data, size_t size)
{
    HEVCScalingListData::parse(data, size);
}



//----------------------------------------------------------------------------
// Parse a memory area.
//----------------------------------------------------------------------------

bool ts::HEVCScalingListData::parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> params)
{
    return SuperClass::parse(data, size, params);
}

bool ts::HEVCScalingListData::parse(AVCParser& parser, std::initializer_list<uint32_t> params)
{
    clear();
    valid = true;

    for (size_t sizeId = 0; valid && sizeId < 4; sizeId++) {
        for (size_t matrixId = 0; valid && matrixId < 6; matrixId += sizeId == 3 ? 3 : 1) {
            Scaling& sc(list[sizeId][matrixId]);
            sc.scaling_list_delta_coef.clear();
            valid = parser.u(sc.scaling_list_pred_mode_flag, 1);
            if (!sc.scaling_list_pred_mode_flag) {
                valid = valid && parser.ue(sc.scaling_list_pred_matrix_id_delta);
            }
            else {
                const size_t coefNum = std::min<size_t>(64, (size_t(1) << (4 + size_t(sizeId << 1))));
                if (sizeId > 1) {
                    valid = valid && parser.se(sc.scaling_list_dc_coef_minus8);
                }
                for (size_t i = 0; valid && i < coefNum; i++) {
                    int32_t v = 0;
                    valid = parser.se(v);
                    sc.scaling_list_delta_coef.push_back(v);
                }
            }
        }
    }

    return valid;
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::HEVCScalingListData::display(std::ostream& out, const UString& margin, int level) const
{
#define DISP(n) out << margin << #n "[" << sizeId << "][" << matrixId << "] = " << int64_t(sc.n) << std::endl

    if (valid) {
        for (size_t sizeId = 0; sizeId < 4; sizeId++) {
            for (size_t matrixId = 0; matrixId < 6; matrixId += sizeId == 3 ? 3 : 1) {
                const Scaling& sc(list[sizeId][matrixId]);
                DISP(scaling_list_pred_mode_flag);
                if (!sc.scaling_list_pred_mode_flag) {
                    DISP(scaling_list_pred_matrix_id_delta);
                }
                else {
                    if (sizeId > 1) {
                        DISP(scaling_list_dc_coef_minus8);
                    }
                    for (size_t i = 0; valid && i < sc.scaling_list_delta_coef.size(); i++) {
                        out << margin << "scaling_list_delta_coef[" << sizeId << "][" << matrixId << "][" << i << "] = " << sc.scaling_list_delta_coef[i] << std::endl;
                    }
                }
            }
        }
    }
    return out;

#undef DISP
}
