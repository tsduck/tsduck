//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tsHEVCShortTermReferencePictureSetList.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::HEVCShortTermReferencePictureSetList::HEVCShortTermReferencePictureSetList(const uint8_t* data, size_t size, uint32_t num_short_term_ref_pic_sets) :
    SuperClass(),
    list()
{
    reset(num_short_term_ref_pic_sets);
    for (uint32_t i = 0; valid && i < num_short_term_ref_pic_sets; ++i) {
        valid = parse(data, size, {i});
    }
}


ts::HEVCShortTermReferencePictureSetList::ShortTermReferencePictureSet::ShortTermReferencePictureSet() :
    valid(false),
    inter_ref_pic_set_prediction_flag(0),
    delta_idx_minus1(0),
    delta_rps_sign(0),
    abs_delta_rps_minus1(0),
    use_cur_delta(),
    num_negative_pics(0),
    num_positive_pics(0),
    negative_pics(),
    positive_pics()
{
}

ts::HEVCShortTermReferencePictureSetList::ShortTermReferencePictureSet::CurrDelta::CurrDelta() :
    used_by_curr_pic_flag(0),
    use_delta_flag(0)
{
}

ts::HEVCShortTermReferencePictureSetList::ShortTermReferencePictureSet::DeltaPicture::DeltaPicture() :
    delta_poc_minus1(0),
    used_by_curr_pic_flag(0)
{
}


//----------------------------------------------------------------------------
// Clear or reset this object.
//----------------------------------------------------------------------------

void ts::HEVCShortTermReferencePictureSetList::clear()
{
    SuperClass::clear();
    list.clear();
}

void ts::HEVCShortTermReferencePictureSetList::reset(uint32_t num_short_term_ref_pic_sets)
{
    // Make sure that the list is properly cleared first.
    clear();
    list.resize(size_t(num_short_term_ref_pic_sets) + 1);
    // The global 'valid' of the list becomes true but the individual 'valid' in elements
    // remain false until they are successfully parsed.
    valid = true;
}

void ts::HEVCShortTermReferencePictureSetList::ShortTermReferencePictureSet::clear()
{
    valid = false;
    inter_ref_pic_set_prediction_flag = 0;
    delta_idx_minus1 = 0;
    delta_rps_sign = 0;
    abs_delta_rps_minus1 = 0;
    use_cur_delta.clear();
    num_negative_pics = 0;
    num_positive_pics = 0;
    negative_pics.clear();
    positive_pics.clear();
}


//----------------------------------------------------------------------------
// Compute the NumDeltaPocs[RefRpsIdx] variable.
//----------------------------------------------------------------------------

uint32_t ts::HEVCShortTermReferencePictureSetList::NumDeltaPocs(uint32_t RefRpsIdx) const
{
    // See ITU-T Rec. H.265, 7.4.8 (7-71).
    return RefRpsIdx < list.size() ? list[RefRpsIdx].num_negative_pics + list[RefRpsIdx].num_positive_pics : 0;
}


//----------------------------------------------------------------------------
// Parse a memory area.
//----------------------------------------------------------------------------

bool ts::HEVCShortTermReferencePictureSetList::parse(const uint8_t* data, size_t size, std::initializer_list<uint32_t> params)
{
    // Don't call superclass here, it would clear the object.
    AVCParser parser(data, size);
    return data != nullptr && parse(parser, params);
}

bool ts::HEVCShortTermReferencePictureSetList::parse(AVCParser& parser, std::initializer_list<uint32_t> params)
{
    // The stRpsIdx index must be passed in the initializer list of the parse() methods.
    if (params.size() < 1) {
        return false;
    }
    const uint32_t stRpsIdx = *params.begin();
    if (stRpsIdx >= list.size()) {
        return false;
    }

    // The st_ref_pic_set structure to parse.
    ShortTermReferencePictureSet& st(list[stRpsIdx]);
    st.clear();
    st.valid = true;

    if (stRpsIdx != 0) {
        st.valid = parser.u(st.inter_ref_pic_set_prediction_flag, 1);
    }

    if (st.valid && st.inter_ref_pic_set_prediction_flag) {
        if (stRpsIdx == num_short_term_ref_pic_sets()) {
            st.valid = parser.ue(st.delta_idx_minus1);
        }
        st.valid = st.valid &&
                   parser.u(st.delta_rps_sign, 1) &&
                   parser.ue(st.abs_delta_rps_minus1);

        // RefRpsIdx = stRpsIdx - (delta_idx_minus1 + 1)  (7-59)
        const uint32_t RefRpsIdx = stRpsIdx > st.delta_idx_minus1 ? stRpsIdx - (st.delta_idx_minus1 + 1) : 0;
        st.use_cur_delta.resize(NumDeltaPocs(RefRpsIdx) + 1);
        for (uint32_t j = 0; st.valid && j < st.use_cur_delta.size(); j++) {
            st.valid = parser.u(st.use_cur_delta[j].used_by_curr_pic_flag, 1);
            if (st.valid && !st.use_cur_delta[j].used_by_curr_pic_flag) {
                st.valid = parser.u(st.use_cur_delta[j].use_delta_flag, 1);
            }
        }
    }
    else if (st.valid) {
        st.valid = parser.ue(st.num_negative_pics) &&
                   parser.ue(st.num_positive_pics);
        st.negative_pics.resize(st.num_negative_pics);
        for (uint32_t i = 0; st.valid && i < st.negative_pics.size(); i++) {
            st.valid = parser.ue(st.negative_pics[i].delta_poc_minus1) &&
                       parser.u(st.negative_pics[i].used_by_curr_pic_flag, 1);
        }
        st.positive_pics.resize(st.num_positive_pics);
        for (uint32_t i = 0; st.valid && i < st.positive_pics.size(); i++) {
            st.valid = parser.ue(st.positive_pics[i].delta_poc_minus1) &&
                       parser.u(st.positive_pics[i].used_by_curr_pic_flag, 1);
        }
    }

    return st.valid;
}


//----------------------------------------------------------------------------
// Display structure content
//----------------------------------------------------------------------------

std::ostream& ts::HEVCShortTermReferencePictureSetList::display(std::ostream& out, const UString& margin, int level) const
{
    if (valid) {
        for (size_t stRpsIdx = 0 ; stRpsIdx < list.size(); ++stRpsIdx) {
            const ShortTermReferencePictureSet& st(list[stRpsIdx]);
            if (st.valid) {

#define DISP(n) out << margin << "[" << stRpsIdx << "]." #n " = " << int64_t(st.n) << std::endl
#define DISPsub(n1,i,n2) out << margin << "[" << stRpsIdx << "]." #n1 ".[" << i << "]." #n2 " = " << int64_t(st.n1[i].n2) << std::endl

                if (stRpsIdx != 0) {
                    DISP(inter_ref_pic_set_prediction_flag);
                }
                if (st.inter_ref_pic_set_prediction_flag) {
                    if (stRpsIdx == num_short_term_ref_pic_sets()) {
                        DISP(delta_idx_minus1);
                    }
                    DISP(delta_rps_sign);
                    DISP(abs_delta_rps_minus1);
                    for (uint32_t j = 0; st.valid && j < st.use_cur_delta.size(); j++) {
                        DISPsub(use_cur_delta, j, used_by_curr_pic_flag);
                        if (!st.use_cur_delta[j].used_by_curr_pic_flag) {
                            DISPsub(use_cur_delta, j, use_delta_flag);
                        }
                    }
                }
                else if (st.valid) {
                    DISP(num_negative_pics);
                    DISP(num_positive_pics);
                    for (uint32_t i = 0; st.valid && i < st.negative_pics.size(); i++) {
                        DISPsub(negative_pics, i, delta_poc_minus1);
                        DISPsub(negative_pics, i, used_by_curr_pic_flag);
                    }
                    for (uint32_t i = 0; st.valid && i < st.positive_pics.size(); i++) {
                        DISPsub(positive_pics, i, delta_poc_minus1);
                        DISPsub(positive_pics, i, used_by_curr_pic_flag);
                    }
                }
#undef DISPsub
#undef DISP
            }
        }
    }
    return out;
}
