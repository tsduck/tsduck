//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Maciej Czyzkowski
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBT2ParamsEvaluator.h"

namespace {
    template <typename V, typename... T>
    constexpr auto array_of(T&&... t) -> std::array < V, sizeof...(T) >
    {
        return {{ std::forward<T>(t)... }};
    }

    // m_FftMode
    const auto pref_fft_mode = array_of<int>(DTAPI_DVBT2_FFT_1K, DTAPI_DVBT2_FFT_2K, DTAPI_DVBT2_FFT_4K,
                                             DTAPI_DVBT2_FFT_8K, DTAPI_DVBT2_FFT_16K, DTAPI_DVBT2_FFT_32K);
    // m_GuardInterval
    const auto pref_guard_interval = array_of<int>(DTAPI_DVBT2_GI_1_128, DTAPI_DVBT2_GI_1_32,
                                                   DTAPI_DVBT2_GI_1_16, DTAPI_DVBT2_GI_19_256,
                                                   DTAPI_DVBT2_GI_1_8, DTAPI_DVBT2_GI_19_128,
                                                   DTAPI_DVBT2_GI_1_4);
    // m_L1Modulation
    const auto pref_l1_modulation = array_of<int>(DTAPI_DVBT2_BPSK, DTAPI_DVBT2_QPSK, DTAPI_DVBT2_QAM16,
                                                  DTAPI_DVBT2_QAM64, DTAPI_DVBT2_QAM256);
    // m_Plps[0].m_CodeRate
    const auto pref_code_rate = array_of<int>(DTAPI_DVBT2_COD_1_2, DTAPI_DVBT2_COD_3_5, DTAPI_DVBT2_COD_2_3,
                                              DTAPI_DVBT2_COD_3_4, DTAPI_DVBT2_COD_4_5, DTAPI_DVBT2_COD_5_6);
    // m_Plps[0].m_Modulation
    const auto pref_plp0_modulation = array_of<int>(DTAPI_DVBT2_BPSK, DTAPI_DVBT2_QPSK, DTAPI_DVBT2_QAM16,
                                                    DTAPI_DVBT2_QAM64, DTAPI_DVBT2_QAM256);
}


//----------------------------------------------------------------------------
// Build a list of all possible combinations of modulation parameters
//----------------------------------------------------------------------------

void ts::EvaluateDvbT2ParsForBitrate(Dtapi::DtDvbT2Pars& pars, const ts::BitRate& bitrate)
{
    Dtapi::DtDvbT2Pars best_params(pars);
    ts::BitRate best_bitrate = 0;
    // Initialize to some high value
    ts::BitRate best_bitrate_diff = bitrate;

    // Build a list of all possible modulation parameters for this bitrate.
    for (auto fft_mode : pref_fft_mode) {
        Dtapi::DtDvbT2Pars params = pars;
        params.m_FftMode = fft_mode;
        for (auto guard_interval : pref_guard_interval) {
            params.m_GuardInterval = guard_interval;
            for (auto l1_modulation : pref_l1_modulation) {
                params.m_L1Modulation = l1_modulation;
                for (auto code_rate : pref_code_rate) {
                    params.m_Plps[0].m_CodeRate = code_rate;
                    for (auto modulation : pref_plp0_modulation) {
                        params.m_Plps[0].m_Modulation = modulation;
                        Dtapi::DtDvbT2ParamInfo info;
                        params.OptimisePlpNumBlocks(info, params.m_Plps[0].m_NumBlocks, params.m_NumDataSyms);
                        auto status = params.CheckValidity();
                        if (status != DTAPI_OK) {
                            continue;
                        }
                        ts::BitRate new_bitrate = 0;
                        Dtapi::DtFractionInt frate;
                        status = Dtapi::DtapiModPars2TsRate(frate, params);
                        if (status == DTAPI_OK && frate.m_Num > 0 && frate.m_Den > 0) {
                            FromDektecFractionInt(new_bitrate, frate);
                        }
                        else {
                            int irate = 0;
                            status = Dtapi::DtapiModPars2TsRate(irate, params);
                            if (status == DTAPI_OK) {
                                new_bitrate = ts::BitRate{irate};
                            }
                        }
                        auto new_bitrate_diff = (new_bitrate - bitrate).abs();
                        if (new_bitrate_diff < best_bitrate_diff) {
                            best_params = params;
                            best_bitrate = new_bitrate;
                            best_bitrate_diff = new_bitrate_diff;
                        }
                    }
                }
            }
        }
    }
    pars = std::move(best_params);
}
