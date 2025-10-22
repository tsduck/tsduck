//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsModulationArgs.h"
#include "tsLegacyBandWidth.h"
#include "tsDuckContext.h"
#include "tsDescriptor.h"
#include "tsHFBand.h"
#include "tsArgs.h"
#include "tsNullReport.h"
#include "tsjsonObject.h"
#include "tsCableDeliverySystemDescriptor.h"
#include "tsSatelliteDeliverySystemDescriptor.h"
#include "tsS2SatelliteDeliverySystemDescriptor.h"
#include "tsTerrestrialDeliverySystemDescriptor.h"
#include "tsISDBTerrestrialDeliverySystemDescriptor.h"
#include "tsDektecSupport.h"
#include "tsCerrReport.h"


//----------------------------------------------------------------------------
// Reset all values, they become "unset"
//----------------------------------------------------------------------------

void ts::ModulationArgs::clear()
{
    delivery_system.reset();
    frequency.reset();
    polarity.reset();
    lnb.reset();
    unicable.reset();
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
    sound_broadcasting.reset();
    sb_subchannel_id.reset();
    sb_segment_count.reset();
    sb_segment_index.reset();
    isdbt_layers.reset();
    isdbt_partial_reception.reset();
    layer_a_fec.reset();
    layer_a_modulation.reset();
    layer_a_segment_count.reset();
    layer_a_time_interleaving.reset();
    layer_b_fec.reset();
    layer_b_modulation.reset();
    layer_b_segment_count.reset();
    layer_b_time_interleaving.reset();
    layer_c_fec.reset();
    layer_c_modulation.reset();
    layer_c_segment_count.reset();
    layer_c_time_interleaving.reset();
    stream_id.reset();
}


//----------------------------------------------------------------------------
//! Reset or copy the local reception parameters.
//----------------------------------------------------------------------------

void ts::ModulationArgs::resetLocalReceptionParameters()
{
    lnb.reset();
    unicable.reset();
    satellite_number.reset();
}

void ts::ModulationArgs::copyLocalReceptionParameters(const ModulationArgs& other)
{
    if (other.lnb.has_value()) {
        lnb = other.lnb;
    }
    if (other.unicable.has_value()) {
        unicable = other.unicable;
    }
    if (other.satellite_number.has_value()) {
        satellite_number = other.satellite_number;
    }
}


//----------------------------------------------------------------------------
// Check if any modulation options is set.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::hasModulationArgs() const
{
    return
        delivery_system.has_value() ||
        frequency.has_value() ||
        polarity.has_value() ||
        lnb.has_value() ||
        unicable.has_value() ||
        inversion.has_value() ||
        symbol_rate.has_value() ||
        inner_fec.has_value() ||
        satellite_number.has_value() ||
        modulation.has_value() ||
        bandwidth.has_value() ||
        fec_hp.has_value() ||
        fec_lp.has_value() ||
        transmission_mode.has_value() ||
        guard_interval.has_value() ||
        hierarchy.has_value() ||
        pilots.has_value() ||
        roll_off.has_value() ||
        plp.has_value() ||
        isi.has_value() ||
        pls_code.has_value() ||
        pls_mode.has_value() ||
        sound_broadcasting.has_value() ||
        sb_subchannel_id.has_value() ||
        sb_segment_count.has_value() ||
        sb_segment_index.has_value() ||
        isdbt_layers.has_value() ||
        isdbt_partial_reception.has_value() ||
        layer_a_fec.has_value() ||
        layer_a_modulation.has_value() ||
        layer_a_segment_count.has_value() ||
        layer_a_time_interleaving.has_value() ||
        layer_b_fec.has_value() ||
        layer_b_modulation.has_value() ||
        layer_b_segment_count.has_value() ||
        layer_b_time_interleaving.has_value() ||
        layer_c_fec.has_value() ||
        layer_c_modulation.has_value() ||
        layer_c_segment_count.has_value() ||
        layer_c_time_interleaving.has_value() ||
        stream_id.has_value();
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
    switch (delivery_system.value_or(DS_UNDEFINED)) {
        case DS_DVB_S2:
            // DVB-S2 specific options.
            set_default(pilots, DEFAULT_PILOTS);
            set_default(roll_off, DEFAULT_ROLL_OFF);
            set_default(isi, DEFAULT_ISI);
            set_default(pls_code, DEFAULT_PLS_CODE);
            set_default(pls_mode, DEFAULT_PLS_MODE);
            [[fallthrough]];
        case DS_DVB_S_TURBO:
        case DS_DVB_S:
            // DVB-S2, DVB-S/Turbo and DVB-S common options.
            set_default(modulation, DEFAULT_MODULATION_DVBS);
            set_default(frequency, 0);
            set_default(inversion, DEFAULT_INVERSION);
            set_default(polarity, DEFAULT_POLARITY);
            set_default(symbol_rate, DEFAULT_SYMBOL_RATE_DVBS);
            set_default(inner_fec, DEFAULT_INNER_FEC);
            set_default(lnb, LNB(u"", NULLREP));
            set_default(satellite_number, DEFAULT_SATELLITE_NUMBER);
            break;
        case DS_DVB_T2:
            // DVB-S2 specific options.
            set_default(plp, DEFAULT_PLP);
            [[fallthrough]];
        case DS_DVB_T:
            // DVB-T2 and DVB-T common options.
            set_default(frequency, 0);
            set_default(inversion, DEFAULT_INVERSION);
            set_default(bandwidth, DEFAULT_BANDWIDTH_DVBT);
            set_default(fec_hp, DEFAULT_FEC_HP);
            set_default(fec_lp, DEFAULT_FEC_LP);
            set_default(modulation, DEFAULT_MODULATION_DVBT);
            set_default(transmission_mode, DEFAULT_TRANSMISSION_MODE_DVBT);
            set_default(guard_interval, DEFAULT_GUARD_INTERVAL_DVBT);
            set_default(hierarchy, DEFAULT_HIERARCHY);
            break;
        case DS_DVB_C_ANNEX_A:
        case DS_DVB_C_ANNEX_C:
            // DVB-C annex A,C common options (don't apply to annex B).
            set_default(inner_fec, DEFAULT_INNER_FEC);
            set_default(symbol_rate, DEFAULT_SYMBOL_RATE_DVBC);
            [[fallthrough]];
        case DS_DVB_C_ANNEX_B:
            // DVB-C annex A,B,C common options.
            set_default(frequency, 0);
            set_default(inversion, DEFAULT_INVERSION);
            set_default(modulation, DEFAULT_MODULATION_DVBC);
            break;
        case DS_ATSC:
            set_default(frequency, 0);
            set_default(inversion, DEFAULT_INVERSION);
            set_default(modulation, DEFAULT_MODULATION_ATSC);
            break;
        case DS_ISDB_S:
            set_default(frequency, 0);
            set_default(polarity, DEFAULT_POLARITY);
            set_default(lnb, LNB(u"", NULLREP));
            set_default(satellite_number, DEFAULT_SATELLITE_NUMBER);
            set_default(inversion, DEFAULT_INVERSION);
            set_default(symbol_rate, DEFAULT_SYMBOL_RATE_ISDBS);
            set_default(inner_fec, DEFAULT_INNER_FEC);
            break;
        case DS_ISDB_T:
            set_default(frequency, 0);
            set_default(inversion, DEFAULT_INVERSION);
            set_default(bandwidth, DEFAULT_BANDWIDTH_ISDBT);
            set_default(transmission_mode, DEFAULT_TRANSMISSION_MODE_ISDBT);
            set_default(guard_interval, DEFAULT_GUARD_INTERVAL_ISDBT);
            set_default(sound_broadcasting, false);
            set_default(sb_subchannel_id, DEFAULT_SB_SUBCHANNEL_ID);
            set_default(sb_segment_count, DEFAULT_SB_SEGMENT_COUNT);
            set_default(sb_segment_index, DEFAULT_SB_SEGMENT_INDEX);
            set_default(isdbt_layers, DEFAULT_ISDBT_LAYERS);
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
    if (delivery_system.has_value() && delivery_system.value() != DS_DVB_S2) {
        roll_off.reset();
    }
}


//----------------------------------------------------------------------------
// Check the validity of the delivery system or set a default one.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::resolveDeliverySystem(const DeliverySystemSet& systems, Report& report)
{
    if (delivery_system.has_value()) {
        if (!systems.contains(delivery_system.value())) {
            report.error(u"delivery system %s is not supported by this tuner", DeliverySystemEnum().name(delivery_system.value()));
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
            report.debug(u"using %s as default delivery system", DeliverySystemEnum().name(delivery_system.value()));
            return true;
        }
    }
}


//----------------------------------------------------------------------------
// Registration of BitRateCalculator functions.
//----------------------------------------------------------------------------

// Generic bitrate calculators, for all types of delivery systems.
ts::ModulationArgs::GenericCalculatorsData& ts::ModulationArgs::GenericCalculators()
{
    // Thread-safe init-safe static data pattern:
    static GenericCalculatorsData data;
    return data;
}

// Specialized bitrate calculators, for given types of delivery systems.
ts::ModulationArgs::SpecializedCalculatorsData& ts::ModulationArgs::SpecializedCalculators()
{
    // Thread-safe init-safe static data pattern:
    static SpecializedCalculatorsData data;
    return data;
}

// The constructor registers a new BitRateCalculator function.
ts::ModulationArgs::RegisterBitRateCalculator::RegisterBitRateCalculator(BitRateCalculator func, const DeliverySystemSet& systems)
{
    CERR.debug(u"registering bitrate calculator 0x%X for %d systems (%s)", uintptr_t(func), systems.size(), systems);
    if (systems.size() == 0) {
        GenericCalculators().insert(func);
    }
    else {
        for (auto ds : systems) {
            SpecializedCalculators().insert(std::make_pair(ds, func));
        }
    }
}


//----------------------------------------------------------------------------
// Bitrate calculator for QPSK or QAM modulations.
//----------------------------------------------------------------------------

// This is a private method, we cannot use the macro TS_REGISTER_BITRATE_CALCULATOR.
const ts::ModulationArgs::RegisterBitRateCalculator ts::ModulationArgs::_GetBitRateQAM(GetBitRateQAM, {DS_DVB_C_ANNEX_A, DS_DVB_C_ANNEX_C, DS_DVB_S, DS_ISDB_S});

bool ts::ModulationArgs::GetBitRateQAM(BitRate& bitrate, const ModulationArgs& args)
{
    Modulation modulation = QAM_AUTO;
    InnerFEC fec = FEC_AUTO;
    uint32_t symbol_rate = 0;

    if (args.delivery_system == DS_DVB_C_ANNEX_A || args.delivery_system == DS_DVB_C_ANNEX_C) {
        modulation = args.modulation.value_or(DEFAULT_MODULATION_DVBC);
        fec = args.inner_fec.value_or(DEFAULT_INNER_FEC);
        symbol_rate = args.symbol_rate.value_or(DEFAULT_SYMBOL_RATE_DVBC);
    }
    else if (args.delivery_system == DS_DVB_S) {
        modulation = args.modulation.value_or(DEFAULT_MODULATION_DVBS);
        fec = args.inner_fec.value_or(DEFAULT_INNER_FEC);
        symbol_rate = args.symbol_rate.value_or(DEFAULT_SYMBOL_RATE_DVBS);
    }
    else if (args.delivery_system == DS_ISDB_S) {
        // ISDB-S uses the Trellis coded 8-phase shift keying modulation.
        // For the sake of bitrate computation, this is the same as 8PSK.
        modulation = PSK_8;
        fec = args.inner_fec.value_or(DEFAULT_INNER_FEC);
        symbol_rate = args.symbol_rate.value_or(DEFAULT_SYMBOL_RATE_ISDBS);
    }
    else {
        return false;
    }

    const uint64_t bitpersym = BitsPerSymbol(modulation);
    const uint64_t fec_mul = FECMultiplier(fec);
    const uint64_t fec_div = FECDivider(fec);

    // Compute bitrate. The estimated bitrate is based on 204-bit packets (include 16-bit Reed-Solomon code).
    // We return a bitrate based on 188-bit packets.
    bitrate = fec_div == 0 ? 0 : BitRate(symbol_rate * bitpersym * fec_mul * 188) / BitRate(fec_div * 204);
    return bitrate > 0;
}


//----------------------------------------------------------------------------
// Bitrate calculator for DVB-T and DVB-T2.
//----------------------------------------------------------------------------

// This is a private method, we cannot use the macro TS_REGISTER_BITRATE_CALCULATOR.
const ts::ModulationArgs::RegisterBitRateCalculator ts::ModulationArgs::_GetBitRateDVBT(GetBitRateDVBT, {DS_DVB_T, DS_DVB_T2});

bool ts::ModulationArgs::GetBitRateDVBT(BitRate& bitrate, const ModulationArgs& args)
{
    if (args.delivery_system != DS_DVB_T && args.delivery_system != DS_DVB_T2) {
        return false;
    }

    // DVB-T2 and DVB-T common options.
    const uint64_t bitpersym = BitsPerSymbol(args.modulation.value_or(DEFAULT_MODULATION_DVBT));
    const uint64_t fec_mul = FECMultiplier(args.fec_hp.value_or(DEFAULT_FEC_HP));
    const uint64_t fec_div = FECDivider(args.fec_hp.value_or(DEFAULT_FEC_HP));
    const uint64_t guard_mul = GuardIntervalMultiplier(args.guard_interval.value_or(DEFAULT_GUARD_INTERVAL_DVBT));
    const uint64_t guard_div = GuardIntervalDivider(args.guard_interval.value_or(DEFAULT_GUARD_INTERVAL_DVBT));
    const uint64_t bw = args.bandwidth.value_or(DEFAULT_BANDWIDTH_DVBT);

    if (args.hierarchy.value_or(DEFAULT_HIERARCHY) != HIERARCHY_NONE || fec_div == 0 || guard_div == 0) {
        return false; // unknown bitrate
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
    // BR = useful bitrate
    //    = SR * BPS * FEC * 188/204
    //    = (SR * BPS * FECM * 188) / (FECD * 204)
    //    = (6048 * GID * BW * BPS * FECM * 188) / (7168 * (GID + GIM) * FECD * 204)
    //    = (1137024 * GID * BW * BPS * FECM) / (1462272 * (GID + GIM) * FECD)
    // And 1137024 / 1462272 = 423 / 544

    bitrate = BitRate(423 * guard_div * bw * bitpersym * fec_mul) / BitRate(544 * (guard_div + guard_mul) * fec_div);
    return true;
}


//----------------------------------------------------------------------------
// Bitrate calculator for ATSC.
//----------------------------------------------------------------------------

// This is a private method, we cannot use the macro TS_REGISTER_BITRATE_CALCULATOR.
const ts::ModulationArgs::RegisterBitRateCalculator ts::ModulationArgs::_GetBitRateATSC(GetBitRateATSC, {DS_ATSC});

bool ts::ModulationArgs::GetBitRateATSC(BitRate& bitrate, const ModulationArgs& args)
{
    if (args.delivery_system == DS_ATSC) {
        // Only two modulation values are available for ATSC.
        const Modulation mod = args.modulation.value_or(DEFAULT_MODULATION_ATSC);
        if (mod == VSB_8) {
            bitrate = 19'392'658;
            return true;
        }
        else if (mod == VSB_16) {
            bitrate = 38'785'317;
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Theoretical bitrate computation.
//----------------------------------------------------------------------------

ts::BitRate ts::ModulationArgs::theoreticalBitrate() const
{
    // Warning: This is a hack...
    // We implement a few bitrate calculators in this library. Some more complicated
    // modulations such as DVB-S2 are not supported here. However, the Dektec DTAPI
    // library contains more powerful ways of computing bitrates, including DVB-S2.
    // Because the DTAPI is not open-source, it has been included in an optional shared
    // library "libtsdektec" that open-source zealots may want to delete. Therefore,
    // unless you use the "dektec" plugin, this "libtsdektec", if present, is not loaded
    // inside the process space. By explicitly checking once if Dektec is supported, we
    // force the load of "libtsdektec". During the load, its initialization registers
    // its own generic bitrate calculator, which may support additional modulations.
    [[maybe_unused]] static const bool has_dektec = HasDektecSupport();

    BitRate bitrate = 0;

    // Try specialized calculators first.
    const auto bounds(SpecializedCalculators().equal_range(delivery_system.value_or(DS_UNDEFINED)));
    for (auto it = bounds.first; it != bounds.second; ++it) {
        if (it->second(bitrate, *this)) {
            return bitrate;
        }
    }

    // Then try generic calculators.
    for (auto func : GenericCalculators()) {
        if (func(bitrate, *this)) {
            return bitrate;
        }
    }

    // Don't know how to compute for that modulation.
    return 0;
}


//----------------------------------------------------------------------------
// Get parameters from delivery system descriptors in a descriptor list.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::fromDeliveryDescriptors(DuckContext& duck, const DescriptorList& dlist, uint16_t ts_id, DeliverySystem delsys)
{
    // We need to explore all descriptors. We cannot stop at the first delivery system descriptor
    // because some of them are incremental (eg. DVB-S2). We accumulate values from all of them.
    bool found = false;
    for (size_t i = 0; i < dlist.count(); ++i) {
        found = fromDeliveryDescriptor(duck, dlist[i], ts_id, delsys) || found;
    }
    return found;
}


//----------------------------------------------------------------------------
// Fill modulation parameters from a delivery system descriptor.
//----------------------------------------------------------------------------

bool ts::ModulationArgs::fromDeliveryDescriptor(DuckContext& duck, const Descriptor& desc, uint16_t ts_id, DeliverySystem delsys)
{
    switch (desc.tag()) {
        case DID_DVB_SAT_DELIVERY: {
            // DVB or ISDB satellite delivery network.
            // The descriptor can be used in either DVB or ISDB context.
            // There is no way to distinguish a DVB and an ISDB version without the "duck" context.
            const SatelliteDeliverySystemDescriptor dd(duck, desc);
            if (dd.isValid()) {
                delivery_system = dd.deliverySystem(duck);
                frequency = dd.frequency;
                symbol_rate = dd.symbol_rate;
                polarity = dd.getPolarization();
                inner_fec = dd.getInnerFEC();
                modulation = dd.getModulation();
                if (delivery_system == DS_DVB_S2) {
                    roll_off = dd.getRollOff();
                }
                else {
                    roll_off.reset();
                }
                if (delivery_system == DS_ISDB_S) {
                    // The TS id is used in ISDB-S multi-stream encapsulation.
                    stream_id = ts_id;
                }
                return true;
            }
            break;
        }
        case DID_DVB_S2_SAT_DELIVERY: {
            // Usually comes in addition to a SatelliteDeliverySystemDescriptor.
            const S2SatelliteDeliverySystemDescriptor dd(duck, desc);
            if (dd.isValid()) {
                delivery_system = dd.deliverySystem(duck);
                if (dd.input_stream_identifier.has_value()) {
                    isi = dd.input_stream_identifier.value();
                }
                if (dd.scrambling_sequence_index.has_value()) {
                    pls_mode = PLS_GOLD;
                    pls_code = dd.scrambling_sequence_index.value();
                }
                return true;
            }
            break;
        }
        case DID_DVB_CABLE_DELIVERY: {
            const CableDeliverySystemDescriptor dd(duck, desc);
            if (dd.isValid()) {
                // Scanning a NIT on DVB-C networks has a specific issue.
                // There are three types of DVB-C: /A, /B, /C. However, the cable_delivery_system_descriptor
                // does not specify which one is used. If we want to tune to this transport stream, we need
                // to specify the right flavor of DVB-C. It must be the same as the one from which this
                // descriptor was extracted, otherwise no set-top box could scan the network using the NIT.
                if (delsys == DS_DVB_C_ANNEX_A || delsys == DS_DVB_C_ANNEX_B || delsys == DS_DVB_C_ANNEX_C) {
                    // Use the same flavor of DVB-C as current TS.
                    delivery_system = delsys;
                }
                else {
                    // Use the default one from the descriptor, tune may fail if not DVB-C/A.
                    delivery_system = dd.deliverySystem(duck);
                }
                frequency = dd.frequency;
                symbol_rate = dd.symbol_rate;
                inner_fec = dd.getInnerFEC();
                modulation = dd.getModulation();
                return true;
            }
            break;
        }
        case DID_DVB_TERREST_DELIVERY: {
            const TerrestrialDeliverySystemDescriptor dd(duck, desc);
            if (dd.isValid()) {
                delivery_system = dd.deliverySystem(duck);
                frequency = dd.centre_frequency;
                bandwidth = dd.getBandwidth();
                modulation = dd.getConstellation();
                fec_lp = dd.getCodeRateLP();
                fec_hp = dd.getCodeRateHP();
                transmission_mode = dd.getTransmissionMode();
                guard_interval = dd.getGuardInterval();
                hierarchy = dd.getHierarchy();
                return true;
            }
            break;
        }
        case DID_ISDB_TERRES_DELIV:  {
            const ISDBTerrestrialDeliverySystemDescriptor dd(duck, desc);
            if (dd.isValid()) {
                delivery_system = dd.deliverySystem(duck);
                transmission_mode = dd.getTransmissionMode();
                guard_interval = dd.getGuardInterval();
                if (dd.frequencies.empty()) {
                    frequency.reset();
                }
                else {
                    // Use first frequency only.
                    frequency = dd.frequencies.front();
                }
                return true;
            }
            break;
        }
        default: {
            // Not a valid delivery descriptor.
            break;
        }
    }

    return false;
}


//----------------------------------------------------------------------------
// Format a short description (frequency and essential parameters).
//----------------------------------------------------------------------------

ts::UString ts::ModulationArgs::shortDescription(DuckContext& duck) const
{
    // Don't know what to describe without delivery system or frequency.
    if (!delivery_system.has_value() || !frequency.has_value()) {
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
                desc += UString::Format(u"%s channel %d", band, channel);
                if (offset != 0) {
                    desc += UString::Format(u", offset %+d", offset);
                }
                desc += u" (";
            }
            desc += UString::Format(u"%'d Hz", frequency.value());
            if (band != nullptr) {
                desc += u")";
            }

            if (plp.has_value() && plp != PLP_DISABLE) {
                desc += UString::Format(u", PLP %d", plp.value());
            }
            break;
        }
        case TT_DVB_S:
        case TT_ISDB_S: {
            // Display frequency and polarity.
            desc = UString::Format(u"%'d Hz", frequency.value());
            if (polarity.has_value()) {
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
                desc += u" (" + DeliverySystemEnum().name(delivery_system.value());
                if (modulation.has_value() && modulation != QAM_AUTO) {
                    desc += u", " + ModulationEnum().name(modulation.value());
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
            desc = UString::Format(u"%'d Hz", frequency.value());
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

    if (delivery_system.has_value()) {
        strm << margin << "Delivery system: " << DeliverySystemEnum().name(delivery_system.value()) << std::endl;
    }
    if (frequency.has_value() && *frequency != 0) {
        strm << margin << "Carrier frequency: " << UString::Decimal(frequency.value()) << " Hz" << std::endl;
    }
    if (inversion.has_value() && inversion != SPINV_AUTO) {
        strm << margin << "Spectral inversion: " << SpectralInversionEnum().name(inversion.value()) << std::endl;
    }
    if (modulation.has_value() && modulation != QAM_AUTO) {
        strm << margin << "Modulation: " << ModulationEnum().name(modulation.value()) << std::endl;
    }

    switch (TunerTypeOf(delivery_system.value_or(DS_UNDEFINED))) {
        case TT_DVB_C: {
            if (symbol_rate.has_value() && *symbol_rate != 0) {
                strm << margin << "Symbol rate: " << UString::Decimal(symbol_rate.value()) << " symb/s" << std::endl;
            }
            if (inner_fec.has_value() && inner_fec != FEC_AUTO) {
                strm << margin << "FEC inner: " << InnerFECEnum().name(inner_fec.value()) << std::endl;
            }
            break;
        }
        case TT_DVB_T: {
            if (fec_hp.has_value() && fec_hp != FEC_AUTO) {
                strm << margin << "HP streams FEC: " << InnerFECEnum().name(fec_hp.value()) << std::endl;
            }
            if (fec_lp.has_value() && fec_lp != FEC_AUTO) {
                strm << margin << "LP streams FEC: " << InnerFECEnum().name(fec_lp.value()) << std::endl;
            }
            if (guard_interval.has_value() && guard_interval != GUARD_AUTO) {
                strm << margin << "Guard interval: " << GuardIntervalEnum().name(guard_interval.value()) << std::endl;
            }
            if (bandwidth.has_value() && *bandwidth != 0) {
                strm << margin << "Bandwidth: " << UString::Decimal(bandwidth.value()) << " Hz" << std::endl;
            }
            if (transmission_mode.has_value() && transmission_mode != TM_AUTO) {
                strm << margin << "Transmission mode: " << TransmissionModeEnum().name(transmission_mode.value()) << std::endl;
            }
            if (hierarchy.has_value() && hierarchy != HIERARCHY_AUTO) {
                strm << margin << "Hierarchy: " << HierarchyEnum().name(hierarchy.value()) << std::endl;
            }
            break;
        }
        case TT_DVB_S: {
            if (polarity.has_value() && polarity != POL_AUTO) {
                strm << margin << "Polarity: " << PolarizationEnum().name(polarity.value()) << std::endl;
            }
            if (inversion.has_value() && inversion != SPINV_AUTO) {
                strm << margin << "Spectral inversion: " << SpectralInversionEnum().name(inversion.value()) << std::endl;
            }
            if (symbol_rate.has_value() && *symbol_rate != 0) {
                strm << margin << "Symbol rate: " << UString::Decimal(symbol_rate.value()) << " symb/s" << std::endl;
            }
            if (inner_fec.has_value() && inner_fec != ts::FEC_AUTO) {
                strm << margin << "FEC inner: " << InnerFECEnum().name(inner_fec.value()) << std::endl;
            }
            if ((verbose || delivery_system != DS_DVB_S) && isi.has_value() && isi != ISI_DISABLE) {
                strm << margin << "Input stream id: " << isi.value() << std::endl
                     << margin << "PLS code: " << pls_code.value_or(DEFAULT_PLS_CODE) << std::endl
                     << margin << "PLS mode: "<< PLSModeEnum().name(pls_mode.value_or(DEFAULT_PLS_MODE)) << std::endl;
            }
            if ((verbose || delivery_system != DS_DVB_S) && pilots.has_value() && pilots != PILOT_AUTO) {
                strm << margin << "Pilots: " << PilotEnum().name(pilots.value()) << std::endl;
            }
            if ((verbose || delivery_system != DS_DVB_S) && roll_off.has_value() && roll_off != ROLLOFF_AUTO) {
                strm << margin << "Roll-off: " << RollOffEnum().name(roll_off.value()) << std::endl;
            }
            if (verbose && lnb.has_value()) {
                strm << margin << "LNB: " << lnb.value() << std::endl;
            }
            if (verbose && unicable.has_value()) {
                strm << margin << "Unicable: " << unicable.value() << std::endl;
            }
            if (verbose) {
                strm << margin << "Satellite number: " << satellite_number.value_or(DEFAULT_SATELLITE_NUMBER) << std::endl;
            }
            break;
        }
        case TT_ISDB_S: {
            if (polarity.has_value() && polarity != POL_AUTO) {
                strm << margin << "Polarity: " << PolarizationEnum().name(polarity.value()) << std::endl;
            }
            if (inversion.has_value() && inversion != SPINV_AUTO) {
                strm << margin << "Spectral inversion: " << SpectralInversionEnum().name(inversion.value()) << std::endl;
            }
            if (symbol_rate.has_value() && *symbol_rate != 0) {
                strm << margin << "Symbol rate: " << UString::Decimal(symbol_rate.value()) << " symb/s" << std::endl;
            }
            if (stream_id.has_value()) {
                strm << margin << "Innert transport stream id: " << stream_id.value() << std::endl;
            }
            if (inner_fec.has_value() && inner_fec != ts::FEC_AUTO) {
                strm << margin << "FEC inner: " << InnerFECEnum().name(inner_fec.value()) << std::endl;
            }
            if (verbose && lnb.has_value()) {
                strm << margin << "LNB: " << lnb.value() << std::endl;
            }
            if (verbose) {
                strm << margin << "Satellite number: " << satellite_number.value_or(DEFAULT_SATELLITE_NUMBER) << std::endl;
            }
            break;
        }
        case TT_ISDB_T: {
            if (guard_interval.has_value() && guard_interval != GUARD_AUTO) {
                strm << margin << "Guard interval: " << GuardIntervalEnum().name(guard_interval.value()) << std::endl;
            }
            if (bandwidth.has_value() && *bandwidth != 0) {
                strm << margin << "Bandwidth: " << UString::Decimal(bandwidth.value()) << " Hz" << std::endl;
            }
            if (transmission_mode.has_value() && transmission_mode != TM_AUTO) {
                strm << margin << "Transmission mode: " << TransmissionModeEnum().name(transmission_mode.value()) << std::endl;
            }
            if (sound_broadcasting == true) {
                strm << margin << "Sound broadcasting: on" << std::endl;
                if (sb_subchannel_id.has_value()) {
                    strm << margin << "- Sub-channel id: " << sb_subchannel_id.value() << std::endl;
                }
                if (sb_segment_count.has_value()) {
                    strm << margin << "- Segment count: " << sb_segment_count.value() << std::endl;
                }
                if (sb_segment_index.has_value()) {
                    strm << margin << "- Segment index: " << sb_segment_index.value() << std::endl;
                }
            }
            if (isdbt_layers.has_value()) {
                strm << margin << "Layers: " << (isdbt_layers.value().empty() ? u"none" : isdbt_layers.value()) << std::endl;
            }
            if (isdbt_partial_reception.has_value()) {
                strm << margin << "Partial reception: " << UString::OnOff(isdbt_partial_reception.value()) << std::endl;
            }
            if (layer_a_fec.has_value() && layer_a_fec != FEC_AUTO) {
                strm << margin << "Layer A FEC: " << InnerFECEnum().name(layer_a_fec.value()) << std::endl;
            }
            if (layer_a_modulation.has_value() && layer_a_modulation != QAM_AUTO) {
                strm << margin << "Layer A modulation: " << ModulationEnum().name(layer_a_modulation.value()) << std::endl;
            }
            if (layer_a_segment_count.has_value() && layer_a_segment_count.value() <= MAX_ISDBT_SEGMENT_COUNT) {
                strm << margin << "Layer A segment count: " << layer_a_segment_count.value() << std::endl;
            }
            if (layer_a_time_interleaving.has_value() && IsValidISDBTTimeInterleaving(layer_a_time_interleaving.value())) {
                strm << margin << "Layer A time interleaving: " << layer_a_time_interleaving.value() << std::endl;
            }
            if (layer_b_fec.has_value() && layer_b_fec != FEC_AUTO) {
                strm << margin << "Layer B FEC: " << InnerFECEnum().name(layer_b_fec.value()) << std::endl;
            }
            if (layer_b_modulation.has_value() && layer_b_modulation != QAM_AUTO) {
                strm << margin << "Layer B modulation: " << ModulationEnum().name(layer_b_modulation.value()) << std::endl;
            }
            if (layer_b_segment_count.has_value() && layer_b_segment_count.value() <= MAX_ISDBT_SEGMENT_COUNT) {
                strm << margin << "Layer B segment count: " << layer_b_segment_count.value() << std::endl;
            }
            if (layer_b_time_interleaving.has_value() && IsValidISDBTTimeInterleaving(layer_b_time_interleaving.value())) {
                strm << margin << "Layer B time interleaving: " << layer_b_time_interleaving.value() << std::endl;
            }
            if (layer_c_fec.has_value() && layer_c_fec != FEC_AUTO) {
                strm << margin << "Layer C FEC: " << InnerFECEnum().name(layer_c_fec.value()) << std::endl;
            }
            if (layer_c_modulation.has_value() && layer_c_modulation != QAM_AUTO) {
                strm << margin << "Layer C modulation: " << ModulationEnum().name(layer_c_modulation.value()) << std::endl;
            }
            if (layer_c_segment_count.has_value() && layer_c_segment_count.value() <= MAX_ISDBT_SEGMENT_COUNT) {
                strm << margin << "Layer C segment count: " << layer_c_segment_count.value() << std::endl;
            }
            if (layer_c_time_interleaving.has_value() && IsValidISDBTTimeInterleaving(layer_c_time_interleaving.value())) {
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
    if (!delivery_system.has_value() || !frequency.has_value()) {
        return UString();
    }

    // Delivery system and frequency are common options and always come first.
    UString opt(UString::Format(u"--delivery-system %s --frequency %'d", DeliverySystemEnum().name(delivery_system.value()), frequency.value()));

    // All other options depend on the tuner type.
    switch (TunerTypeOf(delivery_system.value())) {
        case TT_ATSC: {
            opt += UString::Format(u" --modulation %s", ModulationEnum().name(modulation.value_or(DEFAULT_MODULATION_ATSC)));
            break;
        }
        case TT_DVB_C: {
            opt += UString::Format(u" --symbol-rate %'d --fec-inner %s --modulation %s",
                                   symbol_rate.value_or(DEFAULT_SYMBOL_RATE_DVBC),
                                   InnerFECEnum().name(inner_fec.value_or(DEFAULT_INNER_FEC)),
                                   ModulationEnum().name(modulation.value_or(DEFAULT_MODULATION_DVBC)));
            break;
        }
        case TT_DVB_T: {
            opt += UString::Format(u" --modulation %s"
                                   u" --high-priority-fec %s"
                                   u" --low-priority-fec %s"
                                   u" --bandwidth %'d"
                                   u" --transmission-mode %s"
                                   u" --guard-interval %s"
                                   u" --hierarchy %s",
                                   ModulationEnum().name(modulation.value_or(DEFAULT_MODULATION_DVBT)),
                                   InnerFECEnum().name(fec_hp.value_or(DEFAULT_FEC_HP)),
                                   InnerFECEnum().name(fec_lp.value_or(DEFAULT_FEC_LP)),
                                   bandwidth.value_or(DEFAULT_BANDWIDTH_DVBT),
                                   TransmissionModeEnum().name(transmission_mode.value_or(DEFAULT_TRANSMISSION_MODE_DVBT)),
                                   GuardIntervalEnum().name(guard_interval.value_or(DEFAULT_GUARD_INTERVAL_DVBT)),
                                   HierarchyEnum().name(hierarchy.value_or(DEFAULT_HIERARCHY)));
            if (plp.has_value() && plp != PLP_DISABLE) {
                opt += UString::Format(u" --plp %d", plp.value());
            }
            break;
        }
        case TT_DVB_S: {
            opt += UString::Format(u" --symbol-rate %'d"
                                   u" --fec-inner %s"
                                   u" --polarity %s"
                                   u" --modulation %s",
                                   symbol_rate.value_or(DEFAULT_SYMBOL_RATE_DVBS),
                                   InnerFECEnum().name(inner_fec.value_or(DEFAULT_INNER_FEC)),
                                   PolarizationEnum().name(polarity.value_or(DEFAULT_POLARITY)),
                                   ModulationEnum().name(modulation.value_or(DEFAULT_MODULATION_DVBS)));
            if (delivery_system == DS_DVB_S2) {
                opt += UString::Format(u" --pilots %s --roll-off %s",
                                       PilotEnum().name(pilots.value_or(DEFAULT_PILOTS)),
                                       RollOffEnum().name(roll_off.value_or(DEFAULT_ROLL_OFF)));
                if (isi.has_value() && isi != DEFAULT_ISI) {
                    opt += UString::Format(u" --isi %d", isi.value());
                }
                if (pls_code.has_value() && pls_code != DEFAULT_PLS_CODE) {
                    opt += UString::Format(u" --pls-code %d", pls_code.value());
                }
                if (pls_mode.has_value() && pls_mode != DEFAULT_PLS_MODE) {
                    opt += UString::Format(u" --pls-mode %s", PLSModeEnum().name(pls_mode.value()));
                }
            }
            if (!no_local && lnb.has_value()) {
                opt += UString::Format(u" --lnb %s", lnb.value());
            }
            if (!no_local && unicable.has_value()) {
                opt += UString::Format(u" --unicable %s", unicable.value());
            }
            if (!no_local && satellite_number.has_value()) {
                opt += UString::Format(u" --satellite-number %d", satellite_number.value());
            }
            break;
        }
        case TT_ISDB_S: {
            opt += UString::Format(u" --symbol-rate %'d --fec-inner %s --polarity %s",
                                   symbol_rate.value_or(DEFAULT_SYMBOL_RATE_ISDBS),
                                   InnerFECEnum().name(inner_fec.value_or(DEFAULT_INNER_FEC)),
                                   PolarizationEnum().name(polarity.value_or(DEFAULT_POLARITY)));
            if (stream_id.has_value() && stream_id != DEFAULT_STREAM_ID) {
                opt += UString::Format(u" --stream-id %d", stream_id.value());
            }
            if (!no_local && lnb.has_value()) {
                opt += UString::Format(u" --lnb %s", lnb.value());
            }
            if (!no_local && satellite_number.has_value()) {
                opt += UString::Format(u" --satellite-number %d", satellite_number.value());
            }
            break;
        }
        case TT_ISDB_T: {
            opt += UString::Format(u" --bandwidth %'d --transmission-mode %s --guard-interval %s",
                                   bandwidth.value_or(DEFAULT_BANDWIDTH_ISDBT),
                                   TransmissionModeEnum().name(transmission_mode.value_or(DEFAULT_TRANSMISSION_MODE_DVBT)),
                                   GuardIntervalEnum().name(guard_interval.value_or(DEFAULT_GUARD_INTERVAL_DVBT)));
            if (sound_broadcasting == true) {
                opt += UString::Format(u" --sound-broadcasting --sb-subchannel-id %d --sb-segment-count %d --sb-segment-index %d",
                                       sb_subchannel_id.value_or(DEFAULT_SB_SUBCHANNEL_ID),
                                       sb_segment_count.value_or(DEFAULT_SB_SEGMENT_COUNT),
                                       sb_segment_index.value_or(DEFAULT_SB_SEGMENT_INDEX));
            }
            if (isdbt_partial_reception == true) {
                opt += u" --isdbt-partial-reception";
            }
            if (!isdbt_layers.has_value() || !isdbt_layers.value().empty()) {
                opt += UString::Format(u" --isdbt-layers \"%s\"", isdbt_layers.value_or(DEFAULT_ISDBT_LAYERS));
            }
            if (layer_a_fec.has_value() && layer_a_fec != FEC_AUTO) {
                opt += UString::Format(u" --isdbt-layer-a-fec %s", InnerFECEnum().name(layer_a_fec.value()));
            }
            if (layer_a_modulation.has_value() && layer_a_modulation != QAM_AUTO) {
                opt += UString::Format(u" --isdbt-layer-a-modulation %s", ModulationEnum().name(layer_a_modulation.value()));
            }
            if (layer_a_segment_count.has_value()) {
                opt += UString::Format(u" --isdbt-layer-a-segment-count %d", layer_a_segment_count.value());
            }
            if (layer_a_time_interleaving.has_value()) {
                opt += UString::Format(u" --isdbt-layer-a-time-interleaving %d", layer_a_time_interleaving.value());
            }
            if (layer_b_fec.has_value() && layer_b_fec != FEC_AUTO) {
                opt += UString::Format(u" --isdbt-layer-b-fec %s", InnerFECEnum().name(layer_b_fec.value()));
            }
            if (layer_b_modulation.has_value() && layer_b_modulation != QAM_AUTO) {
                opt += UString::Format(u" --isdbt-layer-b-modulation %s", ModulationEnum().name(layer_b_modulation.value()));
            }
            if (layer_b_segment_count.has_value()) {
                opt += UString::Format(u" --isdbt-layer-b-segment-count %d", layer_b_segment_count.value());
            }
            if (layer_b_time_interleaving.has_value()) {
                opt += UString::Format(u" --isdbt-layer-b-time-interleaving %d", layer_b_time_interleaving.value());
            }
            if (layer_c_fec.has_value() && layer_c_fec != FEC_AUTO) {
                opt += UString::Format(u" --isdbt-layer-c-fec %s", InnerFECEnum().name(layer_c_fec.value()));
            }
            if (layer_c_modulation.has_value() && layer_c_modulation != QAM_AUTO) {
                opt += UString::Format(u" --isdbt-layer-c-modulation %s", ModulationEnum().name(layer_c_modulation.value()));
            }
            if (layer_c_segment_count.has_value()) {
                opt += UString::Format(u" --isdbt-layer-c-segment-count %d", layer_c_segment_count.value());
            }
            if (layer_c_time_interleaving.has_value()) {
                opt += UString::Format(u" --isdbt-layer-c-time-interleaving %d", layer_c_time_interleaving.value());
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
    if (inversion.has_value() && inversion != DEFAULT_INVERSION) {
        opt += u" --spectral-inversion ";
        opt += SpectralInversionEnum().name(inversion.value());
    }

    return opt;
}


//----------------------------------------------------------------------------
// Format the modulation parameters in a JSON object.
//----------------------------------------------------------------------------

void ts::ModulationArgs::toJSON(json::Object& obj) const
{
    // Don't know what to describe without delivery system or frequency.
    if (!delivery_system.has_value() || !frequency.has_value()) {
        return;
    }

    obj.add(u"delivery-system", DeliverySystemEnum().name(delivery_system.value()));
    obj.add(u"frequency", frequency.value());

    const BitRate tuner_bitrate = theoreticalBitrate();
    if (tuner_bitrate > 0) {
        obj.add(u"theoretical-bitrate", tuner_bitrate.toString());
    }

    if (modulation.has_value()) {
        obj.add(u"modulation", ModulationEnum().name(modulation.value()));
    }
    if (symbol_rate.has_value()) {
        obj.add(u"symbol-rate", symbol_rate.value());
    }
    if (inner_fec.has_value()) {
        obj.add(u"fec-inner", InnerFECEnum().name(inner_fec.value()));
    }
    if (fec_hp.has_value()) {
        obj.add(u"high-priority-fec", InnerFECEnum().name(fec_hp.value()));
    }
    if (fec_lp.has_value()) {
        obj.add(u"low-priority-fec", InnerFECEnum().name(fec_lp.value()));
    }
    if (bandwidth.has_value()) {
        obj.add(u"bandwidth", bandwidth.value());
    }
    if (transmission_mode.has_value()) {
        obj.add(u"transmission-mode", TransmissionModeEnum().name(transmission_mode.value()));
    }
    if (guard_interval.has_value()) {
        obj.add(u"guard-interval", GuardIntervalEnum().name(guard_interval.value()));
    }
    if (hierarchy.has_value()) {
        obj.add(u"hierarchy", HierarchyEnum().name(hierarchy.value()));
    }
    if (polarity.has_value()) {
        obj.add(u"polarity", PolarizationEnum().name(polarity.value()));
    }
    if (delivery_system == DS_DVB_S2) {
        if (pilots.has_value()) {
            obj.add(u"pilots", PilotEnum().name(pilots.value()));
        }
        if (roll_off.has_value()) {
            obj.add(u"roll-off", RollOffEnum().name(roll_off.value()));
        }
    }
    if (stream_id.has_value() && stream_id != DEFAULT_STREAM_ID) {
        obj.add(u"stream-id", stream_id.value());
    }
    if (inversion.has_value() && inversion != DEFAULT_INVERSION) {
        obj.add(u"spectral-inversion", SpectralInversionEnum().name(inversion.value()));
    }
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
        const uint32_t channel = args.intValue<uint32_t>(u"uhf-channel");
        status = duck.uhfBand()->isValidChannel(channel, args) && status;
        frequency = duck.uhfBand()->frequency(channel, args.intValue<int32_t>(u"offset-count"));
    }
    else if (args.present(u"vhf-channel")) {
        const uint32_t channel = args.intValue<uint32_t>(u"vhf-channel");
        status = duck.vhfBand()->isValidChannel(channel, args) && status;
        frequency = duck.vhfBand()->frequency(channel, args.intValue<int32_t>(u"offset-count"));
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
            args.debug(u"loaded LNB \"%s\" from command line", l);
            lnb = l;
        }
    }
    if (args.present(u"unicable")) {
        Unicable uc;
        if (uc.decode(args.value(u"unicable"), duck.report())) {
            unicable = uc;
        }
        else {
            status = false;
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
    args.option(u"delivery-system", 0, DeliverySystemEnum());
    args.help(u"delivery-system",
              u"Specify which delivery system to use. By default, use the default system for the tuner.");

    args.option(u"frequency", allow_short_options ? 'f' : 0, Args::UNSIGNED);
    args.help(u"frequency", u"Carrier frequency in Hz (all tuners). There is no default.");

    args.option(u"polarity", 0, PolarizationEnum());
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

    args.option(u"unicable", 0, Args::STRING);
    args.help(u"unicable",
              u"Used for satellite tuners only, in local Unicable distribution networks. "
              u"Description of the Unicable switch. " + Unicable::StringFormat());

    args.option(u"spectral-inversion", 0, SpectralInversionEnum());
    args.help(u"spectral-inversion",
              u"Spectral inversion. The default is \"auto\".");

    args.option(u"symbol-rate", allow_short_options ? 's' : 0, Args::UNSIGNED);
    args.help(u"symbol-rate",
              u"Used for satellite and cable tuners only. "
              u"Symbol rate in symbols/second. The default is " +
              UString::Decimal(DEFAULT_SYMBOL_RATE_DVBS) + u" sym/s for DVB-S, " +
              UString::Decimal(DEFAULT_SYMBOL_RATE_DVBC) + u" sym/s for DVB-C, " +
              UString::Decimal(DEFAULT_SYMBOL_RATE_ISDBS) + u" sym/s for ISDB-S, ");

    args.option(u"fec-inner", 0, InnerFECEnum());
    args.help(u"fec-inner",
              u"Used for satellite and cable tuners only. Inner Forward Error Correction. "
              u"The default is \"auto\".");

    args.option(u"satellite-number", 0, Args::INTEGER, 0, 1, 0, 63);
    args.help(u"satellite-number",
              u"Used for satellite tuners only. Satellite/dish number. "
              u"Must be 0 to 63 with DiSEqC switches and 0 to 1 for non-DiSEqC switches. The default is 0. "
              u"If you have cascaded switches, it is assumed that the DiSEqC 1.1 switch is nearest to the receiver.");

    args.option(u"modulation", allow_short_options ? 'm' : 0, ModulationEnum());
    args.help(u"modulation",
              u"Used for DVB-C, DVB-T, DVB-S2 and ATSC tuners. Modulation type. The default is \"" +
              ModulationEnum().name(DEFAULT_MODULATION_DVBT) + u"\" for DVB-T/T2, \"" +
              ModulationEnum().name(DEFAULT_MODULATION_DVBC) + u"\" for DVB-C, \"" +
              ModulationEnum().name(DEFAULT_MODULATION_DVBS) + u"\" for DVB-S2, \"" +
              ModulationEnum().name(DEFAULT_MODULATION_ATSC) + u"\" for ATSC.");

    args.option(u"high-priority-fec", 0, InnerFECEnum());
    args.help(u"high-priority-fec",
              u"Used for DVB-T/T2 tuners only. Error correction for high priority streams. "
              u"The default is \"auto\".");

    args.option(u"low-priority-fec", 0, InnerFECEnum());
    args.help(u"low-priority-fec",
              u"Used for DVB-T/T2 tuners only. Error correction for low priority streams. "
              u"The default is \"auto\".");

    DefineLegacyBandWidthArg(args, u"bandwidth", 0, DEFAULT_BANDWIDTH_DVBT, DEFAULT_BANDWIDTH_ISDBT);

    args.option(u"transmission-mode", 0, TransmissionModeEnum());
    args.help(u"transmission-mode",
              u"Used for terrestrial tuners only. Transmission mode. The default is \"" +
              TransmissionModeEnum().name(DEFAULT_TRANSMISSION_MODE_DVBT) + u"\" for DVB-T/T2, \"" +
              TransmissionModeEnum().name(DEFAULT_TRANSMISSION_MODE_ISDBT) + u"\" for ISDB-T.");

    args.option(u"guard-interval", 0, GuardIntervalEnum());
    args.help(u"guard-interval",
              u"Used for terrestrial tuners only. Guard interval. The default is \"" +
              GuardIntervalEnum().name(DEFAULT_GUARD_INTERVAL_DVBT) + u"\" for DVB-T/T2, \"" +
              GuardIntervalEnum().name(DEFAULT_GUARD_INTERVAL_ISDBT) + u"\" for ISDB-T.");

    args.option(u"hierarchy", 0, HierarchyEnum());
    args.help(u"hierarchy", u"Used for DVB-T/T2 tuners only. The default is \"none\".");

    args.option(u"pilots", 0, PilotEnum());
    args.help(u"pilots",
              u"Used for DVB-S2 tuners only. Presence of pilots frames. "
              u"The default is \"off\". ");

    args.option(u"roll-off", 0, RollOffEnum());
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

    args.option(u"pls-mode", 0, PLSModeEnum());
    args.help(u"pls-mode", u"mode",
              u"Used for DVB-S2 tuners only. "
              u"Physical Layer Scrambling (PLS) mode. With multistream only. The default is GOLD. "
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

    args.option(u"isdbt-layer-a-fec", 0, InnerFECEnum());
    args.option(u"isdbt-layer-b-fec", 0, InnerFECEnum());
    args.option(u"isdbt-layer-c-fec", 0, InnerFECEnum());

    args.help(u"isdbt-layer-a-fec",
              u"Used for ISDB-T tuners only. Error correction for layer A. "
              u"The default is automatically detected.");
    args.help(u"isdbt-layer-b-fec", u"Same as --isdbt-layer-a-fec for layer B.");
    args.help(u"isdbt-layer-c-fec", u"Same as --isdbt-layer-a-fec for layer C.");

    args.option(u"isdbt-layer-a-modulation", 0, ModulationEnum());
    args.option(u"isdbt-layer-b-modulation", 0, ModulationEnum());
    args.option(u"isdbt-layer-c-modulation", 0, ModulationEnum());

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
