//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCShortTermReferencePictureSetList.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::HEVCShortTermReferencePictureSetList::HEVCShortTermReferencePictureSetList(const uint8_t* data, size_t size, uint32_t num_short_term_ref_pic_sets)
{
    reset(num_short_term_ref_pic_sets);
    for (uint32_t i = 0; valid && i < num_short_term_ref_pic_sets; ++i) {
        valid = HEVCShortTermReferencePictureSetList::parse(data, size, {i});
    }
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
    list.resize(size_t(num_short_term_ref_pic_sets));
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
    used_by_curr_pic_flag.clear();
    use_delta_flag.clear();
    num_negative_pics = 0;
    num_positive_pics = 0;
    delta_poc_s0_minus1.clear();
    used_by_curr_pic_s0_flag.clear();
    delta_poc_s1_minus1.clear();
    used_by_curr_pic_s1_flag.clear();
    NumNegativePics = 0;
    NumPositivePics = 0;
    UsedByCurrPicS0.clear();
    UsedByCurrPicS1.clear();
    DeltaPocS0.clear();
    DeltaPocS1.clear();
    NumDeltaPocs = 0;
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
    HEVC_TRACE(u"----- HEVCShortTermReferencePictureSetList::parse(), stRpsIdx=%d, st.inter_ref_pic_set_prediction_flag=%d", stRpsIdx, st.inter_ref_pic_set_prediction_flag);

    if (st.valid && st.inter_ref_pic_set_prediction_flag) {
        // This picture is predicted from a reference picture.
        if (stRpsIdx == num_short_term_ref_pic_sets()) {
            // The ShortTermReferencePictureSet index 0 to num_short_term_ref_pic_sets - 1 are stored
            // in the HEVCSequenceParameterSet. The one with index num_short_term_ref_pic_sets, when
            // present, is directly stored in the slice header. So, in practice, this index is
            // currently never deserialized here.
            st.valid = parser.ue(st.delta_idx_minus1);
        }

        // See ITU-T Rec. H.265, 7.4.8 (7-59). RefRpsIdx is always valid since it is lower than stRpsIdx.
        const uint32_t RefRpsIdx = stRpsIdx - std::min(stRpsIdx, st.delta_idx_minus1 + 1);
        const ShortTermReferencePictureSet& ref(list[RefRpsIdx]);

        // See ITU-T Rec. H.265, 7.4.8 (7-60).
        st.valid = st.valid && parser.u(st.delta_rps_sign, 1) && parser.ue(st.abs_delta_rps_minus1);
        const int32_t deltaRps = (st.delta_rps_sign ? -1 : 1) * (int32_t(st.abs_delta_rps_minus1) + 1);

        HEVC_TRACE(u"st.abs_delta_rps_minus1=%d, RefRpsIdx=%d, NumDeltaPocs(RefRpsIdx)=%d, deltaRps=%d", st.abs_delta_rps_minus1, RefRpsIdx, ref.NumDeltaPocs, deltaRps);
        st.used_by_curr_pic_flag.resize(ref.NumDeltaPocs + 1);
        st.use_delta_flag.resize(ref.NumDeltaPocs + 1, 0);
        for (uint32_t j = 0; st.valid && j <= ref.NumDeltaPocs; j++) {
            st.valid = parser.u(st.used_by_curr_pic_flag[j], 1);
            if (st.valid && !st.used_by_curr_pic_flag[j]) {
                st.valid = parser.u(st.use_delta_flag[j], 1);
            }
            else {
                // Inferred to be 1 if omitted (H.265, 7.4.8).
                st.use_delta_flag[j] = 1;
            }
        }
        HEVC_TRACE(u"st.used_by_curr_pic_flag=(%s)", UString::Decimal(st.used_by_curr_pic_flag));
        HEVC_TRACE(u"st.use_delta_flag=(%s)", UString::Decimal(st.use_delta_flag));

        // See ITU-T Rec. H.265, 7.4.8 (7-61).
        for (int32_t j = int32_t(ref.NumPositivePics) - 1; j >= 0; j--) {
            if (j < int32_t(ref.DeltaPocS1.size()) && ref.NumNegativePics + j < st.use_delta_flag.size()) {
                const int32_t dPoc = ref.DeltaPocS1[j] + deltaRps;
                HEVC_TRACE(u"S0, dPoc=%d, st.use_delta_flag[ref.NumNegativePics + j]=%d", dPoc, st.use_delta_flag[ref.NumNegativePics + j]);
                if (dPoc < 0 && st.use_delta_flag[ref.NumNegativePics + j] && ref.NumNegativePics + j < st.used_by_curr_pic_flag.size()) {
                    HEVC_TRACE(u"=> push negative pic (%d)", 1);
                    st.DeltaPocS0.push_back(dPoc);
                    st.UsedByCurrPicS0.push_back(st.used_by_curr_pic_flag[ref.NumNegativePics + j]);
                }
            }
        }
        if (deltaRps < 0 &&
            ref.NumDeltaPocs < st.use_delta_flag.size() &&
            st.use_delta_flag[ref.NumDeltaPocs] &&
            ref.NumDeltaPocs < st.used_by_curr_pic_flag.size())
        {
            HEVC_TRACE(u"=> push negative pic (%d)", 2);
            st.DeltaPocS0.push_back(deltaRps);
            st.UsedByCurrPicS0.push_back(st.used_by_curr_pic_flag[ref.NumDeltaPocs]);
        }
        for (uint32_t j = 0; j < ref.NumNegativePics; j++) {
            if (j < ref.DeltaPocS0.size() && j < ref.DeltaPocS0.size() && j < st.use_delta_flag.size()) {
                const int32_t dPoc = ref.DeltaPocS0[j] + deltaRps;
                HEVC_TRACE(u"S0, dPoc=%d, st.use_delta_flag[j]=%d", dPoc, st.use_delta_flag[j]);
                if (dPoc < 0 && st.use_delta_flag[j] && j < st.used_by_curr_pic_flag.size()) {
                    HEVC_TRACE(u"=> push negative pic (%d)", 3);
                    st.DeltaPocS0.push_back(dPoc);
                    st.UsedByCurrPicS0.push_back(st.used_by_curr_pic_flag[j]);
                }
            }
        }
        st.NumNegativePics = uint32_t(st.DeltaPocS0.size());
        assert(st.NumNegativePics == st.UsedByCurrPicS0.size());

        // See ITU-T Rec. H.265, 7.4.8 (7-62).
        for (int32_t j = int32_t(ref.NumNegativePics) - 1; j >= 0; j--) {
            if (j < int32_t(ref.DeltaPocS0.size()) && j < int32_t(st.use_delta_flag.size())) {
                const int32_t dPoc = ref.DeltaPocS0[j] + deltaRps;
                HEVC_TRACE(u"S1, dPoc=%d, st.use_delta_flag[j]=%d", dPoc, st.use_delta_flag[j]);
                if (dPoc > 0 && st.use_delta_flag[j] && j < int32_t(st.used_by_curr_pic_flag.size())) {
                    HEVC_TRACE(u"=> push positive pic (%d)", 1);
                    st.DeltaPocS1.push_back(dPoc);
                    st.UsedByCurrPicS1.push_back(st.used_by_curr_pic_flag[j]);
                }
            }
        }
        if (deltaRps > 0 &&
            ref.NumDeltaPocs < st.use_delta_flag.size() &&
            st.use_delta_flag[ref.NumDeltaPocs] &&
            ref.NumDeltaPocs < st.used_by_curr_pic_flag.size())
        {
            HEVC_TRACE(u"=> push positive pic (%d)", 2);
            st.DeltaPocS1.push_back(deltaRps);
            st.UsedByCurrPicS1.push_back(st.used_by_curr_pic_flag[ref.NumDeltaPocs]);
        }
        for (uint32_t j = 0; j < ref.NumPositivePics; j++) {
            if (j < ref.DeltaPocS1.size() && ref.NumNegativePics + j < st.use_delta_flag.size()) {
                const int32_t dPoc = ref.DeltaPocS1[j] + deltaRps;
                HEVC_TRACE(u"S1, dPoc=%d, st.use_delta_flag[ref.NumNegativePics + j]=%d", dPoc, st.use_delta_flag[ref.NumNegativePics + j]);
                if (dPoc > 0 && st.use_delta_flag[ref.NumNegativePics + j] && ref.NumNegativePics + j < st.used_by_curr_pic_flag.size()) {
                    HEVC_TRACE(u"=> push positive pic (%d)", 3);
                    st.DeltaPocS1.push_back(dPoc);
                    st.UsedByCurrPicS1.push_back(st.used_by_curr_pic_flag[ref.NumNegativePics + j]);
                }
            }
        }
        st.NumPositivePics = uint32_t(st.DeltaPocS1.size());
        assert(st.NumPositivePics == st.UsedByCurrPicS1.size());
    }
    else if (st.valid) {
        // This picture is not predicted, there is no reference picture.
        st.valid = parser.ue(st.num_negative_pics) && parser.ue(st.num_positive_pics);
        st.delta_poc_s0_minus1.resize(st.num_negative_pics);
        st.used_by_curr_pic_s0_flag.resize(st.num_negative_pics);
        for (uint32_t i = 0; st.valid && i < st.num_negative_pics; i++) {
            st.valid = parser.ue(st.delta_poc_s0_minus1[i]) && parser.u(st.used_by_curr_pic_s0_flag[i], 1);
        }
        st.delta_poc_s1_minus1.resize(st.num_positive_pics);
        st.used_by_curr_pic_s1_flag.resize(st.num_positive_pics);
        for (uint32_t i = 0; st.valid && i < st.num_positive_pics; i++) {
            st.valid = parser.ue(st.delta_poc_s1_minus1[i]) && parser.u(st.used_by_curr_pic_s1_flag[i], 1);
        }
        // See ITU-T Rec. H.265, 7.4.8 (7-63, 7-64).
        st.NumNegativePics = st.num_negative_pics;
        st.NumPositivePics = st.num_positive_pics;
        // See ITU-T Rec. H.265, 7.4.8 (7-65, 7-66).
        st.UsedByCurrPicS0 = st.used_by_curr_pic_s0_flag;
        st.UsedByCurrPicS1 = st.used_by_curr_pic_s1_flag;
        // See ITU-T Rec. H.265, 7.4.8 (7-67 to 7-70).
        st.DeltaPocS0.resize(st.num_negative_pics);
        if (st.num_negative_pics > 0) {
            st.DeltaPocS0[0] = -int32_t(st.delta_poc_s0_minus1[0]) - 1;
        }
        for (uint32_t i = 1; i < st.num_negative_pics; ++i) {
            st.DeltaPocS0[i] = st.DeltaPocS0[i-1] - int32_t(st.delta_poc_s0_minus1[i]) - 1;
        }
        st.DeltaPocS1.resize(st.num_positive_pics);
        if (st.num_positive_pics > 0) {
            st.DeltaPocS1[0] = st.delta_poc_s1_minus1[0] + 1;
        }
        for (uint32_t i = 1; i < st.num_positive_pics; ++i) {
            st.DeltaPocS1[i] = st.DeltaPocS1[i-1] + st.delta_poc_s1_minus1[i] + 1;
        }
    }

    // See ITU-T Rec. H.265, 7.4.8 (7-71).
    st.NumDeltaPocs = st.NumNegativePics + st.NumPositivePics;

    HEVC_TRACE(u"st.NumDeltaPocs=%d, st.NumNegativePics=%d, st.NumPositivePics=%d", st.NumDeltaPocs, st.NumNegativePics, st.NumPositivePics);
    HEVC_TRACE(u"st.UsedByCurrPicS0=(%s)", UString::Decimal(st.UsedByCurrPicS0));
    HEVC_TRACE(u"st.UsedByCurrPicS1=(%s)", UString::Decimal(st.UsedByCurrPicS1));
    HEVC_TRACE(u"st.DeltaPocS0=(%s)", UString::Decimal(st.DeltaPocS0));
    HEVC_TRACE(u"st.DeltaPocS1=(%s)", UString::Decimal(st.DeltaPocS1));
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

#define DISP(n)      out << margin << "[" << stRpsIdx << "]." #n " = " << int64_t(st.n) << std::endl
#define DISPsub(n,i) out << margin << "[" << stRpsIdx << "]." #n "[" << i << "] = " << int64_t(st.n[i]) << std::endl

                if (stRpsIdx != 0) {
                    DISP(inter_ref_pic_set_prediction_flag);
                }
                if (st.inter_ref_pic_set_prediction_flag) {
                    if (stRpsIdx == num_short_term_ref_pic_sets()) {
                        DISP(delta_idx_minus1);
                    }
                    DISP(delta_rps_sign);
                    DISP(abs_delta_rps_minus1);
                    for (uint32_t j = 0; st.valid && j < st.used_by_curr_pic_flag.size(); j++) {
                        DISPsub(used_by_curr_pic_flag, j);
                        if (!st.used_by_curr_pic_flag[j]) {
                            DISPsub(use_delta_flag, j);
                        }
                    }
                }
                else if (st.valid) {
                    DISP(num_negative_pics);
                    DISP(num_positive_pics);
                    for (uint32_t i = 0; st.valid && i < st.num_negative_pics; i++) {
                        DISPsub(delta_poc_s0_minus1, i);
                        DISPsub(used_by_curr_pic_s0_flag, i);
                    }
                    for (uint32_t i = 0; st.valid && i < st.num_positive_pics; i++) {
                        DISPsub(delta_poc_s1_minus1, i);
                        DISPsub(used_by_curr_pic_s1_flag, i);
                    }
                }
#undef DISPsub
#undef DISP
            }
        }
    }
    return out;
}
