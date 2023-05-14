//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsModulationArgs.h"
#include "tsLegacyBandWidth.h"
#include "tsDuckContext.h"
#include "tsDescriptor.h"
#include "tsHFBand.h"
#include "tsArgs.h"
#include "tsNullReport.h"
#include "tsBCD.h"
#include "tsDektec.h"
#include "tsAlgorithm.h"

const ts::UString ts::ModulationArgs::DEFAULT_ISDBT_LAYERS(u"ABC"); // all layers

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr ts::SpectralInversion ts::ModulationArgs::DEFAULT_INVERSION;
constexpr ts::InnerFEC          ts::ModulationArgs::DEFAULT_INNER_FEC;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_SYMBOL_RATE_DVBS;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_SYMBOL_RATE_DVBC;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_SYMBOL_RATE_ISDBS;
constexpr ts::Modulation        ts::ModulationArgs::DEFAULT_MODULATION_DVBS;
constexpr ts::Modulation        ts::ModulationArgs::DEFAULT_MODULATION_DVBT;
constexpr ts::Modulation        ts::ModulationArgs::DEFAULT_MODULATION_DVBC;
constexpr ts::Modulation        ts::ModulationArgs::DEFAULT_MODULATION_ATSC;
constexpr ts::BandWidth         ts::ModulationArgs::DEFAULT_BANDWIDTH_DVBT;
constexpr ts::BandWidth         ts::ModulationArgs::DEFAULT_BANDWIDTH_ISDBT;
constexpr ts::InnerFEC          ts::ModulationArgs::DEFAULT_FEC_HP;
constexpr ts::InnerFEC          ts::ModulationArgs::DEFAULT_FEC_LP;
constexpr ts::TransmissionMode  ts::ModulationArgs::DEFAULT_TRANSMISSION_MODE_DVBT;
constexpr ts::TransmissionMode  ts::ModulationArgs::DEFAULT_TRANSMISSION_MODE_ISDBT;
constexpr ts::GuardInterval     ts::ModulationArgs::DEFAULT_GUARD_INTERVAL_DVBT;
constexpr ts::GuardInterval     ts::ModulationArgs::DEFAULT_GUARD_INTERVAL_ISDBT;
constexpr ts::Hierarchy         ts::ModulationArgs::DEFAULT_HIERARCHY;
constexpr ts::Polarization      ts::ModulationArgs::DEFAULT_POLARITY;
constexpr size_t                ts::ModulationArgs::DEFAULT_SATELLITE_NUMBER;
constexpr ts::Pilot             ts::ModulationArgs::DEFAULT_PILOTS;
constexpr ts::RollOff           ts::ModulationArgs::DEFAULT_ROLL_OFF;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_PLP;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_ISI;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_PLS_CODE;
constexpr ts::PLSMode           ts::ModulationArgs::DEFAULT_PLS_MODE;
constexpr int                   ts::ModulationArgs::DEFAULT_SB_SUBCHANNEL_ID;
constexpr int                   ts::ModulationArgs::DEFAULT_SB_SEGMENT_COUNT;
constexpr int                   ts::ModulationArgs::DEFAULT_SB_SEGMENT_INDEX;
constexpr int                   ts::ModulationArgs::MAX_ISDBT_SEGMENT_COUNT;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_STREAM_ID;
#endif


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::ModulationArgs::ModulationArgs() :
    delivery_system(),
    frequency(),
    polarity(),
    lnb(),
    inversion(),
    symbol_rate(),
    inner_fec(),
    satellite_number(),
    modulation(),
    bandwidth(),
    fec_hp(),
    fec_lp(),
    transmission_mode(),
    guard_interval(),
    hierarchy(),
    pilots(),
    roll_off(),
    plp(),
    isi(),
    pls_code(),
    pls_mode(),
    sound_broadcasting(),
    sb_subchannel_id(),
    sb_segment_count(),
    sb_segment_index(),
    isdbt_layers(),
    isdbt_partial_reception(),
    layer_a_fec(),
    layer_a_modulation(),
    layer_a_segment_count(),
    layer_a_time_interleaving(),
    layer_b_fec(),
    layer_b_modulation(),
    layer_b_segment_count(),
    layer_b_time_interleaving(),
    layer_c_fec(),
    layer_c_modulation(),
    layer_c_segment_count(),
    layer_c_time_interleaving(),
    stream_id()
{
}


//----------------------------------------------------------------------------
// Reset all values, they become "unset"
//----------------------------------------------------------------------------

void ts::ModulationArgs::clear()
{
    delivery_system.clear();
    frequency.clear();
    polarity.clear();
    lnb.clear();
    inversion.clear();
    symbol_rate.clear();
    inner_fec.clear();
    satellite_number.clear();
    modulation.clear();
    bandwidth.clear();
    fec_hp.clear();
    fec_lp.clear();
    transmission_mode.clear();
    guard_interval.clear();
    hierarchy.clear();
    pilots.clear();
    roll_off.clear();
    plp.clear();
    isi.clear();
    pls_code.clear();
    pls_mode.clear();
    sound_broadcasting.clear();
    sb_subchannel_id.clear();
    sb_segment_count.clear();
    sb_segment_index.clear();
    isdbt_layers.clear();
    isdbt_partial_reception.clear();
    layer_a_fec.clear();
    layer_a_modulation.clear();
    layer_a_segment_count.clear();
    layer_a_time_interleaving.clear();
    layer_b_fec.clear();
    layer_b_modulation.clear();
    layer_b_segment_count.clear();
    layer_b_time_interleaving.clear();
    layer_c_fec.clear();
    layer_c_modulation.clear();
    layer_c_segment_count.clear();
    layer_c_time_interleaving.clear();
    stream_id.clear();
}


//----------------------------------------------------------------------------
//! Reset or copy the local reception parameters.
//----------------------------------------------------------------------------

void ts::ModulationArgs::resetLocalReceptionParameters()
{
    lnb.clear();
    satellite_number.clear();
}

void ts::ModulationArgs::copyLocalReceptionParameters(const ModulationArgs& other)
{
    if (other.lnb.set()) {
        lnb = other.lnb;
    }
    if (other.satellite_number.set()) {
        satellite_number = other.satellite_number;
    }
}


//----------------------------------------------------------------------------
// Check if any modulation options is set.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::hasModulationArgs() const
{
    return
        delivery_system.set() ||
        frequency.set() ||
        polarity.set() ||
        lnb.set() ||
        inversion.set() ||
        symbol_rate.set() ||
        inner_fec.set() ||
        satellite_number.set() ||
        modulation.set() ||
        bandwidth.set() ||
        fec_hp.set() ||
        fec_lp.set() ||
        transmission_mode.set() ||
        guard_interval.set() ||
        hierarchy.set() ||
        pilots.set() ||
        roll_off.set() ||
        plp.set() ||
        isi.set() ||
        pls_code.set() ||
        pls_mode.set() ||
        sound_broadcasting.set() ||
        sb_subchannel_id.set() ||
        sb_segment_count.set() ||
        sb_segment_index.set() ||
        isdbt_layers.set() ||
        isdbt_partial_reception.set() ||
        layer_a_fec.set() ||
        layer_a_modulation.set() ||
        layer_a_segment_count.set() ||
        layer_a_time_interleaving.set() ||
        layer_b_fec.set() ||
        layer_b_modulation.set() ||
        layer_b_segment_count.set() ||
        layer_b_time_interleaving.set() ||
        layer_c_fec.set() ||
        layer_c_modulation.set() ||
        layer_c_segment_count.set() ||
        layer_c_time_interleaving.set() ||
        stream_id.set();
}


//----------------------------------------------------------------------------
// Check if an ISDB-T time interleaving value is valid.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::IsValidISDBTTimeInterleaving(int ti)
{
    return ti == -1 || ti == 0 || ti == 1 || ti == 2 || ti == 4;
}


//----------------------------------------------------------------------------
// Set the default values for the delivery system.
//----------------------------------------------------------------------------

void ts::ModulationArgs::setDefaultValues()
{
    switch (delivery_system.value(DS_UNDEFINED)) {
        case DS_DVB_S2:
            // DVB-S2 specific options.
            pilots.setDefault(DEFAULT_PILOTS);
            roll_off.setDefault(DEFAULT_ROLL_OFF);
            isi.setDefault(DEFAULT_ISI);
            pls_code.setDefault(DEFAULT_PLS_CODE);
            pls_mode.setDefault(DEFAULT_PLS_MODE);
            TS_FALLTHROUGH
        case DS_DVB_S_TURBO:
        case DS_DVB_S:
            // DVB-S2, DVB-S/Turbo and DVB-S common options.
            modulation.setDefault(DEFAULT_MODULATION_DVBS);
            frequency.setDefault(0);
            inversion.setDefault(DEFAULT_INVERSION);
            polarity.setDefault(DEFAULT_POLARITY);
            symbol_rate.setDefault(DEFAULT_SYMBOL_RATE_DVBS);
            inner_fec.setDefault(DEFAULT_INNER_FEC);
            lnb.setDefault(LNB(u"", NULLREP));
            satellite_number.setDefault(DEFAULT_SATELLITE_NUMBER);
            break;
        case DS_DVB_T2:
            // DVB-S2 specific options.
            plp.setDefault(DEFAULT_PLP);
            TS_FALLTHROUGH
        case DS_DVB_T:
            // DVB-T2 and DVB-T common options.
            frequency.setDefault(0);
            inversion.setDefault(DEFAULT_INVERSION);
            bandwidth.setDefault(DEFAULT_BANDWIDTH_DVBT);
            fec_hp.setDefault(DEFAULT_FEC_HP);
            fec_lp.setDefault(DEFAULT_FEC_LP);
            modulation.setDefault(DEFAULT_MODULATION_DVBT);
            transmission_mode.setDefault(DEFAULT_TRANSMISSION_MODE_DVBT);
            guard_interval.setDefault(DEFAULT_GUARD_INTERVAL_DVBT);
            hierarchy.setDefault(DEFAULT_HIERARCHY);
            break;
        case DS_DVB_C_ANNEX_A:
        case DS_DVB_C_ANNEX_C:
            // DVB-C annex A,C common options (don't apply to annex B).
            inner_fec.setDefault(DEFAULT_INNER_FEC);
            symbol_rate.setDefault(DEFAULT_SYMBOL_RATE_DVBC);
            TS_FALLTHROUGH
        case DS_DVB_C_ANNEX_B:
            // DVB-C annex A,B,C common options.
            frequency.setDefault(0);
            inversion.setDefault(DEFAULT_INVERSION);
            modulation.setDefault(DEFAULT_MODULATION_DVBC);
            break;
        case DS_ATSC:
            frequency.setDefault(0);
            inversion.setDefault(DEFAULT_INVERSION);
            modulation.setDefault(DEFAULT_MODULATION_ATSC);
            break;
        case DS_ISDB_S:
            frequency.setDefault(0);
            polarity.setDefault(DEFAULT_POLARITY);
            lnb.setDefault(LNB(u"", NULLREP));
            satellite_number.setDefault(DEFAULT_SATELLITE_NUMBER);
            inversion.setDefault(DEFAULT_INVERSION);
            symbol_rate.setDefault(DEFAULT_SYMBOL_RATE_ISDBS);
            inner_fec.setDefault(DEFAULT_INNER_FEC);
            break;
        case DS_ISDB_T:
            frequency.setDefault(0);
            inversion.setDefault(DEFAULT_INVERSION);
            bandwidth.setDefault(DEFAULT_BANDWIDTH_ISDBT);
            transmission_mode.setDefault(DEFAULT_TRANSMISSION_MODE_ISDBT);
            guard_interval.setDefault(DEFAULT_GUARD_INTERVAL_ISDBT);
            sound_broadcasting.setDefault(false);
            sb_subchannel_id.setDefault(DEFAULT_SB_SUBCHANNEL_ID);
            sb_segment_count.setDefault(DEFAULT_SB_SEGMENT_COUNT);
            sb_segment_index.setDefault(DEFAULT_SB_SEGMENT_INDEX);
            isdbt_layers.setDefault(DEFAULT_ISDBT_LAYERS);
            break;
        case DS_ISDB_C:
        case DS_DVB_C2:
        case DS_DVB_H:
        case DS_ATSC_MH:
        case DS_DTMB:
        case DS_CMMB:
        case DS_DAB:
        case DS_DSS:
        case DS_UNDEFINED:
        default:
            // Unsupported so far.
            break;
    }

    // Erase unused values.
    if (delivery_system.set() && delivery_system.value() != DS_DVB_S2) {
        roll_off.clear();
    }
}


//----------------------------------------------------------------------------
// Check the validity of the delivery system or set a default one.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::resolveDeliverySystem(const DeliverySystemSet& systems, Report& report)
{
    if (delivery_system.set()) {
        if (!Contains(systems, delivery_system.value())) {
            report.error(u"delivery system %s is not supported by this tuner", {DeliverySystemEnum.name(delivery_system.value())});
            return false;
        }
        else {
            return true;
        }
    }
    else {
        // Delivery system not set, use the first one as default value.
        if (systems.empty()) {
            report.error(u"this tuner has no default delivery system");
            return false;
        }
        else {
            delivery_system = systems.preferred();
            report.debug(u"using %s as default delivery system", {DeliverySystemEnum.name(delivery_system.value())});
            return true;
        }
    }
}


//----------------------------------------------------------------------------
// This protected method computes the theoretical useful bitrate of a
// transponder, based on 188-bytes packets, for QPSK or QAM modulation.
//----------------------------------------------------------------------------

ts::BitRate ts::ModulationArgs::TheoreticalBitrateForModulation(Modulation modulation, InnerFEC fec, uint32_t symbol_rate)
{
    const uint64_t bitpersym = BitsPerSymbol(modulation);
    const uint64_t fec_mul = FECMultiplier(fec);
    const uint64_t fec_div = FECDivider(fec);

    // Compute bitrate. The estimated bitrate is based on 204-bit packets (include 16-bit Reed-Solomon code).
    // We return a bitrate based on 188-bit packets.

    return fec_div == 0 ? 0 : BitRate(symbol_rate * bitpersym * fec_mul * 188) / BitRate(fec_div * 204);
}


//----------------------------------------------------------------------------
// Theoretical bitrate computation.
//----------------------------------------------------------------------------

ts::BitRate ts::ModulationArgs::theoreticalBitrate() const
{
    BitRate bitrate = 0;
    const DeliverySystem delsys = delivery_system.value(DS_UNDEFINED);

    switch (delsys) {
        case DS_ATSC: {
            // Only two modulation values are available for ATSC.
            const Modulation mod = modulation.value(DEFAULT_MODULATION_ATSC);
            if (mod == VSB_8) {
                bitrate = 19392658;
            }
            else if (mod == VSB_16) {
                bitrate = 38785317;
            }
            break;
        }
        case DS_DVB_C_ANNEX_A:
        case DS_DVB_C_ANNEX_C: {
            // Applies only to annex A and C, not B.
            bitrate = TheoreticalBitrateForModulation(modulation.value(DEFAULT_MODULATION_DVBC), inner_fec.value(DEFAULT_INNER_FEC), symbol_rate.value(DEFAULT_SYMBOL_RATE_DVBC));
            break;
        }
        case DS_DVB_S:
        case DS_DVB_S_TURBO:
        case DS_DVB_S2: {
            const uint32_t symrate = symbol_rate.value(DEFAULT_SYMBOL_RATE_DVBS);
            // Let the Dektec API compute the TS rate if we have a Dektec library.
            #if !defined(TS_NO_DTAPI)
                int mod = 0, param0 = 0, param1 = 0, param2 = 0, irate = 0;
                Dtapi::DtFractionInt frate;
                if (convertToDektecModulation(mod, param0, param1, param2)) {
                    // Successfully found Dektec modulation parameters. Compute the bitrate in fractional form first.
                    // It has been observed that the values from the DtFractionInt are sometimes negative.
                    // This is a DTAPI bug, probably due to some internal integer overflow.
                    if (Dtapi::DtapiModPars2TsRate(frate, mod, param0, param1, param2, int(symrate)) == DTAPI_OK && frate.m_Num > 0 && frate.m_Den > 0) {
                        FromDektecFractionInt(bitrate, frate);
                    }
                    else if (Dtapi::DtapiModPars2TsRate(irate, mod, param0, param1, param2, int(symrate)) == DTAPI_OK && irate > 0) {
                        // The fractional version failed or returned a negative value. Used the int version.
                        bitrate = irate;
                    }
                }
            #endif
            // Otherwise, don't know how to compute DVB-S2 bitrate...
            if (bitrate == 0 && delsys == DS_DVB_S) {
                bitrate = TheoreticalBitrateForModulation(modulation.value(DEFAULT_MODULATION_DVBS), inner_fec.value(DEFAULT_INNER_FEC), symrate);
            }
            break;
        }
        case DS_DVB_T:
        case DS_DVB_T2: {
            // DVB-T2 and DVB-T common options.
            const uint64_t bitpersym = BitsPerSymbol(modulation.value(DEFAULT_MODULATION_DVBT));
            const uint64_t fec_mul = FECMultiplier(fec_hp.value(DEFAULT_FEC_HP));
            const uint64_t fec_div = FECDivider(fec_hp.value(DEFAULT_FEC_HP));
            const uint64_t guard_mul = GuardIntervalMultiplier(guard_interval.value(DEFAULT_GUARD_INTERVAL_DVBT));
            const uint64_t guard_div = GuardIntervalDivider(guard_interval.value(DEFAULT_GUARD_INTERVAL_DVBT));
            const uint64_t bw = bandwidth.value(DEFAULT_BANDWIDTH_DVBT);

            if (hierarchy.value(DEFAULT_HIERARCHY) != HIERARCHY_NONE || fec_div == 0 || guard_div == 0) {
                return 0; // unknown bitrate
            }

            // Compute symbol rate, then bitrate
            // Reference: ETSI EN 300 744 V1.5.1
            // (DVB; Framing structure, channel coding and modulation for digital terrestrial television).
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

            bitrate = BitRate(423 * guard_div * bw * bitpersym * fec_mul) / BitRate(544 * (guard_div + guard_mul) * fec_div);
            break;
        }
        case DS_ISDB_S: {
            // ISDB-S uses the Trellis coded 8-phase shift keying modulation.
            // For the sake of bitrate computation, this is the same as 8PSK.
            bitrate = TheoreticalBitrateForModulation(PSK_8, inner_fec.value(DEFAULT_INNER_FEC), symbol_rate.value(DEFAULT_SYMBOL_RATE_ISDBS));
            break;
        }
        case DS_ISDB_T:
        case DS_ISDB_C:
        case DS_DVB_C_ANNEX_B:
        case DS_DVB_C2:
        case DS_DVB_H:
        case DS_ATSC_MH:
        case DS_DTMB:
        case DS_CMMB:
        case DS_DAB:
        case DS_DSS:
        case DS_UNDEFINED:
        default: {
            // Unknown bitrate or unsupported so far.
            break;
        }
    }

    return bitrate;
}


//----------------------------------------------------------------------------
// Attempt to get a "modulation type" for Dektec modulator cards.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::getDektecModulationType(int& type) const
{
#if defined(TS_NO_DTAPI)
    // No Dektec library.
    return false;
#else
    // Not all enum values used in switch, intentionally.
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(switch-default)
    TS_LLVM_NOWARNING(switch-enum)
    TS_MSC_NOWARNING(4061)

    // Determine modulation type.
    bool supported = true;
    switch (delivery_system.value(DS_UNDEFINED)) {
        case DS_DVB_S:   type = DTAPI_MOD_DVBS_QPSK; break;
        case DS_DVB_T:   type = DTAPI_MOD_DVBT; break;
        case DS_DVB_T2:  type = DTAPI_MOD_DVBT2; break;
        case DS_ATSC:    type = DTAPI_MOD_ATSC; break;
        case DS_ATSC_MH: type = DTAPI_MOD_ATSC_MH; break;
        case DS_ISDB_S:  type = DTAPI_MOD_ISDBS; break;
        case DS_ISDB_T:  type = DTAPI_MOD_ISDBT; break;
        case DS_DVB_C2:  type = DTAPI_MOD_DVBC2; break;
        case DS_DAB:     type = DTAPI_MOD_DAB; break;
        case DS_CMMB:    type = DTAPI_MOD_CMMB; break;
        case DS_DVB_S2:
            switch (modulation.value(DEFAULT_MODULATION_DVBS)) {
                case QPSK:    type = DTAPI_MOD_DVBS2_QPSK; break;
                case PSK_8:   type = DTAPI_MOD_DVBS2_8PSK; break;
                case APSK_16: type = DTAPI_MOD_DVBS2_16APSK; break;
                case APSK_32: type = DTAPI_MOD_DVBS2_32APSK; break;
                default:      type = DTAPI_MOD_DVBS2; break;
            }
            break;
        case DS_DVB_C_ANNEX_A:
        case DS_DVB_C_ANNEX_B:
        case DS_DVB_C_ANNEX_C:
            switch (modulation.value(QAM_AUTO)) {
                case QAM_16:  type = DTAPI_MOD_QAM16;  break;
                case QAM_32:  type = DTAPI_MOD_QAM32;  break;
                case QAM_64:  type = DTAPI_MOD_QAM64;  break;
                case QAM_128: type = DTAPI_MOD_QAM128; break;
                case QAM_256: type = DTAPI_MOD_QAM256; break;
                default: supported = false; break;
            }
            break;
        default:
            supported = false;
            break;
    }
    return supported;

    TS_POP_WARNING()
#endif // TS_NO_DTAPI
}


//----------------------------------------------------------------------------
// Attempt to get a "FEC type" for Dektec modulator cards.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::getDektecCodeRate(int& fec) const
{
    return ToDektecCodeRate(fec, inner_fec.value(DEFAULT_INNER_FEC));
}

bool ts::ModulationArgs::ToDektecCodeRate(int& fec, InnerFEC in_enum)
{
#if defined(TS_NO_DTAPI)
    // No Dektec library.
    return false;
#else
    // Not all enum values used in switch, intentionally.
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(switch-default)
    TS_LLVM_NOWARNING(switch-enum)
    TS_MSC_NOWARNING(4061)

    bool supported = true;
    switch (in_enum) {
        case FEC_1_2:  fec = DTAPI_MOD_1_2; break;
        case FEC_1_3:  fec = DTAPI_MOD_1_3; break;
        case FEC_1_4:  fec = DTAPI_MOD_1_4; break;
        case FEC_2_3:  fec = DTAPI_MOD_2_3; break;
        case FEC_2_5:  fec = DTAPI_MOD_2_5; break;
        case FEC_3_4:  fec = DTAPI_MOD_3_4; break;
        case FEC_3_5:  fec = DTAPI_MOD_3_5; break;
        case FEC_4_5:  fec = DTAPI_MOD_4_5; break;
        case FEC_5_6:  fec = DTAPI_MOD_5_6; break;
        case FEC_6_7:  fec = DTAPI_MOD_6_7; break;
        case FEC_7_8:  fec = DTAPI_MOD_7_8; break;
        case FEC_8_9:  fec = DTAPI_MOD_8_9; break;
        case FEC_9_10: fec = DTAPI_MOD_9_10; break;
        default: supported = false; break;
    }
    return supported;

    TS_POP_WARNING()
#endif // TS_NO_DTAPI
}


//----------------------------------------------------------------------------
// Attempt to convert the tuning parameters for Dektec modulator cards.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::convertToDektecModulation(int& modulation_type, int& param0, int& param1, int& param2) const
{
#if defined(TS_NO_DTAPI)
    // No Dektec library.
    return false;
#else
    // Get known parameters.
    if (!getDektecModulationType(modulation_type) || !getDektecCodeRate(param0)) {
        return false;
    }

    // Additional parameters param1 and param2
    param1 = param2 = 0;
    if (delivery_system == DS_DVB_S2) {
        param1 = pilots.value(DEFAULT_PILOTS) == PILOT_ON ? DTAPI_MOD_S2_PILOTS : DTAPI_MOD_S2_NOPILOTS;
        // Assume long FEC frame for broadcast service (should be updated by caller if necessary).
        param1 |= DTAPI_MOD_S2_LONGFRM;
        // Roll-off
        switch (roll_off.value(DEFAULT_ROLL_OFF)) {
            case ROLLOFF_AUTO: param1 |= DTAPI_MOD_ROLLOFF_AUTO; break;
            case ROLLOFF_20:   param1 |= DTAPI_MOD_ROLLOFF_20; break;
            case ROLLOFF_25:   param1 |= DTAPI_MOD_ROLLOFF_25; break;
            case ROLLOFF_35:   param1 |= DTAPI_MOD_ROLLOFF_35; break;
            default: break;
        }
        // Physical layer scrambling initialization sequence
        param2 = int(pls_code.value(DEFAULT_PLS_CODE));
    }

    return true;

#endif // TS_NO_DTAPI
}


//----------------------------------------------------------------------------
// Fill modulation parameters from a delivery system descriptor.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::fromDeliveryDescriptor(DuckContext& duck, const Descriptor& desc, uint16_t ts_id)
{
    // Filter out invalid descriptors.
    if (!desc.isValid()) {
        return false;
    }

    // Analyze descriptor.
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    bool status = true;

    switch (desc.tag()) {
        case DID_SAT_DELIVERY: {
            // DVB or ISDB satellite delivery network.
            // The descriptor can be used in either DVB or ISDB context. It has the same size
            // in both cases but a slightly different binary layout and semantics of fields.
            // There is no way to distinguish a DVB and an ISDB version without context.
            const bool isDVB = !(duck.standards() & Standards::ISDB);
            // TODO: Check S2_satellite_delivery_system_descriptor to get multistream id and PLS code. What about PLS mode?
            status = size >= 11;
            if (status) {
                frequency = uint64_t(DecodeBCD(data, 8)) * 10000;
                symbol_rate = DecodeBCD(data + 7, 7) * 100;
                // Polarity.
                switch ((data[6] >> 5) & 0x03) {
                    case 0: polarity = POL_HORIZONTAL; break;
                    case 1: polarity = POL_VERTICAL; break;
                    case 2: polarity = POL_LEFT; break;
                    case 3: polarity = POL_RIGHT; break;
                    default: assert(false);
                }
                if (isDVB) {
                    // DVB-S/S2 variant.
                    // Inner FEC.
                    switch (data[10] & 0x0F) {
                        case 1:  inner_fec = FEC_1_2; break;
                        case 2:  inner_fec = FEC_2_3; break;
                        case 3:  inner_fec = FEC_3_4; break;
                        case 4:  inner_fec = FEC_5_6; break;
                        case 5:  inner_fec = FEC_7_8; break;
                        case 6:  inner_fec = FEC_8_9; break;
                        case 7:  inner_fec = FEC_3_5; break;
                        case 8:  inner_fec = FEC_4_5; break;
                        case 9:  inner_fec = FEC_9_10; break;
                        case 15: inner_fec = FEC_NONE; break;
                        default: inner_fec = FEC_AUTO; break;
                    }
                    // Modulation type.
                    switch (data[6] & 0x03) {
                        case 0: modulation = QAM_AUTO; break;
                        case 1: modulation = QPSK; break;
                        case 2: modulation = PSK_8; break;
                        case 3: modulation = QAM_16; break;
                        default: assert(false);
                    }
                    // Modulation system.
                    switch ((data[6] >> 2) & 0x01) {
                        case 0:
                            delivery_system = DS_DVB_S;
                            roll_off.clear();
                            break;
                        case 1:
                            delivery_system = DS_DVB_S2;
                            // Roll off.
                            switch ((data[6] >> 3) & 0x03) {
                                case 0: roll_off = ROLLOFF_35; break;
                                case 1: roll_off = ROLLOFF_25; break;
                                case 2: roll_off = ROLLOFF_20; break;
                                case 3: roll_off = ROLLOFF_AUTO; break;
                                default: assert(false);
                            }
                            break;
                        default:
                            assert(false);
                    }
                }
                else {
                    // ISDB variant.
                    delivery_system = DS_ISDB_S;
                    roll_off.clear();
                    // The TS id is used in ISDB-S multi-stream encapsulation.
                    stream_id = ts_id;
                    // Inner FEC.
                    switch (data[10] & 0x0F) {
                        case 1:  inner_fec = FEC_1_2; break;
                        case 2:  inner_fec = FEC_2_3; break;
                        case 3:  inner_fec = FEC_3_4; break;
                        case 4:  inner_fec = FEC_5_6; break;
                        case 5:  inner_fec = FEC_7_8; break;
                        // 8  = ISDB-S system (refer to TMCC signal)
                        // 9  = 2.6GHz band digital satellite sound broadcasting
                        // 10 = Advanced narrow-band CS digital broadcasting
                        // Don't really know how to translate this...
                        case 15: inner_fec = FEC_NONE; break;
                        default: inner_fec = FEC_AUTO; break;
                    }
                    // Modulation type.
                    switch (data[6] & 0x03) {
                        case 0: modulation = QAM_AUTO; break;
                        case 1: modulation = QPSK; break;
                        case 8: modulation = PSK_8; break;
                        // 8  = "ISDB-S system (refer to TMCC signal)", TC8PSK?, is this the same as PSK_8?
                        // 9  = 2.6GHz band digital satellite sound broadcasting
                        // 10 = Advanced narrow-band CS digital broadcasting
                        // Don't really know how to translate this...
                        default: modulation.clear(); break;
                    }
                }
            }
            break;
        }
        case DID_CABLE_DELIVERY: {
            // DVB cable delivery network.
            status = size >= 11;
            if (status) {
                delivery_system = DS_DVB_C;
                frequency = uint64_t(DecodeBCD(data, 8)) * 100;
                symbol_rate = DecodeBCD(data + 7, 7) * 100;
                switch (data[10] & 0x0F) {
                    case 1:  inner_fec = FEC_1_2; break;
                    case 2:  inner_fec = FEC_2_3; break;
                    case 3:  inner_fec = FEC_3_4; break;
                    case 4:  inner_fec = FEC_5_6; break;
                    case 5:  inner_fec = FEC_7_8; break;
                    case 6:  inner_fec = FEC_8_9; break;
                    case 7:  inner_fec = FEC_3_5; break;
                    case 8:  inner_fec = FEC_4_5; break;
                    case 9:  inner_fec = FEC_9_10; break;
                    case 15: inner_fec = FEC_NONE; break;
                    default: inner_fec = FEC_AUTO; break;
                }
                switch (data[6]) {
                    case 1:  modulation = QAM_16; break;
                    case 2:  modulation = QAM_32; break;
                    case 3:  modulation = QAM_64; break;
                    case 4:  modulation = QAM_128; break;
                    case 5:  modulation = QAM_256; break;
                    default: modulation = QAM_AUTO; break;
                }
            }
            break;
        }
        case DID_TERREST_DELIVERY:  {
            // DVB terrestrial delivery network.
            status = size >= 11;
            if (status) {
                uint64_t freq = GetUInt32(data);
                uint8_t bwidth = data[4] >> 5;
                uint8_t constel = data[5] >> 6;
                uint8_t hier = (data[5] >> 3) & 0x07;
                uint8_t rate_hp = data[5] & 0x07;
                uint8_t rate_lp = data[6] >> 5;
                uint8_t guard = (data[6] >> 3) & 0x03;
                uint8_t transm = (data[6] >> 1) & 0x03;
                delivery_system = DS_DVB_T;
                frequency = freq == 0xFFFFFFFF ? 0 : freq * 10;
                switch (bwidth) {
                    case 0:  bandwidth = 8000000; break;
                    case 1:  bandwidth = 7000000; break;
                    case 2:  bandwidth = 6000000; break;
                    case 3:  bandwidth = 5000000; break;
                    default: bandwidth = 0; break;
                }
                switch (rate_hp) {
                    case 0:  fec_hp = FEC_1_2; break;
                    case 1:  fec_hp = FEC_2_3; break;
                    case 2:  fec_hp = FEC_3_4; break;
                    case 3:  fec_hp = FEC_5_6; break;
                    case 4:  fec_hp = FEC_7_8; break;
                    default: fec_hp = FEC_AUTO; break;
                }
                switch (rate_lp) {
                    case 0:  fec_lp = FEC_1_2; break;
                    case 1:  fec_lp = FEC_2_3; break;
                    case 2:  fec_lp = FEC_3_4; break;
                    case 3:  fec_lp = FEC_5_6; break;
                    case 4:  fec_lp = FEC_7_8; break;
                    default: fec_lp = FEC_AUTO; break;
                }
                switch (constel) {
                    case 0:  modulation = QPSK; break;
                    case 1:  modulation = QAM_16; break;
                    case 2:  modulation = QAM_64; break;
                    default: modulation = QAM_AUTO; break;
                }
                switch (transm) {
                    case 0:  transmission_mode = TM_2K; break;
                    case 1:  transmission_mode = TM_8K; break;
                    case 2:  transmission_mode = TM_4K; break;
                    default: transmission_mode = TM_AUTO; break;
                }
                switch (guard) {
                    case 0:  guard_interval = GUARD_1_32; break;
                    case 1:  guard_interval = GUARD_1_16; break;
                    case 2:  guard_interval = GUARD_1_8; break;
                    case 3:  guard_interval = GUARD_1_4; break;
                    default: guard_interval = GUARD_AUTO; break;
                }
                switch (hier & 0x03) {
                    case 0:  hierarchy = HIERARCHY_NONE; break;
                    case 1:  hierarchy = HIERARCHY_1; break;
                    case 2:  hierarchy = HIERARCHY_2; break;
                    case 3:  hierarchy = HIERARCHY_4; break;
                    default: hierarchy = HIERARCHY_AUTO; break;
                }
            }
            break;
        }
        case DID_ISDB_TERRES_DELIV:  {
            // ISDB terrestrial delivery network.
            status = size >= 4;
            if (status) {
                const uint8_t guard = (data[1] >> 2) & 0x03;
                const uint8_t transm = data[1] & 0x03;
                delivery_system = DS_ISDB_T;
                // The frequency in the descriptor is in units of 1/7 MHz.
                frequency = (1000000 * uint64_t(GetUInt16(data + 2))) / 7;
                switch (transm) {
                    case 0:  transmission_mode = TM_2K; break;
                    case 1:  transmission_mode = TM_8K; break;
                    case 2:  transmission_mode = TM_4K; break;
                    default: transmission_mode = TM_AUTO; break;
                }
                switch (guard) {
                    case 0:  guard_interval = GUARD_1_32; break;
                    case 1:  guard_interval = GUARD_1_16; break;
                    case 2:  guard_interval = GUARD_1_8; break;
                    case 3:  guard_interval = GUARD_1_4; break;
                    default: guard_interval = GUARD_AUTO; break;
                }
            }
            break;
        }
        default: {
            // Not a valid delivery descriptor.
            status = false;
            break;
        }
    }

    return status;
}


//----------------------------------------------------------------------------
// Format a short description (frequency and essential parameters).
//----------------------------------------------------------------------------

ts::UString ts::ModulationArgs::shortDescription(DuckContext& duck) const
{
    // Don't know what to describe without delivery system or frequency.
    if (!delivery_system.set() || !frequency.set()) {
        return UString();
    }

    UString desc;
    switch (TunerTypeOf(delivery_system.value())) {
        case TT_DVB_T:
        case TT_ISDB_T: {
            // Try to resolve UHF/VHF channels.
            const UChar* band = nullptr;
            uint32_t channel = 0;
            int32_t offset = 0;

            // Get UHF and VHF band descriptions in the default region.
            const ts::HFBand* uhf = duck.uhfBand();
            const ts::HFBand* vhf = duck.vhfBand();

            if (uhf->inBand(frequency.value(), true)) {
                band = u"UHF";
                channel = uhf->channelNumber(frequency.value());
                offset = uhf->offsetCount(frequency.value());
            }
            else if (vhf->inBand(frequency.value(), true)) {
                band = u"VHF";
                channel = vhf->channelNumber(frequency.value());
                offset = vhf->offsetCount(frequency.value());
            }

            if (band != nullptr) {
                desc += UString::Format(u"%s channel %d", {band, channel});
                if (offset != 0) {
                    desc += UString::Format(u", offset %+d", {offset});
                }
                desc += u" (";
            }
            desc += UString::Format(u"%'d Hz", {frequency.value()});
            if (band != nullptr) {
                desc += u")";
            }

            if (plp != PLP_DISABLE) {
                desc += UString::Format(u", PLP %d", {plp.value()});
            }
            break;
        }
        case TT_DVB_S:
        case TT_ISDB_S: {
            // Display frequency and polarity.
            desc = UString::Format(u"%'d Hz", {frequency.value()});
            if (polarity.set()) {
                switch (polarity.value()) {
                    case POL_HORIZONTAL:
                        desc += u" H";
                        break;
                    case POL_VERTICAL:
                        desc += u" V";
                        break;
                    case POL_LEFT:
                        desc += u" L";
                        break;
                    case POL_RIGHT:
                        desc += u" R";
                        break;
                    case POL_AUTO:
                    case POL_NONE:
                    default:
                        break;
                }
            }
            if (delivery_system != DS_DVB_S && delivery_system != DS_ISDB_S) {
                desc += u" (" + DeliverySystemEnum.name(delivery_system.value());
                if (modulation != QAM_AUTO) {
                    desc += u", " + ModulationEnum.name(modulation.value());
                }
                desc += u")";
            }
            break;
        }
        case TT_ATSC:
        case TT_DVB_C:
        case TT_ISDB_C:
        case TT_UNDEFINED:
        default: {
            // Generic display.
            desc = UString::Format(u"%'d Hz", {frequency.value()});
            break;
        }
    }
    return desc;
}


//----------------------------------------------------------------------------
// Display a description of the paramters on a stream, line by line.
//----------------------------------------------------------------------------

std::ostream& ts::ModulationArgs::display(std::ostream& strm, const ts::UString& margin, int level) const
{
    const bool verbose = level >= Severity::Verbose;

    if (frequency.set() && frequency != 0) {
        strm << margin << "Carrier frequency: " << UString::Decimal(frequency.value()) << " Hz" << std::endl;
    }
    if (inversion.set() && inversion != SPINV_AUTO) {
        strm << margin << "Spectral inversion: " << SpectralInversionEnum.name(inversion.value()) << std::endl;
    }
    if (modulation.set() && modulation != QAM_AUTO) {
        strm << margin << "Modulation: " << ModulationEnum.name(modulation.value()) << std::endl;
    }

    switch (TunerTypeOf(delivery_system.value(DS_UNDEFINED))) {
        case TT_DVB_C: {
            if (symbol_rate.set() && symbol_rate != 0) {
                strm << margin << "Symbol rate: " << UString::Decimal(symbol_rate.value()) << " symb/s" << std::endl;
            }
            if (inner_fec.set() && inner_fec != FEC_AUTO) {
                strm << margin << "FEC inner: " << InnerFECEnum.name(inner_fec.value()) << std::endl;
            }
            break;
        }
        case TT_DVB_T: {
            if (fec_hp.set() && fec_hp != FEC_AUTO) {
                strm << margin << "HP streams FEC: " << InnerFECEnum.name(fec_hp.value()) << std::endl;
            }
            if (fec_lp.set() && fec_lp != FEC_AUTO) {
                strm << margin << "LP streams FEC: " << InnerFECEnum.name(fec_lp.value()) << std::endl;
            }
            if (guard_interval.set() && guard_interval != GUARD_AUTO) {
                strm << margin << "Guard interval: " << GuardIntervalEnum.name(guard_interval.value()) << std::endl;
            }
            if (bandwidth.set() && bandwidth != 0) {
                strm << margin << "Bandwidth: " << UString::Decimal(bandwidth.value()) << " Hz" << std::endl;
            }
            if (transmission_mode.set() && transmission_mode != TM_AUTO) {
                strm << margin << "Transmission mode: " << TransmissionModeEnum.name(transmission_mode.value()) << std::endl;
            }
            if (hierarchy.set() && hierarchy != HIERARCHY_AUTO) {
                strm << margin << "Hierarchy: " << HierarchyEnum.name(hierarchy.value()) << std::endl;
            }
            break;
        }
        case TT_DVB_S: {
            if (polarity.set() && polarity != POL_AUTO) {
                strm << margin << "Polarity: " << PolarizationEnum.name(polarity.value()) << std::endl;
            }
            if (inversion.set() && inversion != SPINV_AUTO) {
                strm << margin << "Spectral inversion: " << SpectralInversionEnum.name(inversion.value()) << std::endl;
            }
            if (symbol_rate.set() && symbol_rate != 0) {
                strm << margin << "Symbol rate: " << UString::Decimal(symbol_rate.value()) << " symb/s" << std::endl;
            }
            if (inner_fec.set() && inner_fec != ts::FEC_AUTO) {
                strm << margin << "FEC inner: " << InnerFECEnum.name(inner_fec.value()) << std::endl;
            }
            if (isi.set() && isi != ISI_DISABLE) {
                strm << margin << "Input stream id: " << isi.value() << std::endl
                     << margin << "PLS code: " << pls_code.value(DEFAULT_PLS_CODE) << std::endl
                     << margin << "PLS mode: "<< PLSModeEnum.name(pls_mode.value(DEFAULT_PLS_MODE)) << std::endl;
            }
            if ((verbose || delivery_system != DS_DVB_S) && pilots.set() && pilots != PILOT_AUTO) {
                strm << margin << "Pilots: " << PilotEnum.name(pilots.value()) << std::endl;
            }
            if ((verbose || delivery_system != DS_DVB_S) && roll_off.set() && roll_off != ROLLOFF_AUTO) {
                strm << margin << "Roll-off: " << RollOffEnum.name(roll_off.value()) << std::endl;
            }
            if (verbose && lnb.set()) {
                strm << margin << "LNB: " << lnb.value() << std::endl;
            }
            if (verbose) {
                strm << margin << "Satellite number: " << satellite_number.value(DEFAULT_SATELLITE_NUMBER) << std::endl;
            }
            break;
        }
        case TT_ISDB_S: {
            if (polarity.set() && polarity != POL_AUTO) {
                strm << margin << "Polarity: " << PolarizationEnum.name(polarity.value()) << std::endl;
            }
            if (inversion.set() && inversion != SPINV_AUTO) {
                strm << margin << "Spectral inversion: " << SpectralInversionEnum.name(inversion.value()) << std::endl;
            }
            if (symbol_rate.set() && symbol_rate != 0) {
                strm << margin << "Symbol rate: " << UString::Decimal(symbol_rate.value()) << " symb/s" << std::endl;
            }
            if (stream_id.set()) {
                strm << margin << "Innert transport stream id: " << stream_id.value() << std::endl;
            }
            if (inner_fec.set() && inner_fec != ts::FEC_AUTO) {
                strm << margin << "FEC inner: " << InnerFECEnum.name(inner_fec.value()) << std::endl;
            }
            if (verbose && lnb.set()) {
                strm << margin << "LNB: " << lnb.value() << std::endl;
            }
            if (verbose) {
                strm << margin << "Satellite number: " << satellite_number.value(DEFAULT_SATELLITE_NUMBER) << std::endl;
            }
            break;
        }
        case TT_ISDB_T: {
            if (guard_interval.set() && guard_interval != GUARD_AUTO) {
                strm << margin << "Guard interval: " << GuardIntervalEnum.name(guard_interval.value()) << std::endl;
            }
            if (bandwidth.set() && bandwidth != 0) {
                strm << margin << "Bandwidth: " << UString::Decimal(bandwidth.value()) << " Hz" << std::endl;
            }
            if (transmission_mode.set() && transmission_mode != TM_AUTO) {
                strm << margin << "Transmission mode: " << TransmissionModeEnum.name(transmission_mode.value()) << std::endl;
            }
            if (sound_broadcasting == true) {
                strm << margin << "Sound broadcasting: on" << std::endl;
                if (sb_subchannel_id.set()) {
                    strm << margin << "- Sub-channel id: " << sb_subchannel_id.value() << std::endl;
                }
                if (sb_segment_count.set()) {
                    strm << margin << "- Segment count: " << sb_segment_count.value() << std::endl;
                }
                if (sb_segment_index.set()) {
                    strm << margin << "- Segment index: " << sb_segment_index.value() << std::endl;
                }
            }
            if (isdbt_layers.set()) {
                strm << margin << "Layers: " << (isdbt_layers.value().empty() ? u"none" : isdbt_layers.value()) << std::endl;
            }
            if (isdbt_partial_reception.set()) {
                strm << margin << "Partial reception: " << UString::OnOff(isdbt_partial_reception.value()) << std::endl;
            }
            if (layer_a_fec.set() && layer_a_fec != FEC_AUTO) {
                strm << margin << "Layer A FEC: " << InnerFECEnum.name(layer_a_fec.value()) << std::endl;
            }
            if (layer_a_modulation.set() && layer_a_modulation != QAM_AUTO) {
                strm << margin << "Layer A modulation: " << ModulationEnum.name(layer_a_modulation.value()) << std::endl;
            }
            if (layer_a_segment_count.set() && layer_a_segment_count.value() <= MAX_ISDBT_SEGMENT_COUNT) {
                strm << margin << "Layer A segment count: " << layer_a_segment_count.value() << std::endl;
            }
            if (layer_a_time_interleaving.set() && IsValidISDBTTimeInterleaving(layer_a_time_interleaving.value())) {
                strm << margin << "Layer A time interleaving: " << layer_a_time_interleaving.value() << std::endl;
            }
            if (layer_b_fec.set() && layer_b_fec != FEC_AUTO) {
                strm << margin << "Layer B FEC: " << InnerFECEnum.name(layer_b_fec.value()) << std::endl;
            }
            if (layer_b_modulation.set() && layer_b_modulation != QAM_AUTO) {
                strm << margin << "Layer B modulation: " << ModulationEnum.name(layer_b_modulation.value()) << std::endl;
            }
            if (layer_b_segment_count.set() && layer_b_segment_count.value() <= MAX_ISDBT_SEGMENT_COUNT) {
                strm << margin << "Layer B segment count: " << layer_b_segment_count.value() << std::endl;
            }
            if (layer_b_time_interleaving.set() && IsValidISDBTTimeInterleaving(layer_b_time_interleaving.value())) {
                strm << margin << "Layer B time interleaving: " << layer_b_time_interleaving.value() << std::endl;
            }
            if (layer_c_fec.set() && layer_c_fec != FEC_AUTO) {
                strm << margin << "Layer C FEC: " << InnerFECEnum.name(layer_c_fec.value()) << std::endl;
            }
            if (layer_c_modulation.set() && layer_c_modulation != QAM_AUTO) {
                strm << margin << "Layer C modulation: " << ModulationEnum.name(layer_c_modulation.value()) << std::endl;
            }
            if (layer_c_segment_count.set() && layer_c_segment_count.value() <= MAX_ISDBT_SEGMENT_COUNT) {
                strm << margin << "Layer C segment count: " << layer_c_segment_count.value() << std::endl;
            }
            if (layer_c_time_interleaving.set() && IsValidISDBTTimeInterleaving(layer_c_time_interleaving.value())) {
                strm << margin << "Layer C time interleaving: " << layer_c_time_interleaving.value() << std::endl;
            }
            break;
        }
        case TT_ISDB_C:
        case TT_ATSC:
        case TT_UNDEFINED:
        default: {
            break;
        }
    }
    return strm;
}


//----------------------------------------------------------------------------
// Format the modulation parameters as command line arguments.
//----------------------------------------------------------------------------

ts::UString ts::ModulationArgs::toPluginOptions(bool no_local) const
{
    // Don't know what to describe without delivery system or frequency.
    if (!delivery_system.set() || !frequency.set()) {
        return UString();
    }

    // Delivery system and frequency are common options and always come first.
    UString opt(UString::Format(u"--delivery-system %s --frequency %'d", {DeliverySystemEnum.name(delivery_system.value()), frequency.value()}));

    // All other options depend on the tuner type.
    switch (TunerTypeOf(delivery_system.value())) {
        case TT_ATSC: {
            opt += UString::Format(u" --modulation %s", {ModulationEnum.name(modulation.value(DEFAULT_MODULATION_ATSC))});
            break;
        }
        case TT_DVB_C: {
            opt += UString::Format(u" --symbol-rate %'d --fec-inner %s --modulation %s", {
                                   symbol_rate.value(DEFAULT_SYMBOL_RATE_DVBC),
                                   InnerFECEnum.name(inner_fec.value(DEFAULT_INNER_FEC)),
                                   ModulationEnum.name(modulation.value(DEFAULT_MODULATION_DVBC))});
            break;
        }
        case TT_DVB_T: {
            opt += UString::Format(u" --modulation %s"
                                   u" --high-priority-fec %s"
                                   u" --low-priority-fec %s"
                                   u" --bandwidth %'d"
                                   u" --transmission-mode %s"
                                   u" --guard-interval %s"
                                   u" --hierarchy %s", {
                                   ModulationEnum.name(modulation.value(DEFAULT_MODULATION_DVBT)),
                                   InnerFECEnum.name(fec_hp.value(DEFAULT_FEC_HP)),
                                   InnerFECEnum.name(fec_lp.value(DEFAULT_FEC_LP)),
                                   bandwidth.value(DEFAULT_BANDWIDTH_DVBT),
                                   TransmissionModeEnum.name(transmission_mode.value(DEFAULT_TRANSMISSION_MODE_DVBT)),
                                   GuardIntervalEnum.name(guard_interval.value(DEFAULT_GUARD_INTERVAL_DVBT)),
                                   HierarchyEnum.name(hierarchy.value(DEFAULT_HIERARCHY))});
            if (plp.set() && plp != PLP_DISABLE) {
                opt += UString::Format(u" --plp %d", {plp.value()});
            }
            break;
        }
        case TT_DVB_S: {
            opt += UString::Format(u" --symbol-rate %'d"
                                   u" --fec-inner %s"
                                   u" --polarity %s"
                                   u" --modulation %s", {
                                   symbol_rate.value(DEFAULT_SYMBOL_RATE_DVBS),
                                   InnerFECEnum.name(inner_fec.value(DEFAULT_INNER_FEC)),
                                   PolarizationEnum.name(polarity.value(DEFAULT_POLARITY)),
                                   ModulationEnum.name(modulation.value(DEFAULT_MODULATION_DVBS))});
            if (delivery_system == DS_DVB_S2) {
                opt += UString::Format(u" --pilots %s --roll-off %s", {
                                       PilotEnum.name(pilots.value(DEFAULT_PILOTS)),
                                       RollOffEnum.name(roll_off.value(DEFAULT_ROLL_OFF))});
            }
            if (isi.set() && isi != DEFAULT_ISI) {
                opt += UString::Format(u" --isi %d", {isi.value()});
            }
            if (pls_code.set() && pls_code != DEFAULT_PLS_CODE) {
                opt += UString::Format(u" --pls-code %d", {pls_code.value()});
            }
            if (pls_mode.set() && pls_mode != DEFAULT_PLS_MODE) {
                opt += UString::Format(u" --pls-mode %s", {PLSModeEnum.name(pls_mode.value())});
            }
            if (!no_local && lnb.set()) {
                opt += UString::Format(u" --lnb %s", {lnb.value()});
            }
            if (!no_local && satellite_number.set()) {
                opt += UString::Format(u" --satellite-number %d", {satellite_number.value()});
            }
            break;
        }
        case TT_ISDB_S: {
            opt += UString::Format(u" --symbol-rate %'d --fec-inner %s --polarity %s", {
                                   symbol_rate.value(DEFAULT_SYMBOL_RATE_DVBS),
                                   InnerFECEnum.name(inner_fec.value(DEFAULT_INNER_FEC)),
                                   PolarizationEnum.name(polarity.value(DEFAULT_POLARITY))});
            if (stream_id.set() && stream_id != DEFAULT_STREAM_ID) {
                opt += UString::Format(u" --stream-id %d", {stream_id.value()});
            }
            if (!no_local && lnb.set()) {
                opt += UString::Format(u" --lnb %s", {lnb.value()});
            }
            if (!no_local && satellite_number.set()) {
                opt += UString::Format(u" --satellite-number %d", {satellite_number.value()});
            }
            break;
        }
        case TT_ISDB_T: {
            opt += UString::Format(u" --bandwidth %'d --transmission-mode %s --guard-interval %s", {
                                   bandwidth.value(DEFAULT_BANDWIDTH_ISDBT),
                                   TransmissionModeEnum.name(transmission_mode.value(DEFAULT_TRANSMISSION_MODE_DVBT)),
                                   GuardIntervalEnum.name(guard_interval.value(DEFAULT_GUARD_INTERVAL_DVBT))});
            if (sound_broadcasting == true) {
                opt += UString::Format(u" --sound-broadcasting --sb-subchannel-id %d --sb-segment-count %d --sb-segment-index %d", {
                                       sb_subchannel_id.value(DEFAULT_SB_SUBCHANNEL_ID),
                                       sb_segment_count.value(DEFAULT_SB_SEGMENT_COUNT),
                                       sb_segment_index.value(DEFAULT_SB_SEGMENT_INDEX)});
            }
            if (isdbt_partial_reception == true) {
                opt += u" --isdbt-partial-reception";
            }
            if (!isdbt_layers.set() || !isdbt_layers.value().empty()) {
                opt += UString::Format(u" --isdbt-layers \"%s\"", {isdbt_layers.value(DEFAULT_ISDBT_LAYERS)});
            }
            if (layer_a_fec.set() && layer_a_fec != FEC_AUTO) {
                opt += UString::Format(u" --isdbt-layer-a-fec %s", {InnerFECEnum.name(layer_a_fec.value())});
            }
            if (layer_a_modulation.set() && layer_a_modulation != QAM_AUTO) {
                opt += UString::Format(u" --isdbt-layer-a-modulation %s", {ModulationEnum.name(layer_a_modulation.value())});
            }
            if (layer_a_segment_count.set()) {
                opt += UString::Format(u" --isdbt-layer-a-segment-count %d", {layer_a_segment_count.value()});
            }
            if (layer_a_time_interleaving.set()) {
                opt += UString::Format(u" --isdbt-layer-a-time-interleaving %d", {layer_a_time_interleaving.value()});
            }
            if (layer_b_fec.set() && layer_b_fec != FEC_AUTO) {
                opt += UString::Format(u" --isdbt-layer-b-fec %s", {InnerFECEnum.name(layer_b_fec.value())});
            }
            if (layer_b_modulation.set() && layer_b_modulation != QAM_AUTO) {
                opt += UString::Format(u" --isdbt-layer-b-modulation %s", {ModulationEnum.name(layer_b_modulation.value())});
            }
            if (layer_b_segment_count.set()) {
                opt += UString::Format(u" --isdbt-layer-b-segment-count %d", {layer_b_segment_count.value()});
            }
            if (layer_b_time_interleaving.set()) {
                opt += UString::Format(u" --isdbt-layer-b-time-interleaving %d", {layer_b_time_interleaving.value()});
            }
            if (layer_c_fec.set() && layer_c_fec != FEC_AUTO) {
                opt += UString::Format(u" --isdbt-layer-c-fec %s", {InnerFECEnum.name(layer_c_fec.value())});
            }
            if (layer_c_modulation.set() && layer_c_modulation != QAM_AUTO) {
                opt += UString::Format(u" --isdbt-layer-c-modulation %s", {ModulationEnum.name(layer_c_modulation.value())});
            }
            if (layer_c_segment_count.set()) {
                opt += UString::Format(u" --isdbt-layer-c-segment-count %d", {layer_c_segment_count.value()});
            }
            if (layer_c_time_interleaving.set()) {
                opt += UString::Format(u" --isdbt-layer-c-time-interleaving %d", {layer_c_time_interleaving.value()});
            }
            break;
        }
        case TT_ISDB_C:
        case TT_UNDEFINED:
        default: {
            break;
        }
    }

    // Add spectral inversion (common option).
    if (inversion.set() && inversion != DEFAULT_INVERSION) {
        opt += u" --spectral-inversion ";
        opt += SpectralInversionEnum.name(inversion.value());
    }

    return opt;
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::loadArgs(DuckContext& duck, Args& args)
{
    bool status = true;

    // If delivery system is unspecified, will use the default one for the tuner.
    args.getOptionalIntValue(delivery_system, u"delivery-system");

    // Carrier frequency
    if (args.present(u"frequency") + args.present(u"uhf-channel") + args.present(u"vhf-channel") > 1) {
        args.error(u"options --frequency, --uhf-channel and --vhf-channel are mutually exclusive");
        status = false;
    }
    else if (args.present(u"frequency")) {
        args.getOptionalIntValue(frequency, u"frequency");
    }
    else if (args.present(u"uhf-channel")) {
        frequency = duck.uhfBand()->frequency(args.intValue<uint32_t>(u"uhf-channel"), args.intValue<int32_t>(u"offset-count", 0));
    }
    else if (args.present(u"vhf-channel")) {
        frequency = duck.vhfBand()->frequency(args.intValue<uint32_t>(u"vhf-channel"), args.intValue<int32_t>(u"offset-count", 0));
    }

    // Other individual tuning options
    args.getOptionalIntValue(symbol_rate, u"symbol-rate");
    args.getOptionalIntValue(polarity, u"polarity");
    args.getOptionalIntValue(inversion, u"spectral-inversion");
    args.getOptionalIntValue(inner_fec, u"fec-inner");
    args.getOptionalIntValue(modulation, u"modulation");
    args.getOptionalIntValue(fec_hp, u"high-priority-fec");
    args.getOptionalIntValue(fec_lp, u"low-priority-fec");
    args.getOptionalIntValue(transmission_mode, u"transmission-mode");
    args.getOptionalIntValue(guard_interval, u"guard-interval");
    args.getOptionalIntValue(hierarchy, u"hierarchy");
    args.getOptionalIntValue(pilots, u"pilots");
    args.getOptionalIntValue(roll_off, u"roll-off");
    args.getOptionalIntValue(plp, u"plp");
    args.getOptionalIntValue(isi, u"isi");
    args.getOptionalIntValue(pls_code, u"pls-code");
    args.getOptionalIntValue(pls_mode, u"pls-mode");
    if (args.present(u"sound-broadcasting")) {
        sound_broadcasting = true;
    }
    if (args.present(u"isdbt-partial-reception")) {
        isdbt_partial_reception = true;
    }
    args.getOptionalIntValue(sb_subchannel_id, u"sb-subchannel-id");
    args.getOptionalIntValue(sb_segment_count, u"sb-segment-count");
    args.getOptionalIntValue(sb_segment_index, u"sb-segment-index");
    args.getOptionalValue(isdbt_layers, u"isdbt-layers");
    args.getOptionalIntValue(layer_a_fec, u"isdbt-layer-a-fec");
    args.getOptionalIntValue(layer_a_modulation, u"isdbt-layer-a-modulation");
    args.getOptionalIntValue(layer_a_segment_count, u"isdbt-layer-a-segment-count");
    args.getOptionalIntValue(layer_a_time_interleaving, u"isdbt-layer-a-time-interleaving");
    args.getOptionalIntValue(layer_b_fec, u"isdbt-layer-b-fec");
    args.getOptionalIntValue(layer_b_modulation, u"isdbt-layer-b-modulation");
    args.getOptionalIntValue(layer_b_segment_count, u"isdbt-layer-b-segment-count");
    args.getOptionalIntValue(layer_b_time_interleaving, u"isdbt-layer-b-time-interleaving");
    args.getOptionalIntValue(layer_c_fec, u"isdbt-layer-c-fec");
    args.getOptionalIntValue(layer_c_modulation, u"isdbt-layer-c-modulation");
    args.getOptionalIntValue(layer_c_segment_count, u"isdbt-layer-c-segment-count");
    args.getOptionalIntValue(layer_c_time_interleaving, u"isdbt-layer-c-time-interleaving");
    args.getOptionalIntValue(stream_id, u"stream-id");
    LoadLegacyBandWidthArg(bandwidth, args, u"bandwidth");

    // Local options (not related to transponder)
    if (args.present(u"lnb")) {
        const UString s(args.value(u"lnb"));
        const LNB l(s, duck.report());
        if (!l.isValid()) {
            status = false;
        }
        else {
            args.debug(u"loaded LNB \"%s\" from command line", {l});
            lnb = l;
        }
    }
    args.getOptionalIntValue(satellite_number, u"satellite-number");

    // Mark arguments as invalid is some errors were found.
    if (!status) {
        args.invalidate();
    }
    return status;
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::ModulationArgs::defineArgs(Args& args, bool allow_short_options)
{
    args.option(u"delivery-system", 0, DeliverySystemEnum);
    args.help(u"delivery-system",
              u"Specify which delivery system to use. By default, use the default system for the tuner.");

    args.option(u"frequency", allow_short_options ? 'f' : 0, Args::UNSIGNED);
    args.help(u"frequency", u"Carrier frequency in Hz (all tuners). There is no default.");

    args.option(u"polarity", 0, PolarizationEnum);
    args.help(u"polarity",
              u"Used for satellite tuners only. "
              u"Polarity. The default is \"vertical\".");

    args.option(u"lnb", 0, Args::STRING);
    args.help(u"lnb", u"name",
              u"Used for satellite tuners only. "
              u"Description of the LNB. The specified string is the name (or an alias for that name) "
              u"of a preconfigured LNB in the configuration file tsduck.lnbs.xml. "
              u"For compatibility, the legacy format 'low_freq[,high_freq,switch_freq]' is also accepted "
              u"(all frequencies are in MHz). The default is a universal extended LNB.");

    args.option(u"spectral-inversion", 0, SpectralInversionEnum);
    args.help(u"spectral-inversion",
              u"Spectral inversion. The default is \"auto\".");

    args.option(u"symbol-rate", allow_short_options ? 's' : 0, Args::UNSIGNED);
    args.help(u"symbol-rate",
              u"Used for satellite and cable tuners only. "
              u"Symbol rate in symbols/second. The default is " +
              UString::Decimal(DEFAULT_SYMBOL_RATE_DVBS) + u" sym/s for DVB-S, " +
              UString::Decimal(DEFAULT_SYMBOL_RATE_DVBC) + u" sym/s for DVB-C, " +
              UString::Decimal(DEFAULT_SYMBOL_RATE_ISDBS) + u" sym/s for ISDB-S, ");

    args.option(u"fec-inner", 0, InnerFECEnum);
    args.help(u"fec-inner",
              u"Used for satellite and cable tuners only. Inner Forward Error Correction. "
              u"The default is \"auto\".");

    args.option(u"satellite-number", 0, Args::INTEGER, 0, 1, 0, 3);
    args.help(u"satellite-number",
              u"Used for satellite tuners only. Satellite/dish number. "
              u"Must be 0 to 3 with DiSEqC switches and 0 to 1 fornon-DiSEqC switches. The default is 0.");

    args.option(u"modulation", allow_short_options ? 'm' : 0, ModulationEnum);
    args.help(u"modulation",
              u"Used for DVB-C, DVB-T, DVB-S2 and ATSC tuners. Modulation type. The default is \"" +
              ModulationEnum.name(DEFAULT_MODULATION_DVBT) + u"\" for DVB-T/T2, \"" +
              ModulationEnum.name(DEFAULT_MODULATION_DVBC) + u"\" for DVB-C, \"" +
              ModulationEnum.name(DEFAULT_MODULATION_DVBS) + u"\" for DVB-S2, \"" +
              ModulationEnum.name(DEFAULT_MODULATION_ATSC) + u"\" for ATSC.");

    args.option(u"high-priority-fec", 0, InnerFECEnum);
    args.help(u"high-priority-fec",
              u"Used for DVB-T/T2 tuners only. Error correction for high priority streams. "
              u"The default is \"auto\".");

    args.option(u"low-priority-fec", 0, InnerFECEnum);
    args.help(u"low-priority-fec",
              u"Used for DVB-T/T2 tuners only. Error correction for low priority streams. "
              u"The default is \"auto\".");

    DefineLegacyBandWidthArg(args, u"bandwidth", 0, DEFAULT_BANDWIDTH_DVBT, DEFAULT_BANDWIDTH_ISDBT);

    args.option(u"transmission-mode", 0, TransmissionModeEnum);
    args.help(u"transmission-mode",
              u"Used for terrestrial tuners only. Transmission mode. The default is \"" +
              TransmissionModeEnum.name(DEFAULT_TRANSMISSION_MODE_DVBT) + u"\" for DVB-T/T2, \"" +
              TransmissionModeEnum.name(DEFAULT_TRANSMISSION_MODE_ISDBT) + u"\" for ISDB-T.");

    args.option(u"guard-interval", 0, GuardIntervalEnum);
    args.help(u"guard-interval",
              u"Used for terrestrial tuners only. Guard interval. The default is \"" +
              GuardIntervalEnum.name(DEFAULT_GUARD_INTERVAL_DVBT) + u"\" for DVB-T/T2, \"" +
              GuardIntervalEnum.name(DEFAULT_GUARD_INTERVAL_ISDBT) + u"\" for ISDB-T.");

    args.option(u"hierarchy", 0, HierarchyEnum);
    args.help(u"hierarchy", u"Used for DVB-T/T2 tuners only. The default is \"none\".");

    args.option(u"pilots", 0, PilotEnum);
    args.help(u"pilots",
              u"Used for DVB-S2 tuners only. Presence of pilots frames. "
              u"The default is \"off\". ");

    args.option(u"roll-off", 0, RollOffEnum);
    args.help(u"roll-off",
              u"Used for DVB-S2 tuners only. Roll-off factor. "
              u"The default is \"0.35\" (implied for DVB-S, default for DVB-S2).");

    args.option(u"plp", 0, Args::UINT8);
    args.help(u"plp",
              u"Used for DVB-T2 tuners only. "
              u"Physical Layer Pipe (PLP) number to select, from 0 to 255. "
              u"The default is to keep the entire stream, without PLP selection. "
              u"Warning: this option is supported on Linux only.");

    args.option(u"isi", 0, Args::UINT8);
    args.help(u"isi",
              u"Used for DVB-S2 tuners only. "
              u"Input Stream Id (ISI) number to select, from 0 to 255. "
              u"The default is to keep the entire stream, without multistream selection. "
              u"Warning: this option is supported on Linux only.");

    args.option(u"pls-code", 0, Args::INTEGER, 0, 1, 0, PLS_CODE_MAX);
    args.help(u"pls-code",
              u"Used for DVB-S2 tuners only. "
              u"Physical Layer Scrambling (PLS) code value. With multistream only. "
              u"Warning: this option is supported on Linux only.");

    args.option(u"pls-mode", 0, PLSModeEnum);
    args.help(u"pls-mode", u"mode",
              u"Used for DVB-S2 tuners only. "
              u"Physical Layer Scrambling (PLS) mode. With multistream only. The default is ROOT. "
              u"Warning: this option is supported on Linux only.");

    // ISDB-T specific options.
    args.option(u"sound-broadcasting");
    args.help(u"sound-broadcasting",
              u"Used for ISDB-T tuners only. "
              u"Specify that the reception is an ISDB-Tsb (sound broadcasting) channel instead of an ISDB-T one.");

    args.option(u"sb-subchannel-id", 0, Args::INTEGER, 0, 1, 0, 41);
    args.help(u"sb-subchannel-id",
              u"Used for ISDB-T tuners only. "
              u"With --sound-broadcasting, specify the sub-channel id of the segment to be demodulated "
              u"in the ISDB-Tsb channel. Possible values: 0 to 41. The default is " +
              UString::Decimal(DEFAULT_SB_SUBCHANNEL_ID) + u".");

    args.option(u"sb-segment-count", 0, Args::INTEGER, 0, 1, 1, 13);
    args.help(u"sb-segment-count",
              u"Used for ISDB-T tuners only. "
              u"With --sound-broadcasting, specify the total count of connected ISDB-Tsb channels. "
              u"Possible values: 1 to 13. The default is " + UString::Decimal(DEFAULT_SB_SEGMENT_COUNT) + u".");

    args.option(u"sb-segment-index", 0, Args::INTEGER, 0, 1, 0, 12);
    args.help(u"sb-segment-index",
              u"Used for ISDB-T tuners only. "
              u"With --sound-broadcasting, specify the index of the segment to be demodulated for "
              u"an ISDB-Tsb channel where several of them are transmitted in the connected manner. "
              u"Possible values: 0 to sb-segment-count - 1. The default is " +
              UString::Decimal(DEFAULT_SB_SEGMENT_INDEX) + u".");

    args.option(u"isdbt-partial-reception");
    args.help(u"isdbt-partial-reception",
              u"Used for ISDB-T tuners only. "
              u"Specify that the reception of the ISDB-T channel is in partial reception mode. "
              u"The default is automatically detected.");

    args.option(u"isdbt-layers", 0, Args::STRING);
    args.help(u"isdbt-layers", u"'string'",
              u"Used for ISDB-T tuners only. "
              u"Hierarchical reception in ISDB-T is achieved by enabling or disabling layers in the decoding process. "
              u"The specified string contains a combination of characters 'A', 'B', 'C', indicating which layers "
              u"shall be used. The default is \"ABC\" (all layers).");

    args.option(u"isdbt-layer-a-fec", 0, InnerFECEnum);
    args.option(u"isdbt-layer-b-fec", 0, InnerFECEnum);
    args.option(u"isdbt-layer-c-fec", 0, InnerFECEnum);

    args.help(u"isdbt-layer-a-fec",
              u"Used for ISDB-T tuners only. Error correction for layer A. "
              u"The default is automatically detected.");
    args.help(u"isdbt-layer-b-fec", u"Same as --isdbt-layer-a-fec for layer B.");
    args.help(u"isdbt-layer-c-fec", u"Same as --isdbt-layer-a-fec for layer C.");

    args.option(u"isdbt-layer-a-modulation", 0, ModulationEnum);
    args.option(u"isdbt-layer-b-modulation", 0, ModulationEnum);
    args.option(u"isdbt-layer-c-modulation", 0, ModulationEnum);

    args.help(u"isdbt-layer-a-modulation",
              u"Used for ISDB-T tuners only. Modulation for layer A. "
              u"The default is automatically detected.");
    args.help(u"isdbt-layer-b-modulation", u"Same as --isdbt-layer-a-modulation for layer B.");
    args.help(u"isdbt-layer-c-modulation", u"Same as --isdbt-layer-a-modulation for layer C.");

    args.option(u"isdbt-layer-a-segment-count", 0, Args::INTEGER, 0, 1, 0, 13);
    args.option(u"isdbt-layer-b-segment-count", 0, Args::INTEGER, 0, 1, 0, 13);
    args.option(u"isdbt-layer-c-segment-count", 0, Args::INTEGER, 0, 1, 0, 13);

    args.help(u"isdbt-layer-a-segment-count",
              u"Used for ISDB-T tuners only. Number of segments for layer A. "
              u"Possible values: 0 to 13. The default is automatically detected.");
    args.help(u"isdbt-layer-b-segment-count", u"Same as --isdbt-layer-a-segment-count for layer B.");
    args.help(u"isdbt-layer-c-segment-count", u"Same as --isdbt-layer-a-segment-count for layer C.");

    args.option(u"isdbt-layer-a-time-interleaving", 0, Args::INTEGER, 0, 1, 0, 3);
    args.option(u"isdbt-layer-b-time-interleaving", 0, Args::INTEGER, 0, 1, 0, 3);
    args.option(u"isdbt-layer-c-time-interleaving", 0, Args::INTEGER, 0, 1, 0, 3);

    args.help(u"isdbt-layer-a-time-interleaving",
              u"Used for ISDB-T tuners only. Time interleaving for layer A. "
              u"Possible values: 0 to 3. The default is automatically detected.");
    args.help(u"isdbt-layer-b-time-interleaving", u"Same as --isdbt-layer-a-time-interleaving for layer B.");
    args.help(u"isdbt-layer-c-time-interleaving", u"Same as --isdbt-layer-a-time-interleaving for layer C.");

    args.option(u"stream-id", 0, Args::UINT16);
    args.help(u"stream-id",
              u"Used for ISDB-S tuners only. "
              u"In the case of multi-stream broadcasting, specify the inner transport stream id. "
              u"By default, use the first inner transport stream, if any is found. "
              u"Warning: this option is supported on Linux only.");

    // UHF/VHF frequency bands options.
    args.option(u"uhf-channel", allow_short_options ? 'u' : 0, Args::POSITIVE);
    args.help(u"uhf-channel",
              u"Used for terrestrial tuners only. "
              u"Specify the UHF channel number of the carrier. "
              u"Can be used in replacement to --frequency. "
              u"Can be combined with an --offset-count option. "
              u"The UHF frequency layout depends on the region, see --hf-band-region option.");

    args.option(u"vhf-channel", allow_short_options ? 'v' : 0, Args::POSITIVE);
    args.help(u"vhf-channel",
              u"Used for terrestrial tuners only. "
              u"Specify the VHF channel number of the carrier. "
              u"Can be used in replacement to --frequency. "
              u"Can be combined with an --offset-count option. "
              u"The VHF frequency layout depends on the region, see --hf-band-region option.");

    args.option(u"offset-count", 0, Args::INTEGER, 0, 1, -10, 10);
    args.help(u"offset-count",
              u"Used for terrestrial tuners only. "
              u"Specify the number of offsets from the UHF or VHF channel. "
              u"The default is zero. See options --uhf-channel or --vhf-channel.");
}
