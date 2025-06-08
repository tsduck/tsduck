//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Maciej Czyzkowski
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Get DtDvbT2Pars based on input bitrate
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDektec.h"
#include "tsBitRate.h"

namespace ts {
    void EvaluateDvbT2ParsForBitrate(Dtapi::DtDvbT2Pars& params, const ts::BitRate& bitrate);
}
