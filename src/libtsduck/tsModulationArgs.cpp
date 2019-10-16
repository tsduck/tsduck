//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2019, Thierry Lelegard
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
#include "tsDuckContext.h"
#include "tsDescriptor.h"
#include "tsHFBand.h"
#include "tsArgs.h"
#include "tsSysUtils.h"
#include "tsNullReport.h"
#include "tsBCD.h"
#include "tsDektec.h"
TSDUCK_SOURCE;

const ts::LNB& ts::ModulationArgs::DEFAULT_LNB(ts::LNB::Universal);

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr ts::SpectralInversion ts::ModulationArgs::DEFAULT_INVERSION;
constexpr ts::InnerFEC          ts::ModulationArgs::DEFAULT_INNER_FEC;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_SYMBOL_RATE_DVBS;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_SYMBOL_RATE_DVBC;
constexpr ts::Modulation        ts::ModulationArgs::DEFAULT_MODULATION_DVBS;
constexpr ts::Modulation        ts::ModulationArgs::DEFAULT_MODULATION_DVBT;
constexpr ts::Modulation        ts::ModulationArgs::DEFAULT_MODULATION_DVBC;
constexpr ts::Modulation        ts::ModulationArgs::DEFAULT_MODULATION_ATSC;
constexpr ts::BandWidth         ts::ModulationArgs::DEFAULT_BANDWIDTH_DVBT;
constexpr ts::InnerFEC          ts::ModulationArgs::DEFAULT_FEC_HP;
constexpr ts::InnerFEC          ts::ModulationArgs::DEFAULT_FEC_LP;
constexpr ts::TransmissionMode  ts::ModulationArgs::DEFAULT_TRANSMISSION_MODE_DVBT;
constexpr ts::GuardInterval     ts::ModulationArgs::DEFAULT_GUARD_INTERVAL_DVBT;
constexpr ts::Hierarchy         ts::ModulationArgs::DEFAULT_HIERARCHY;
constexpr ts::Polarization      ts::ModulationArgs::DEFAULT_POLARITY;
constexpr size_t                ts::ModulationArgs::DEFAULT_SATELLITE_NUMBER;
constexpr ts::Pilot             ts::ModulationArgs::DEFAULT_PILOTS;
constexpr ts::RollOff           ts::ModulationArgs::DEFAULT_ROLL_OFF;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_PLP;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_ISI;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_PLS_CODE;
constexpr ts::PLSMode           ts::ModulationArgs::DEFAULT_PLS_MODE;
#endif


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::ModulationArgs::ModulationArgs(bool allow_short_options) :
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
    _allow_short_options(allow_short_options)
{
}


//----------------------------------------------------------------------------
// Reset all values, they become "unset"
//----------------------------------------------------------------------------

void ts::ModulationArgs::reset()
{
    delivery_system.reset();
    frequency.reset();
    polarity.reset();
    lnb.reset();
    inversion.reset();
    symbol_rate.reset();
    inner_fec.reset();
    satellite_number.reset();
    modulation.reset();
    bandwidth.reset();
    fec_hp.reset();
    fec_lp.reset();
    transmission_mode.reset();
    guard_interval.reset();
    hierarchy.reset();
    pilots.reset();
    roll_off.reset();
    plp.reset();
    isi.reset();
    pls_code.reset();
    pls_mode.reset();
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
        pls_mode.set();
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
            // DVB-S2 and DVB-S/Turbo common options.
            modulation.setDefault(DEFAULT_MODULATION_DVBS);
            TS_FALLTHROUGH
        case DS_DVB_S:
            // DVB-S2, DVB-S/Turbo and DVB-S common options.
            frequency.setDefault(0);
            inversion.setDefault(DEFAULT_INVERSION);
            polarity.setDefault(DEFAULT_POLARITY);
            symbol_rate.setDefault(DEFAULT_SYMBOL_RATE_DVBS);
            inner_fec.setDefault(DEFAULT_INNER_FEC);
            lnb.setDefault(DEFAULT_LNB);
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
        case DS_DVB_C2:
        case DS_DVB_H:
        case DS_ISDB_S:
        case DS_ISDB_T:
        case DS_ISDB_C:
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
}


//----------------------------------------------------------------------------
// Check the validity of the delivery system or set a default one.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::resolveDeliverySystem(const DeliverySystemSet& systems, Report& report)
{
    if (delivery_system.set()) {
        if (systems.find(delivery_system.value()) == systems.end()) {
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

    return fec_div == 0 ? 0 : BitRate((uint64_t(symbol_rate) * bitpersym * fec_mul * 188) / (fec_div * 204));
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
                int brate = 0, mod = 0, param0 = 0, param1 = 0, param2 = 0;
                if (convertToDektecModulation(mod, param0, param1, param2) && Dtapi::DtapiModPars2TsRate(brate, mod, param0, param1, param2, int(symrate)) == DTAPI_OK) {
                    // Successfully found Dektec modulation parameters and computed TS bitrate
                    bitrate = BitRate(brate);
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
            const uint64_t bw = BandWidthValueHz(bandwidth.value(DEFAULT_BANDWIDTH_DVBT));

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

            bitrate = BitRate((423 * guard_div * bw * bitpersym * fec_mul) / (544 * (guard_div + guard_mul) * fec_div));
            break;
        }
        case DS_DVB_C_ANNEX_B:
        case DS_DVB_C2:
        case DS_DVB_H:
        case DS_ISDB_S:
        case DS_ISDB_T:
        case DS_ISDB_C:
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

bool ts::ModulationArgs::fromDeliveryDescriptor(const Descriptor& desc)
{
    // Completely clear previous content.
    reset();

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
                        roll_off = ROLLOFF_AUTO;
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
            break;
        }
        case DID_CABLE_DELIVERY: {
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
                    case 0:  bandwidth = BW_8_MHZ; break;
                    case 1:  bandwidth = BW_7_MHZ; break;
                    case 2:  bandwidth = BW_6_MHZ; break;
                    case 3:  bandwidth = BW_5_MHZ; break;
                    default: bandwidth = BW_AUTO; break;
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

ts::UString ts::ModulationArgs::shortDescription(DuckContext& duck, int strength, int quality) const
{
    // Strength and quality as a string.
    UString qual_string;
    if (strength >= 0) {
        qual_string = UString::Format(u"strength: %d%%", {strength});
    }
    if (quality >= 0) {
        if (!qual_string.empty()) {
            qual_string += u", ";
        }
        qual_string += UString::Format(u"quality: %d%%", {quality});
    }

    // Don't know what to describe without delivery system or frequency.
    if (!delivery_system.set() || !frequency.set()) {
        return qual_string;
    }

    UString desc;
    switch (TunerTypeOf(delivery_system.value())) {
        case TT_DVB_T: {
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
        case TT_DVB_S: {
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
            if (delivery_system != DS_DVB_S) {
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
        case TT_UNDEFINED:
        default: {
            // Generic display.
            desc = UString::Format(u"%'d Hz", {frequency.value()});
            break;
        }
    }

    // Final string.
    if (!qual_string.empty()) {
        desc += u", " + qual_string;
    }
    return desc;
}


//----------------------------------------------------------------------------
// Display a description of the paramters on a stream, line by line.
//----------------------------------------------------------------------------

void ts::ModulationArgs::display(std::ostream& strm, const ts::UString& margin, bool verbose) const
{
    if (frequency.set() && frequency != 0) {
        strm << margin << "Carrier frequency: " << UString::Decimal(frequency.value()) << " Hz" << std::endl;
    }
    if (inversion.set() && inversion != SPINV_AUTO) {
        strm << margin << "Spectral inversion: " << SpectralInversionEnum.name(inversion.value()) << std::endl;
    }
    if (modulation.set() && modulation != QAM_AUTO) {
        strm << margin << "Modulation: " << ModulationEnum.name(modulation.value()) << std::endl;
    }

    switch (TunerTypeOf(delivery_system.value())) {
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
            if (bandwidth.set() && bandwidth != BW_AUTO) {
                strm << margin << "Bandwidth: " << BandWidthEnum.name(bandwidth.value()) << std::endl;
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
            if (verbose) {
                strm << margin << "LNB: " << UString(lnb.value(DEFAULT_LNB)) << std::endl;
            }
            if (verbose) {
                strm << margin << "Satellite number: " << satellite_number.value(DEFAULT_SATELLITE_NUMBER) << std::endl;
            }
            break;
        }
        case TT_ATSC:
        case TT_UNDEFINED:
        default: {
            break;
        }
    }
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
                                   u" --bandwidth %s"
                                   u" --transmission-mode %s"
                                   u" --guard-interval %s"
                                   u" --hierarchy %s", {
                                   ModulationEnum.name(modulation.value(DEFAULT_MODULATION_DVBT)),
                                   InnerFECEnum.name(fec_hp.value(DEFAULT_FEC_HP)),
                                   InnerFECEnum.name(fec_lp.value(DEFAULT_FEC_LP)),
                                   BandWidthEnum.name(bandwidth.value(DEFAULT_BANDWIDTH_DVBT)),
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
                                   u" --modulation %s"
                                   u" --pilots %s"
                                   u" --roll-off %s", {
                                   symbol_rate.value(DEFAULT_SYMBOL_RATE_DVBS),
                                   InnerFECEnum.name(inner_fec.value(DEFAULT_INNER_FEC)),
                                   PolarizationEnum.name(polarity.value(DEFAULT_POLARITY)),
                                   ModulationEnum.name(modulation.value(DEFAULT_MODULATION_DVBS)),
                                   PilotEnum.name(pilots.value(DEFAULT_PILOTS)),
                                   RollOffEnum.name(roll_off.value(DEFAULT_ROLL_OFF))});
            if (isi.set() && isi != DEFAULT_ISI) {
                opt += UString::Format(u" --isi %d", {isi.value()});
            }
            if (pls_code.set() && pls_code != DEFAULT_PLS_CODE) {
                opt += UString::Format(u" --pls-code %d", {pls_code.value()});
            }
            if (pls_mode.set() && pls_mode != DEFAULT_PLS_MODE) {
                opt += UString::Format(u" --pls-mode %s", {PLSModeEnum.name(pls_mode.value())});
            }
            if (!no_local) {
                opt += UString::Format(u" --lnb %s --satellite-number %d", {UString(lnb.value(DEFAULT_LNB)), satellite_number.value()});
            }
            break;
        }
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
    reset();

    bool status = true;

    // If delivery system is unspecified, will use the default one for the tuner.
    if (args.present(u"delivery-system")) {
        delivery_system = args.enumValue<DeliverySystem>(u"delivery-system");
    }

    // Carrier frequency
    if (args.present(u"frequency") + args.present(u"uhf-channel") + args.present(u"vhf-channel") > 1) {
        args.error(u"options --frequency, --uhf-channel and --vhf-channel are mutually exclusive");
        status = false;
    }
    else if (args.present(u"frequency")) {
        frequency = args.intValue<uint64_t>(u"frequency");
    }
    else if (args.present(u"uhf-channel")) {
        frequency = duck.uhfBand()->frequency(args.intValue<uint32_t>(u"uhf-channel"));
    }
    else if (args.present(u"vhf-channel")) {
        frequency = duck.vhfBand()->frequency(args.intValue<uint32_t>(u"vhf-channel"));
    }

    // Other individual tuning options
    if (args.present(u"symbol-rate")) {
        symbol_rate = args.intValue<uint32_t>(u"symbol-rate");
    }
    if (args.present(u"polarity")) {
        polarity = args.enumValue<Polarization>(u"polarity");
    }
    if (args.present(u"spectral-inversion")) {
        inversion = args.enumValue<SpectralInversion>(u"spectral-inversion");
    }
    if (args.present(u"fec-inner")) {
        inner_fec = args.enumValue<InnerFEC>(u"fec-inner");
    }
    if (args.present(u"modulation")) {
        modulation = args.enumValue<Modulation>(u"modulation");
    }
    if (args.present(u"bandwidth")) {
        bandwidth = args.enumValue<BandWidth>(u"bandwidth");
    }
    if (args.present(u"high-priority-fec")) {
        fec_hp = args.enumValue<InnerFEC>(u"high-priority-fec");
    }
    if (args.present(u"low-priority-fec")) {
        fec_lp = args.enumValue<InnerFEC>(u"low-priority-fec");
    }
    if (args.present(u"transmission-mode")) {
        transmission_mode = args.enumValue<TransmissionMode>(u"transmission-mode");
    }
    if (args.present(u"guard-interval")) {
        guard_interval = args.enumValue<GuardInterval>(u"guard-interval");
    }
    if (args.present(u"hierarchy")) {
        hierarchy = args.enumValue<Hierarchy>(u"hierarchy");
    }
    if (args.present(u"pilots")) {
        pilots = args.enumValue<Pilot>(u"pilots");
    }
    if (args.present(u"roll-off")) {
        roll_off = args.enumValue<RollOff>(u"roll-off");
    }
    if (args.present(u"plp")) {
        plp = args.intValue<uint32_t>(u"plp");
    }
    if (args.present(u"isi")) {
        isi = args.intValue<uint32_t>(u"isi");
    }
    if (args.present(u"pls-code")) {
        pls_code = args.intValue<uint32_t>(u"pls-code");
    }
    if (args.present(u"pls-mode")) {
        pls_mode = args.enumValue<PLSMode>(u"pls-mode");
    }

    // Local options (not related to transponder)
    if (args.present(u"lnb")) {
        UString s(args.value(u"lnb"));
        LNB l(s);
        if (!l.isValid()) {
            args.error(u"invalid LNB description " + s);
            status = false;
        }
        else {
            lnb = l;
        }
    }
    if (args.present(u"satellite-number")) {
        satellite_number = args.intValue<size_t>(u"satellite-number");
    }

    return status;
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::ModulationArgs::defineArgs(Args& args) const
{
    args.option(u"delivery-system", 0, DeliverySystemEnum);
    args.help(u"delivery-system",
              u"Specify which delivery system to use. By default, use the default system for the tuner.");

    args.option(u"frequency", _allow_short_options ? 'f' : 0, Args::UNSIGNED);
    args.help(u"frequency", u"Carrier frequency in Hz (all tuners). There is no default.");

    args.option(u"polarity", 0, PolarizationEnum);
    args.help(u"polarity",
              u"Used for satellite tuners only. "
              u"Polarity. The default is \"vertical\".");

    args.option(u"lnb", 0, Args::STRING);
    args.help(u"lnb", u"low_freq[,high_freq,switch_freq]",
              u"Used for satellite tuners only. "
              u"Description of the LNB.  All frequencies are in MHz. "
              u"low_freq and high_freq are the frequencies of the local oscillators. "
              u"switch_freq is the limit between the low and high band. "
              u"high_freq and switch_freq are used for dual-band LNB's only. "
              u"The default is a universal LNB: low_freq = 9750 MHz, high_freq = 10600 MHz, switch_freq = 11700 MHz.");

    args.option(u"spectral-inversion", 0, SpectralInversionEnum);
    args.help(u"spectral-inversion",
              u"Spectral inversion. The default is \"auto\".");

    args.option(u"symbol-rate", _allow_short_options ? 's' : 0, Args::UNSIGNED);
    args.help(u"symbol-rate",
              u"Used for satellite and cable tuners only. "
              u"Symbol rate in symbols/second. The default is " + UString::Decimal(DEFAULT_SYMBOL_RATE_DVBS) +
              u" sym/s for satellite and " + UString::Decimal(DEFAULT_SYMBOL_RATE_DVBC) +
              u" sym/s for cable. ");

    args.option(u"fec-inner", 0, InnerFECEnum);
    args.help(u"fec-inner",
              u"Used for satellite and cable tuners only. Inner Forward Error Correction. "
              u"The default is \"auto\".");

    args.option(u"satellite-number", 0, Args::INTEGER, 0, 1, 0, 3);
    args.help(u"satellite-number",
              u"Used for satellite tuners only. Satellite/dish number. "
              u"Must be 0 to 3 with DiSEqC switches and 0 to 1 fornon-DiSEqC switches. The default is 0.");

    args.option(u"modulation", _allow_short_options ? 'm' : 0, ModulationEnum);
    args.help(u"modulation",
              u"Used for DVB-C, DVB-T, DVB-S2 and ATSC tuners. Modulation type. The default is \"" +
              ModulationEnum.name(DEFAULT_MODULATION_DVBT) + u"\" for DVB-T/T2, \"" +
              ModulationEnum.name(DEFAULT_MODULATION_DVBC) + u"\" for DVB-C, \"" +
              ModulationEnum.name(DEFAULT_MODULATION_DVBS) + u"\" for DVB-S2, \"" +
              ModulationEnum.name(DEFAULT_MODULATION_ATSC) + u"\" for ATSC.");

    args.option(u"bandwidth", 0, BandWidthEnum);
    args.help(u"bandwidth",
              u"Used for terrestrial tuners only. Bandwidth. The default is \"" +
              BandWidthEnum.name(DEFAULT_BANDWIDTH_DVBT) + u"\" for DVB-T/T2.");

    args.option(u"high-priority-fec", 0, InnerFECEnum);
    args.help(u"high-priority-fec",
              u"Used for DVB-T/T2 tuners only. Error correction for high priority streams. "
              u"The default is \"auto\".");

    args.option(u"low-priority-fec", 0, InnerFECEnum);
    args.help(u"low-priority-fec",
              u"Used for DVB-T/T2 tuners only. Error correction for low priority streams. "
              u"The default is \"auto\".");

    args.option(u"transmission-mode", 0, TransmissionModeEnum);
    args.help(u"transmission-mode",
              u"Used for terrestrial tuners only. Transmission mode. The default is \"" +
              TransmissionModeEnum.name(DEFAULT_TRANSMISSION_MODE_DVBT) + u"\" for DVB-T/T2.");

    args.option(u"guard-interval", 0, GuardIntervalEnum);
    args.help(u"guard-interval",
              u"Used for terrestrial tuners only. Guard interval. The default is \"" +
              GuardIntervalEnum.name(DEFAULT_GUARD_INTERVAL_DVBT) + u"\" for DVB-T/T2.");

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

    // UHF/VHF frequency bands options.
    args.option(u"uhf-channel", _allow_short_options ? 'u' : 0, Args::POSITIVE);
    args.help(u"uhf-channel",
              u"Used for terrestrial tuners only. "
              u"Specify the UHF channel number of the carrier. "
              u"Can be used in replacement to --frequency. "
              u"Can be combined with an --offset-count option. "
              u"The UHF frequency layout depends on the region, see --hf-band-region option.");

    args.option(u"vhf-channel", _allow_short_options ? 'v' : 0, Args::POSITIVE);
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
