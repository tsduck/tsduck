//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  DVB-T (terrestrial, OFDM) tuners parameters
//
//----------------------------------------------------------------------------

#include "tsTunerParametersDVBT.h"
#include "tsEnumeration.h"
#include "tsDecimal.h"
#include "tsFormat.h"


#if defined (TS_NEED_STATIC_CONST_DEFINITIONS)
const ts::SpectralInversion ts::TunerParametersDVBT::DEFAULT_INVERSION;
const ts::BandWidth ts::TunerParametersDVBT::DEFAULT_BANDWIDTH;
const ts::InnerFEC ts::TunerParametersDVBT::DEFAULT_FEC_HP;
const ts::InnerFEC ts::TunerParametersDVBT::DEFAULT_FEC_LP;
const ts::Modulation ts::TunerParametersDVBT::DEFAULT_MODULATION;
const ts::TransmissionMode ts::TunerParametersDVBT::DEFAULT_TRANSMISSION_MODE;
const ts::GuardInterval ts::TunerParametersDVBT::DEFAULT_GUARD_INTERVAL;
const ts::Hierarchy ts::TunerParametersDVBT::DEFAULT_HIERARCHY;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TunerParametersDVBT::TunerParametersDVBT() :
    TunerParameters (DVB_T),
    frequency (0),
    inversion (DEFAULT_INVERSION),
    bandwidth (DEFAULT_BANDWIDTH),
    fec_hp (DEFAULT_FEC_HP),
    fec_lp (DEFAULT_FEC_LP),
    modulation (DEFAULT_MODULATION),
    transmission_mode (DEFAULT_TRANSMISSION_MODE),
    guard_interval (DEFAULT_GUARD_INTERVAL),
    hierarchy (DEFAULT_HIERARCHY)
{
}


//----------------------------------------------------------------------------
// Virtual assignment
//----------------------------------------------------------------------------

void ts::TunerParametersDVBT::copy (const TunerParameters& obj) throw (IncompatibleTunerParametersError)
{
    const TunerParametersDVBT* other = dynamic_cast <const TunerParametersDVBT*> (&obj);
    if (other == 0) {
        throw IncompatibleTunerParametersError ("DVBT != " + TunerTypeEnum.name (obj.tunerType()));
    }
    else {
        this->frequency = other->frequency;
        this->inversion = other->inversion;
        this->bandwidth = other->bandwidth;
        this->fec_hp = other->fec_hp;
        this->fec_lp = other->fec_lp;
        this->modulation = other->modulation;
        this->transmission_mode = other->transmission_mode;
        this->guard_interval = other->guard_interval;
        this->hierarchy = other->hierarchy;
    }
}


//----------------------------------------------------------------------------
// Values as encoded in zap format
//----------------------------------------------------------------------------

namespace {

    const ts::Enumeration ZapModulationEnum
        ("QPSK",     ts::QPSK,
         "QAM_AUTO", ts::QAM_AUTO,
         "QAM_16",   ts::QAM_16,
         "QAM_32",   ts::QAM_32,
         "QAM_64",   ts::QAM_64,
         "QAM_128",  ts::QAM_128,
         "QAM_256",  ts::QAM_256,
         TS_NULL);

    const ts::Enumeration ZapSpectralInversionEnum
        ("INVERSION_OFF",  ts::SPINV_OFF,
         "INVERSION_ON",   ts::SPINV_ON,
         "INVERSION_AUTO", ts::SPINV_AUTO,
         TS_NULL);

    const ts::Enumeration ZapInnerFECEnum
        ("FEC_NONE", ts::FEC_NONE,
         "FEC_AUTO", ts::FEC_AUTO,
         "FEC_1_2",  ts::FEC_1_2,
         "FEC_2_3",  ts::FEC_2_3,
         "FEC_3_4",  ts::FEC_3_4,
         "FEC_4_5",  ts::FEC_4_5,
         "FEC_5_6",  ts::FEC_5_6,
         "FEC_6_7",  ts::FEC_6_7,
         "FEC_7_8",  ts::FEC_7_8,
         "FEC_8_9",  ts::FEC_8_9,
         TS_NULL);

    const ts::Enumeration ZapBandWidthEnum
        ("BANDWIDTH_AUTO",  ts::BW_AUTO,
         "BANDWIDTH_8_MHZ", ts::BW_8_MHZ,
         "BANDWIDTH_7_MHZ", ts::BW_7_MHZ,
         "BANDWIDTH_6_MHZ", ts::BW_6_MHZ,
         TS_NULL);

    const ts::Enumeration ZapTransmissionModeEnum
        ("TRANSMISSION_MODE_AUTO", ts::TM_AUTO,
         "TRANSMISSION_MODE_2K",   ts::TM_2K,
         "TRANSMISSION_MODE_8K",   ts::TM_8K,
         TS_NULL);

    const ts::Enumeration ZapGuardIntervalEnum
        ("GUARD_INTERVAL_AUTO", ts::GUARD_AUTO,
         "GUARD_INTERVAL_1_32", ts::GUARD_1_32,
         "GUARD_INTERVAL_1_16", ts::GUARD_1_16,
         "GUARD_INTERVAL_1_8",  ts::GUARD_1_8,
         "GUARD_INTERVAL_1_4",  ts::GUARD_1_4,
         TS_NULL);

    const ts::Enumeration ZapHierarchyEnum
        ("HIERARCHY_AUTO", ts::HIERARCHY_AUTO,
         "HIERARCHY_NONE", ts::HIERARCHY_NONE,
         "HIERARCHY_1",    ts::HIERARCHY_1,
         "HIERARCHY_2",    ts::HIERARCHY_2,
         "HIERARCHY_4",    ts::HIERARCHY_4,
         TS_NULL);
}


//----------------------------------------------------------------------------
// Format the tuner parameters according to the Linux DVB "zap" format
// Expected format: "freq:inv:bw:convhp:convlp:modu:mode:guard:hier"
//    With freq = frequency in Hz, inv = inversion (one of INVERSION_OFF,
//    INVERSION_ON, INVERSION_AUTO), bw = bandwidth (one of BANDWIDTH_8_MHZ,
//    BANDWIDTH_7_MHZ, BANDWIDTH_6_MHZ, BANDWIDTH_AUTO), convhp and convlp =
//    convolutional rate for high and low priority (see values in cable),
//    modu = modulation (see values in cable), mode = transmission mode
//    (one of TRANSMISSION_MODE_2K, TRANSMISSION_MODE_8K,
//    TRANSMISSION_MODE_AUTO), guard = guard interval (one of
//    GUARD_INTERVAL_1_32, GUARD_INTERVAL_1_16, GUARD_INTERVAL_1_8,
//    GUARD_INTERVAL_1_4, GUARD_INTERVAL_AUTO), hier = hierarchy (one of
//    HIERARCHY_NONE, HIERARCHY_1, HIERARCHY_2, HIERARCHY_4, HIERARCHY_AUTO).
//----------------------------------------------------------------------------

std::string ts::TunerParametersDVBT::toZapFormat() const
{
    return Format ("%" FMT_INT64 "u:", frequency) +
        ZapSpectralInversionEnum.name (inversion) + ":" +
        ZapBandWidthEnum.name (bandwidth) + ":" +
        ZapInnerFECEnum.name (fec_hp) + ":" +
        ZapInnerFECEnum.name (fec_lp) + ":" +
        ZapModulationEnum.name (modulation) + ":" +
        ZapTransmissionModeEnum.name (transmission_mode) + ":" +
        ZapGuardIntervalEnum.name (guard_interval) + ":" +
        ZapHierarchyEnum.name (hierarchy);
}


//----------------------------------------------------------------------------
// Decode a Linux DVB "zap" specification and set the corresponding values
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBT::fromZapFormat (const std::string& zap)
{
    StringVector values;
    SplitString (values, zap, ':', true);

    uint64_t freq;
    int inv, bw, hp, lp, mod, trans, guard, hier;

    if (values.size() != 9 ||
        !ToInteger (freq, values[0]) ||
        (inv = ZapSpectralInversionEnum.value (values[1])) == Enumeration::UNKNOWN ||
        (bw = ZapBandWidthEnum.value (values[2])) == Enumeration::UNKNOWN ||
        (hp = ZapInnerFECEnum.value (values[3])) == Enumeration::UNKNOWN ||
        (lp = ZapInnerFECEnum.value (values[4])) == Enumeration::UNKNOWN ||
        (mod = ZapModulationEnum.value (values[5])) == Enumeration::UNKNOWN ||
        (trans = ZapTransmissionModeEnum.value (values[6])) == Enumeration::UNKNOWN ||
        (guard = ZapGuardIntervalEnum.value (values[7])) == Enumeration::UNKNOWN ||
        (hier = ZapHierarchyEnum.value (values[8])) == Enumeration::UNKNOWN) {
        return false;
    }

    frequency = freq;
    inversion = SpectralInversion (inv);
    bandwidth = BandWidth (bw);
    fec_hp = InnerFEC (hp);
    fec_lp = InnerFEC (lp);
    modulation = Modulation (mod);
    transmission_mode = TransmissionMode (trans);
    guard_interval = GuardInterval (guard);
    hierarchy = Hierarchy (hier);

    return true;
}


//----------------------------------------------------------------------------
// Format the tuner parameters as a list of options for the dvb tsp plugin.
//----------------------------------------------------------------------------

std::string ts::TunerParametersDVBT::toPluginOptions (bool no_local) const
{
    return Format ("--frequency %" FMT_INT64 "u", frequency) +
        " --spectral-inversion " + SpectralInversionEnum.name (inversion) +
        " --modulation " + ModulationEnum.name (modulation) +
        " --high-priority-fec " + InnerFECEnum.name (fec_hp) +
        " --low-priority-fec " + InnerFECEnum.name (fec_lp) +
        " --bandwidth " + BandWidthEnum.name (bandwidth) +
        " --transmission-mode " + TransmissionModeEnum.name (transmission_mode) +
        " --guard-interval " + GuardIntervalEnum.name (guard_interval) +
        " --hierarchy " + HierarchyEnum.name (hierarchy);
}


//----------------------------------------------------------------------------
// Extract options from a TunerArgs, applying defaults when necessary.
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBT::fromArgs (const TunerArgs& tuner, ReportInterface& report)
{
    if (!tuner.frequency.set()) {
        report.error ("no frequency specified, use option --frequency");
        return false;
    }

    frequency = tuner.frequency.value();
    inversion = tuner.inversion.set() ? tuner.inversion.value() : DEFAULT_INVERSION;
    modulation = tuner.modulation.set() ? tuner.modulation.value() : DEFAULT_MODULATION;
    bandwidth = tuner.bandwidth.set() ? tuner.bandwidth.value() : DEFAULT_BANDWIDTH;
    fec_hp = tuner.fec_hp.set() ? tuner.fec_hp.value() : DEFAULT_FEC_HP;
    fec_lp = tuner.fec_lp.set() ? tuner.fec_lp.value() : DEFAULT_FEC_LP;
    transmission_mode = tuner.transmission_mode.set() ? tuner.transmission_mode.value() : DEFAULT_TRANSMISSION_MODE;
    guard_interval = tuner.guard_interval.set() ? tuner.guard_interval.value() : DEFAULT_GUARD_INTERVAL;
    hierarchy = tuner.hierarchy.set() ? tuner.hierarchy.value() : DEFAULT_HIERARCHY;

    return true;
}


//----------------------------------------------------------------------------
// This abstract method computes the theoretical useful bitrate of a
// transponder, based on 188-bytes packets, in bits/second.
// If the characteristics of the transponder are not sufficient
// to compute the bitrate, return 0.
//----------------------------------------------------------------------------

ts::BitRate ts::TunerParametersDVBT::theoreticalBitrate() const
{
    const uint64_t bitpersym = BitsPerSymbol (modulation);
    const uint64_t fec_mul = FECMultiplier (fec_hp);
    const uint64_t fec_div = FECDivider (fec_hp);
    const uint64_t guard_mul = GuardIntervalMultiplier (guard_interval);
    const uint64_t guard_div = GuardIntervalDivider (guard_interval);
    const uint64_t bw = BandWidthValueHz (bandwidth);

    if (hierarchy != HIERARCHY_NONE || fec_div == 0 || guard_div == 0) {
        return 0; // unknown bitrate
    }

    // Compute symbol rate, then bitrate
    //
    // How did we get that? Long story...
    // Reference: ETSI EN 300 744 V1.5.1
    // (DVB; Framing structure, channel coding and modulation
    // for digital terrestrial television).
    // 
    //  BW = bandwidth in Hz
    //  BM = bandwidth in MHz = BW / 1000000
    //  TM = transmission mode in K
    //  GI = guard interval = GIM/GID
    //  T  = OFDM elementary period = 7 / (8*BM) micro-seconds
    //  TU = useful symbol duration = TM * 1024 * T
    //  TG = guard duration = TU * GI
    //  TS = symbol duration = TG + TU = TU * (1 + GI) = (TU * (GID + GIM)) / GID
    //  K  = number of _active_ carriers = TM * 756
    //  SR = symbol rate
    //     = K / TS  symbols/micro-second
    //     = 1000000 * K / TS  symbols/second
    //     = (1000000 * TM * 756 * GID) / (TU * (GID + GIM))
    //     = (1000000 * TM * 756 * GID) / (TM * 1024 * T * (GID + GIM))
    //     = (1000000 * 756 * GID) / (1024 * T * (GID + GIM))
    //     = (1000000 * 756 * GID * 8 * BM) / (1024 * 7 * (GID + GIM))
    //     = (6048 * GID * BW) / (7168 * (GID + GIM))
    //
    // Compute bitrate. The estimated bitrate is based on 204-bit packets
    // (include 16-bit Reed-Solomon code). We return a bitrate based on
    // 188-bit packets.
    //
    // BPS = bits/symbol
    // FEC = forward error correction = FECM/FECD
    // BR = useful bit rate
    //    = SR * BPS * FEC * 188/204
    //    = (SR * BPS * FECM * 188) / (FECD * 204)
    //    = (6048 * GID * BW * BPS * FECM * 188) / (7168 * (GID + GIM) * FECD * 204)
    //    = (1137024 * GID * BW * BPS * FECM) / (1462272 * (GID + GIM) * FECD)

    return BitRate ((1137024 * guard_div * bw * bitpersym * fec_mul) / (1462272 * (guard_div + guard_mul) * fec_div));
}
