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
#include "tsHFBand.h"
#include "tsArgs.h"
#include "tsSysUtils.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr ts::SpectralInversion ts::ModulationArgs::DEFAULT_INVERSION;
constexpr ts::InnerFEC          ts::ModulationArgs::DEFAULT_INNER_FEC;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_SYMBOL_RATE_DVBS;
constexpr uint32_t              ts::ModulationArgs::DEFAULT_SYMBOL_RATE_DVBC;
constexpr ts::Modulation        ts::ModulationArgs::DEFAULT_MODULATION_DVBS;
constexpr ts::Modulation        ts::ModulationArgs::DEFAULT_MODULATION_DVBC;
constexpr ts::Polarization      ts::ModulationArgs::DEFAULT_POLARITY;
constexpr size_t                ts::ModulationArgs::DEFAULT_SATELLITE_NUMBER;
constexpr ts::Pilot             ts::ModulationArgs::DEFAULT_PILOTS;
constexpr ts::RollOff           ts::ModulationArgs::DEFAULT_ROLL_OFF;
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
    // Tuning options.
    args.option(u"bandwidth", 0, BandWidthEnum);
    args.help(u"bandwidth",
              u"Used for DVB-T/T2 tuners only. The default is \"8-MHz\".");

    args.option(u"delivery-system", 0, DeliverySystemEnum);
    args.help(u"delivery-system",
              u"Used for DVB-S and DVB-S2 tuners only. Which delivery system to use. "
              u"The default is \"DVB-S\".");

    args.option(u"fec-inner", 0, InnerFECEnum);
    args.help(u"fec-inner",
              u"Used for DVB-S/S2 and DVB-C tuners only. Inner Forward Error Correction. "
              u"The default is \"auto\".");

    args.option(u"frequency", _allow_short_options ? 'f' : 0, Args::UNSIGNED);
    args.help(u"frequency", u"Carrier frequency in Hz (all tuners). There is no default.");

    args.option(u"guard-interval", 0, GuardIntervalEnum);
    args.help(u"guard-interval", u"Used for DVB-T/T2 tuners only. The default is \"1/32\".");

    args.option(u"hierarchy", 0, HierarchyEnum);
    args.help(u"hierarchy", u"Used for DVB-T/T2 tuners only. The default is \"none\".");

    args.option(u"high-priority-fec", 0, InnerFECEnum);
    args.help(u"high-priority-fec",
              u"Used for DVB-T/T2 tuners only. "
              u"Error correction for high priority streams. "
              u"The default is \"auto\".");

    args.option(u"lnb", 0, Args::STRING);
    args.help(u"lnb", u"low_freq[,high_freq,switch_freq]",
              u"Used for DVB-S and DVB-S2 tuners only. "
              u"Description of the LNB.  All frequencies are in MHz. "
              u"low_freq and high_freq are the frequencies of the local oscillators. "
              u"switch_freq is the limit between the low and high band. "
              u"high_freq and switch_freq are used for dual-band LNB's only. "
              u"The default is a universal LNB: low_freq = 9750 MHz, high_freq = 10600 MHz, switch_freq = 11700 MHz.");

    args.option(u"low-priority-fec", 0, InnerFECEnum);
    args.help(u"low-priority-fec",
              u"Used for DVB-T/T2 tuners only. "
              u"Error correction for low priority streams. "
              u"The default is \"auto\".");

    args.option(u"modulation", _allow_short_options ? 'm' : 0, ModulationEnum);
    args.help(u"modulation",
              u"Used for DVB-C, DVB-T, DVB-S2 and ATSC tuners. "
              u"Modulation type. "
              u"The default is \"64-QAM\" for DVB-T/T2 and DVB-C, \"QPSK\" for DVB-S2, \"8-VSB\" for ATSC.");

    args.option(u"pilots", 0, PilotEnum);
    args.help(u"pilots",
              u"Used for DVB-S2 tuners only. Presence of pilots frames. "
              u"The default is \"off\". ");

    args.option(u"plp", 0, Args::UINT8);
    args.help(u"plp",
              u"Used for DVB-T2 tuners only. "
              u"Physical Layer Pipe (PLP) number to select, from 0 to 255. "
              u"The default is to keep the entire stream, without PLP selection. "
              u"Warning: this option is supported on Linux only.");

    args.option(u"polarity", 0, PolarizationEnum);
    args.help(u"polarity",
              u"Used for DVB-S and DVB-S2 tuners only. "
              u"Polarity. The default is \"vertical\".");

    args.option(u"roll-off", 0, RollOffEnum);
    args.help(u"roll-off",
              u"Used for DVB-S2 tuners only. Roll-off factor. "
              u"The default is \"0.35\" (implied for DVB-S, default for DVB-S2).");

    args.option(u"satellite-number", 0, Args::INTEGER, 0, 1, 0, 3);
    args.help(u"satellite-number",
              u"Used for DVB-S and DVB-S2 tuners only. "
              u"Satellite/dish number. Must be 0 to 3 with DiSEqC switches and 0 to 1 for "
              u"non-DiSEqC switches. The default is 0.");

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

    args.option(u"spectral-inversion", 0, SpectralInversionEnum);
    args.help(u"spectral-inversion",
              u"Spectral inversion. The default is \"auto\".");

    args.option(u"symbol-rate", _allow_short_options ? 's' : 0, Args::UNSIGNED);
    args.help(u"symbol-rate",
              u"Used for DVB-S, DVB-S2 and DVB-C tuners only. "
              u"Symbol rate in symbols/second. The default is " +
              UString::Decimal(DEFAULT_SYMBOL_RATE_DVBS) +
              u" sym/s for satellite and " +
              UString::Decimal(DEFAULT_SYMBOL_RATE_DVBC) +
              u" sym/s for cable. ");

    args.option(u"transmission-mode", 0, TransmissionModeEnum);
    args.help(u"transmission-mode",
              u"Used for DVB-T tuners only. Transmission mode. The default is \"8K\".");

    // UHF/VHF frequency bands options.
    args.option(u"uhf-channel", 0, Args::POSITIVE);
    args.help(u"uhf-channel",
              u"Used for DVB-T or ATSC tuners only. "
              u"Specify the UHF channel number of the carrier. "
              u"Can be used in replacement to --frequency. "
              u"Can be combined with an --offset-count option. "
              u"The UHF frequency layout depends on the region, see --hf-band-region option.");

    args.option(u"vhf-channel", 0, Args::POSITIVE);
    args.help(u"vhf-channel",
              u"Used for DVB-T or ATSC tuners only. "
              u"Specify the VHF channel number of the carrier. "
              u"Can be used in replacement to --frequency. "
              u"Can be combined with an --offset-count option. "
              u"The VHF frequency layout depends on the region, see --hf-band-region option.");

    args.option(u"offset-count", 0, Args::INTEGER, 0, 1, -10, 10);
    args.help(u"offset-count",
              u"Used for DVB-T or ATSC tuners only. "
              u"Specify the number of offsets from the UHF or VHF channel. "
              u"The default is zero. See options --uhf-channel or --vhf-channel.");
}
