//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  A variant of DVB-T tuners parameters with an offset between a target
//  bitrate and their theoretical bitrate
//
//----------------------------------------------------------------------------

#include "tsTunerParametersBitrateDiffDVBT.h"
#include "tsModulation.h"
#include "tsGuard.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TunerParametersBitrateDiffDVBT::TunerParametersBitrateDiffDVBT() :
    TunerParametersDVBT (),
    bitrate_diff (0)
{
}


//----------------------------------------------------------------------------
// Virtual assignment
//----------------------------------------------------------------------------

void ts::TunerParametersBitrateDiffDVBT::copy(const TunerParameters& obj)
{
    const TunerParametersBitrateDiffDVBT* other = dynamic_cast <const TunerParametersBitrateDiffDVBT*> (&obj);
    if (other == nullptr) {
        throw IncompatibleTunerParametersError(u"BitrateDiff DVBT != " + TunerTypeEnum.name(obj.tunerType()));
    }
    else {
        TunerParametersDVBT::copy(obj); // superclass
        this->bitrate_diff = other->bitrate_diff;
    }
}


//----------------------------------------------------------------------------
// Possible modulation parameters to consider.
// Each array is sorted in decreasing order of preference.
//----------------------------------------------------------------------------

namespace {

    const ts::BandWidth pref_bw[] = {ts::BW_8_MHZ, ts::BW_7_MHZ, ts::BW_6_MHZ, ts::BW_5_MHZ};
    const ts::Modulation pref_mod[] = {ts::QAM_64, ts::QAM_16, ts::QPSK};
    const ts::InnerFEC pref_fec[] = {ts::FEC_2_3, ts::FEC_3_4, ts::FEC_5_6, ts::FEC_7_8, ts::FEC_1_2};
    const ts::GuardInterval pref_guard[] = {ts::GUARD_1_32, ts::GUARD_1_8, ts::GUARD_1_16, ts::GUARD_1_4};

#define ARRSIZE(array) (sizeof(array) / ((const char*)((array)+1) - (const char*)(array)))
    const size_t pref_bw_size    = ARRSIZE (pref_bw);
    const size_t pref_mod_size   = ARRSIZE (pref_mod);
    const size_t pref_fec_size   = ARRSIZE (pref_fec);
    const size_t pref_guard_size = ARRSIZE (pref_guard);
#undef ARRSIZE
}

//----------------------------------------------------------------------------
// Comparison operator for list sort.
//----------------------------------------------------------------------------

bool ts::TunerParametersBitrateDiffDVBT::operator< (const TunerParametersBitrateDiffDVBT& other) const
{
    // If distance from target bitrate is different, use lowest
    if (::abs (bitrate_diff) != ::abs (other.bitrate_diff)) {
        return ::abs (bitrate_diff) < ::abs (other.bitrate_diff);
    }

    // The two sets of parameters have the same distance from the target bitrate.
    // Consider other modulation parameters to select a "better" set of parameters.
    if (bandwidth != other.bandwidth) {
        for (size_t i = 0; i < pref_bw_size; ++i) {
            if (bandwidth == pref_bw[i]) {
                return true; // this < other
            }
            if (other.bandwidth == pref_bw[i]) {
                return false; // other < this
            }
        }
    }
    if (modulation != other.modulation) {
        for (size_t i = 0; i < pref_mod_size; ++i) {
            if (modulation == pref_mod[i]) {
                return true; // this < other
            }
            if (other.modulation == pref_mod[i]) {
                return false; // other < this
            }
        }
    }
    if (fec_hp != other.fec_hp) {
        for (size_t i = 0; i < pref_fec_size; ++i) {
            if (fec_hp == pref_fec[i]) {
                return true; // this < other
            }
            if (other.fec_hp == pref_fec[i]) {
                return false; // other < this
            }
        }
    }
    if (guard_interval != other.guard_interval) {
        for (size_t i = 0; i < pref_guard_size; ++i) {
            if (guard_interval == pref_guard[i]) {
                return true; // this < other
            }
            if (other.guard_interval == pref_guard[i]) {
                return false; // other < this
            }
        }
    }

    return false; // this >= other
}


//----------------------------------------------------------------------------
// Build a list of all possible combinations of bandwidth, constellation,
// guard interval and high-priority FEC, sorted in increasing order of
// bitrate difference from a given target bitrate.
//----------------------------------------------------------------------------

void ts::TunerParametersBitrateDiffDVBT::EvaluateToBitrate (TunerParametersBitrateDiffDVBTList& list, BitRate bitrate)
{
    list.clear();
    TunerParametersBitrateDiffDVBT params;

    // Build a list of all possible modulation parameters for this bitrate.
    for (size_t i_mod = 0; i_mod < pref_mod_size; ++i_mod) {
        params.modulation = pref_mod[i_mod];
        for (size_t i_fec = 0; i_fec < pref_fec_size; ++i_fec) {
            params.fec_hp = pref_fec[i_fec];
            for (size_t i_guard = 0; i_guard < pref_guard_size; ++i_guard) {
                params.guard_interval = pref_guard[i_guard];
                for (size_t i_bw = 0; i_bw < pref_bw_size; ++i_bw) {
                    params.bandwidth = pref_bw[i_bw];
                    params.bitrate_diff = int (bitrate) - int (params.theoreticalBitrate());
                    list.push_back (params);
                }
            }
        }
    }

    // Sort the list by increasing difference with specified bitrate
    list.sort();
}
