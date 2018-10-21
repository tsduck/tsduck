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
//  DVB-T (terrestrial, OFDM) tuners parameters
//
//----------------------------------------------------------------------------

#include "tsTunerParametersDVBT.h"
#include "tsTunerArgs.h"
#include "tsEnumeration.h"
TSDUCK_SOURCE;

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
    TunerParameters(DVB_T),
    frequency(0),
    inversion(DEFAULT_INVERSION),
    bandwidth(DEFAULT_BANDWIDTH),
    fec_hp(DEFAULT_FEC_HP),
    fec_lp(DEFAULT_FEC_LP),
    modulation(DEFAULT_MODULATION),
    transmission_mode(DEFAULT_TRANSMISSION_MODE),
    guard_interval(DEFAULT_GUARD_INTERVAL),
    hierarchy(DEFAULT_HIERARCHY),
    plp(DEFAULT_PLP)
{
}


//----------------------------------------------------------------------------
// Virtual assignment
//----------------------------------------------------------------------------

void ts::TunerParametersDVBT::copy(const TunerParameters& obj)
{
    const TunerParametersDVBT* other = dynamic_cast <const TunerParametersDVBT*> (&obj);
    if (other == nullptr) {
        throw IncompatibleTunerParametersError(u"DVBT != " + TunerTypeEnum.name(obj.tunerType()));
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
        this->plp = other->plp;
    }
}


//----------------------------------------------------------------------------
// Values as encoded in zap format
//----------------------------------------------------------------------------

namespace {

    const ts::Enumeration ZapModulationEnum({
         {u"QPSK",     ts::QPSK},
         {u"QAM_AUTO", ts::QAM_AUTO},
         {u"QAM_16",   ts::QAM_16},
         {u"QAM_32",   ts::QAM_32},
         {u"QAM_64",   ts::QAM_64},
         {u"QAM_128",  ts::QAM_128},
         {u"QAM_256",  ts::QAM_256},
    });

    const ts::Enumeration ZapSpectralInversionEnum({
         {u"INVERSION_OFF",  ts::SPINV_OFF},
         {u"INVERSION_ON",   ts::SPINV_ON},
         {u"INVERSION_AUTO", ts::SPINV_AUTO},
    });

    const ts::Enumeration ZapInnerFECEnum({
         {u"FEC_NONE", ts::FEC_NONE},
         {u"FEC_AUTO", ts::FEC_AUTO},
         {u"FEC_1_2",  ts::FEC_1_2},
         {u"FEC_2_3",  ts::FEC_2_3},
         {u"FEC_3_4",  ts::FEC_3_4},
         {u"FEC_4_5",  ts::FEC_4_5},
         {u"FEC_5_6",  ts::FEC_5_6},
         {u"FEC_6_7",  ts::FEC_6_7},
         {u"FEC_7_8",  ts::FEC_7_8},
         {u"FEC_8_9",  ts::FEC_8_9},
    });

    const ts::Enumeration ZapBandWidthEnum({
         {u"BANDWIDTH_AUTO",  ts::BW_AUTO},
         {u"BANDWIDTH_5_MHZ", ts::BW_5_MHZ},
         {u"BANDWIDTH_6_MHZ", ts::BW_6_MHZ},
         {u"BANDWIDTH_7_MHZ", ts::BW_7_MHZ},
         {u"BANDWIDTH_8_MHZ", ts::BW_8_MHZ},
         {u"BANDWIDTH_10_MHZ", ts::BW_10_MHZ},
    });

    const ts::Enumeration ZapTransmissionModeEnum({
         {u"TRANSMISSION_MODE_AUTO", ts::TM_AUTO},
         {u"TRANSMISSION_MODE_2K",   ts::TM_2K},
         {u"TRANSMISSION_MODE_8K",   ts::TM_8K},
    });

    const ts::Enumeration ZapGuardIntervalEnum({
         {u"GUARD_INTERVAL_AUTO", ts::GUARD_AUTO},
         {u"GUARD_INTERVAL_1_32", ts::GUARD_1_32},
         {u"GUARD_INTERVAL_1_16", ts::GUARD_1_16},
         {u"GUARD_INTERVAL_1_8",  ts::GUARD_1_8},
         {u"GUARD_INTERVAL_1_4",  ts::GUARD_1_4},
    });

    const ts::Enumeration ZapHierarchyEnum({
         {u"HIERARCHY_AUTO", ts::HIERARCHY_AUTO},
         {u"HIERARCHY_NONE", ts::HIERARCHY_NONE},
         {u"HIERARCHY_1",    ts::HIERARCHY_1},
         {u"HIERARCHY_2",    ts::HIERARCHY_2},
         {u"HIERARCHY_4",    ts::HIERARCHY_4},
    });
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

ts::UString ts::TunerParametersDVBT::toZapFormat() const
{
    return UString::Decimal(frequency) + u":" +
        ZapSpectralInversionEnum.name(inversion) + u":" +
        ZapBandWidthEnum.name(bandwidth) + u":" +
        ZapInnerFECEnum.name(fec_hp) + u":" +
        ZapInnerFECEnum.name(fec_lp) + u":" +
        ZapModulationEnum.name(modulation) + u":" +
        ZapTransmissionModeEnum.name(transmission_mode) + u":" +
        ZapGuardIntervalEnum.name(guard_interval) + u":" +
        ZapHierarchyEnum.name(hierarchy);
}


//----------------------------------------------------------------------------
// Decode a Linux DVB "zap" specification and set the corresponding values
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBT::fromZapFormat(const UString& zap)
{
    UStringVector values;
    zap.split(values, u':', true);

    uint64_t freq = 0;
    int inv = 0, bw = 0, hp = 0, lp = 0, mod = 0, trans = 0, guard = 0, hier = 0;

    if (values.size() != 9 ||
        !values[0].toInteger(freq) ||
        (inv = ZapSpectralInversionEnum.value(values[1])) == Enumeration::UNKNOWN ||
        (bw = ZapBandWidthEnum.value(values[2])) == Enumeration::UNKNOWN ||
        (hp = ZapInnerFECEnum.value(values[3])) == Enumeration::UNKNOWN ||
        (lp = ZapInnerFECEnum.value(values[4])) == Enumeration::UNKNOWN ||
        (mod = ZapModulationEnum.value(values[5])) == Enumeration::UNKNOWN ||
        (trans = ZapTransmissionModeEnum.value(values[6])) == Enumeration::UNKNOWN ||
        (guard = ZapGuardIntervalEnum.value(values[7])) == Enumeration::UNKNOWN ||
        (hier = ZapHierarchyEnum.value(values[8])) == Enumeration::UNKNOWN)
    {
        return false;
    }

    frequency = freq;
    inversion = SpectralInversion(inv);
    bandwidth = BandWidth(bw);
    fec_hp = InnerFEC(hp);
    fec_lp = InnerFEC(lp);
    modulation = Modulation(mod);
    transmission_mode = TransmissionMode(trans);
    guard_interval = GuardInterval(guard);
    hierarchy = Hierarchy(hier);
    plp = PLP_DISABLE;

    return true;
}


//----------------------------------------------------------------------------
// Format the tuner parameters as a list of options for the dvb tsp plugin.
//----------------------------------------------------------------------------

ts::UString ts::TunerParametersDVBT::toPluginOptions(bool no_local) const
{
    UString opt(UString::Format(
        u"--frequency %'d"
        u" --spectral-inversion %s"
        u" --modulation %s"
        u" --high-priority-fec %s"
        u" --low-priority-fec %s"
        u" --bandwidth %s"
        u" --transmission-mode %s"
        u" --guard-interval %s"
        u" --hierarchy %s", {
        frequency,
        SpectralInversionEnum.name(inversion),
        ModulationEnum.name(modulation),
        InnerFECEnum.name(fec_hp),
        InnerFECEnum.name(fec_lp),
        BandWidthEnum.name(bandwidth),
        TransmissionModeEnum.name(transmission_mode),
        GuardIntervalEnum.name(guard_interval),
        HierarchyEnum.name(hierarchy)}));

    if (plp != PLP_DISABLE) {
        opt += UString::Format(u" --plp %d", {plp});
    }
    return opt;
}


//----------------------------------------------------------------------------
// Format a short description (frequency and essential parameters).
//----------------------------------------------------------------------------

ts::UString ts::TunerParametersDVBT::shortDescription(int strength, int quality) const
{
    UString desc;
    const UChar* band = nullptr;
    int channel = 0;
    int offset = 0;

    if (UHF::InBand(frequency)) {
        band = u"UHF";
        channel = UHF::Channel(frequency);
        offset = UHF::OffsetCount(frequency);
    }
    else if (VHF::InBand(frequency)) {
        band = u"VHF";
        channel = VHF::Channel(frequency);
        offset = VHF::OffsetCount(frequency);
    }

    if (band != nullptr) {
        desc += UString::Format(u"%s channel %d", {band, channel});
        if (offset != 0) {
            desc += UString::Format(u", offset %+d", {offset});
        }
        desc += u" (";
    }
    desc += UString::Decimal(frequency) + u" Hz";
    if (band != nullptr) {
        desc += u")";
    }

    if (plp != PLP_DISABLE) {
        desc += UString::Format(u", PLP %d", {plp});
    }
    if (strength >= 0) {
        desc += UString::Format(u", strength: %d%%", {strength});
    }
    if (quality >= 0) {
        desc += UString::Format(u", quality: %d%%", {quality});
    }

    return desc;
}


//----------------------------------------------------------------------------
// Display a description of the modulation paramters on a stream, line by line.
//----------------------------------------------------------------------------

void ts::TunerParametersDVBT::displayParameters(std::ostream& strm, const UString& margin, bool verbose) const
{
    if (frequency != 0) {
        strm << margin << "Carrier frequency: " << UString::Decimal(frequency) << " Hz" << std::endl;
    }
    if (inversion != SPINV_AUTO) {
        strm << margin << "Spectral inversion: " << SpectralInversionEnum.name(inversion) << std::endl;
    }
    if (modulation != QAM_AUTO) {
        strm << margin << "Constellation: " << ModulationEnum.name(modulation) << std::endl;
    }
    if (fec_hp != FEC_AUTO) {
        strm << margin << "HP streams FEC: " << InnerFECEnum.name(fec_hp) << std::endl;
    }
    if (fec_lp != FEC_AUTO) {
        strm << margin << "LP streams FEC: " << InnerFECEnum.name(fec_lp) << std::endl;
    }
    if (guard_interval != GUARD_AUTO) {
        strm << margin << "Guard interval: " << GuardIntervalEnum.name(guard_interval) << std::endl;
    }
    if (bandwidth != BW_AUTO) {
        strm << margin << "Bandwidth: " << BandWidthEnum.name(bandwidth) << std::endl;
    }
    if (transmission_mode != TM_AUTO) {
        strm << margin << "Transmission mode: " << TransmissionModeEnum.name(transmission_mode) << std::endl;
    }
    if (hierarchy != HIERARCHY_AUTO) {
        strm << margin << "Hierarchy: " << HierarchyEnum.name(hierarchy) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Extract options from a TunerArgs, applying defaults when necessary.
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBT::fromArgs(const TunerArgs& tuner, Report& report)
{
    if (!tuner.frequency.set()) {
        report.error(u"no frequency specified, use option --frequency");
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
    plp = tuner.plp.set() ? tuner.plp.value() : DEFAULT_PLP;

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
    const uint64_t bitpersym = BitsPerSymbol(modulation);
    const uint64_t fec_mul = FECMultiplier(fec_hp);
    const uint64_t fec_div = FECDivider(fec_hp);
    const uint64_t guard_mul = GuardIntervalMultiplier(guard_interval);
    const uint64_t guard_div = GuardIntervalDivider(guard_interval);
    const uint64_t bw = BandWidthValueHz(bandwidth);

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
    // And 1137024 / 1462272 = 423 / 544

    return BitRate((423 * guard_div * bw * bitpersym * fec_mul) / (544 * (guard_div + guard_mul) * fec_div));
}
