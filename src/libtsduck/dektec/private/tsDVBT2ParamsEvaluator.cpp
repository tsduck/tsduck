//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Maciej Czyzkowski
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

#if defined(TS_NO_DTAPI)
#include "tsPlatform.h"
TS_LLVM_NOWARNING(missing-variable-declarations)
bool tsDVBT2ParamsEvaluatorIsEmpty = true; // Avoid warning about empty module.
#else

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
    Dtapi::DtDvbT2Pars best_params = pars;
    ts::BitRate best_bitrate = 0;
    //initialize to some high value
    auto best_bitrate_diff = bitrate;

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
    pars = best_params;
}

#endif // TS_NO_DTAPI
