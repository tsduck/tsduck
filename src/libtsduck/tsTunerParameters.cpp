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
//  Abstract base class for DVB tuners parameters
//
//----------------------------------------------------------------------------

#include "tsTunerParameters.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBT.h"
#include "tsTunerParametersATSC.h"
#include "tsTunerUtils.h"
#include "tsTunerArgs.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Allocate ("new") a TunerParameters of the appropriate subclass,
// depending on the tuner type. The parameters have their default values.
//----------------------------------------------------------------------------

ts::TunerParameters* ts::TunerParameters::Factory(TunerType tuner_type)
{
    switch (tuner_type) {
        case DVB_S: return new TunerParametersDVBS();
        case DVB_C: return new TunerParametersDVBC();
        case DVB_T: return new TunerParametersDVBT();
        case ATSC:  return new TunerParametersATSC();
        default:    assert(false); return nullptr;
    }
}


//----------------------------------------------------------------------------
// Extract options from a TunerArgs, applying defaults when necessary.
//----------------------------------------------------------------------------

bool ts::TunerParameters::fromTunerArgs (const TunerArgs& tuner, Report& report)
{
    if (tuner.channel_name.set()) {
        // Use --channel-transponder option
        // Get szap/czap/tzap configuration file name.
        UString file;
        if (tuner.zap_file_name.set()) {
            file = tuner.zap_file_name.value();
        }
        else if ((file = TunerArgs::DefaultZapFile(_tuner_type)).empty()) {
            report.error(u"--channel-transponder unsupported for frontend type " + TunerTypeEnum.name(_tuner_type));
            return false;
        }
        // Read tuning info from zap file
        return GetTunerFromZapFile(tuner.channel_name.value(), file, *this, report);
    }
    else if (!tuner.zap_specification.set()) {
        // No --tune specified, invoke subclass for individual tuning options
        return fromArgs(tuner, report);
    }
    else if (fromZapFormat(tuner.zap_specification.value())) {
        return true;
    }
    else {
        report.error(u"invalid --tune specification");
        return false;
    }
}


//----------------------------------------------------------------------------
// This protected method computes the theoretical useful bitrate of a
// transponder, based on 188-bytes packets, for QPSK or QAM modulation.
//----------------------------------------------------------------------------

ts::BitRate ts::TunerParameters::TheoreticalBitrateForModulation(Modulation modulation, InnerFEC fec, uint32_t symbol_rate)
{
    const uint64_t bitpersym = BitsPerSymbol(modulation);
    const uint64_t fec_mul = FECMultiplier(fec);
    const uint64_t fec_div = FECDivider(fec);

    // Compute bitrate. The estimated bitrate is based on 204-bit packets
    // (include 16-bit Reed-Solomon code). We return a bitrate based on
    // 188-bit packets.

    return fec_div == 0 ? 0 : BitRate((uint64_t(symbol_rate) * bitpersym * fec_mul * 188) / (fec_div * 204));
}
