//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsTunerParameters.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBT.h"
#include "tsTunerParametersATSC.h"
#include "tsTunerArgs.h"
#include "tsChannelFile.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TunerParameters::~TunerParameters()
{
}


//----------------------------------------------------------------------------
// Allocate ("new") a TunerParameters of the appropriate subclass,
// depending on the tuner type. The parameters have their default values.
//----------------------------------------------------------------------------

ts::TunerParametersPtr ts::TunerParameters::Factory(TunerType tuner_type)
{
    TunerParameters* ptr = nullptr;
    switch (tuner_type) {
        case DVB_S: ptr = new TunerParametersDVBS(); break;
        case DVB_C: ptr = new TunerParametersDVBC(); break;
        case DVB_T: ptr = new TunerParametersDVBT(); break;
        case ATSC:  ptr = new TunerParametersATSC(); break;
        default:    assert(false); break;
    }
    CheckNonNull(ptr);
    return TunerParametersPtr(ptr);
}


//----------------------------------------------------------------------------
// Allocate a TunerParameters from a delivery system descriptor.
//----------------------------------------------------------------------------

ts::TunerParametersPtr ts::TunerParameters::FromDeliveryDescriptor(const Descriptor& desc)
{
    TunerParametersPtr ptr;
    switch (desc.tag()) {
        case DID_SAT_DELIVERY:     ptr = new TunerParametersDVBS(); break;
        case DID_CABLE_DELIVERY:   ptr = new TunerParametersDVBC(); break;
        case DID_TERREST_DELIVERY: ptr = new TunerParametersDVBT(); break;
        default: return TunerParametersPtr(); // Not a known delivery descriptor
    }
    CheckNonNull(ptr.pointer());
    if (!ptr->fromDeliveryDescriptor(desc)) {
        ptr.reset();
    }
    return ptr;
}


//----------------------------------------------------------------------------
// Extract options from a TunerArgs, applying defaults when necessary.
//----------------------------------------------------------------------------

ts::TunerParametersPtr ts::TunerParameters::FromTunerArgs(TunerType type, const TunerArgs& args, Report& report)
{
    if (args.channel_name.set()) {
        // Use --channel-transponder option

        // Get tuning file name.
        UString file;
        if (args.tuning_file_name.set()) {
            file = args.tuning_file_name.value();
        }
        if (file.empty() || file == u"-") {
            file = ChannelFile::DefaultFileName();
        }

        // Load channels file.
        ChannelFile channels;
        if (!channels.load(file, report)) {
            return TunerParametersPtr();
        }

        // Retrieve tuning options.
        return channels.serviceToTuning(args.channel_name.value(), false, report);
    }
    else {
        // Allocate tuning parameters of the appropriate type
        TunerParametersPtr params(TunerParameters::Factory(type));

        // Invoke subclass for individual tuning options.
        return params->fromArgs(args, report) ? params : TunerParametersPtr();
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


//----------------------------------------------------------------------------
// Attempt to convert the tuning parameters in modulation parameters for
// Dektec modulator cards. Unimplemented by default.
//----------------------------------------------------------------------------

bool ts::TunerParameters::convertToDektecModulation(int& modulation_type, int& param0, int& param1, int& param2) const
{
    return false;
}
