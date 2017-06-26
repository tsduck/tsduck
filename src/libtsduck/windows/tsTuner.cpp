//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//
//  DVB tuner. Windows implementation
//
//-----------------------------------------------------------------------------

#include "tsTuner.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBT.h"
#include "tsTunerParametersATSC.h"
#include "tsTime.h"
#include "tsNullReport.h"
#include "tsSysUtils.h"
#include "tsDirectShowUtils.h"
#include "tsComUtils.h"
#include "tsComPtr.h"
#include "tsDecimal.h"
#include "tsToInteger.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const ts::MilliSecond ts::Tuner::DEFAULT_SIGNAL_TIMEOUT;
const size_t ts::Tuner::DEFAULT_SINK_QUEUE_SIZE;
#endif

// Put the value of a property (named "type") into a COM object.
// Report errors through a variable named report (must be accessible).
// Return true on success, false on error.
#define PUT(obj,type,value) ComSuccess ((obj)->put_##type (value), "error setting " #type, report)


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

ts::Tuner::~Tuner()
{
    close(NULLREP);
}


//-----------------------------------------------------------------------------
// Default constructor, 
//-----------------------------------------------------------------------------

ts::Tuner::Tuner() :
    _is_open(false),
    _info_only(true),
    _tuner_type(DVB_T),
    _device_name(),
    _device_info(),
    _signal_timeout(DEFAULT_SIGNAL_TIMEOUT),
    _signal_timeout_silent(false),
    _receive_timeout(0),
    _delivery_systems(),
    _sink_queue_size(DEFAULT_SINK_QUEUE_SIZE),
    _graph(),
    _media_control(),
    _sink_filter(),
    _provider_filter(),
    _provider_name(),
    _net_provider(),
    _tuner(),
    _tuning_space(),
    _tuning_space_fname(),
    _tuning_space_uname(),
    _tuner_filter(),
    _demods(),
    _demods2(),
    _sigstats(),
    _tunprops()
{
}


//-----------------------------------------------------------------------------
// Constructor from one device name.
//-----------------------------------------------------------------------------

ts::Tuner::Tuner(const std::string& device_name, bool info_only, ReportInterface& report) :
    _is_open(false),
    _info_only(true),
    _tuner_type(DVB_T),
    _device_name(device_name),
    _device_info(),
    _signal_timeout(DEFAULT_SIGNAL_TIMEOUT),
    _signal_timeout_silent(false),
    _receive_timeout(0),
    _sink_queue_size(DEFAULT_SINK_QUEUE_SIZE),
    _graph(),
    _media_control(),
    _sink_filter(),
    _provider_filter(),
    _provider_name(),
    _net_provider(),
    _tuner(),
    _tuning_space(),
    _tuning_space_fname(),
    _tuning_space_uname(),
    _tuner_filter(),
    _demods(),
    _demods2(),
    _sigstats(),
    _tunprops()
{
    this->open(device_name, info_only, report);
}


//-----------------------------------------------------------------------------
// Get the list of all existing DVB tuners.
//-----------------------------------------------------------------------------

bool ts::Tuner::GetAllTuners(TunerPtrVector& tuners, ReportInterface& report)
{
    return FindTuners(0, &tuners, report);
}


//-----------------------------------------------------------------------------
// Open the tuner.
//-----------------------------------------------------------------------------

bool ts::Tuner::open(const std::string& device_name, bool info_only, ReportInterface& report)
{
    if (_is_open) {
        report.error("DVB tuner already open");
        return false;
    }
    _device_name = device_name;
    if (!FindTuners(this, 0, report)) {
        return false;
    }
    else if (_is_open) {
        _info_only = info_only;
        return true;
    }
    else if (device_name.empty()) {
        report.error("No DVB tuner device");
        return false;
    }
    else {
        report.error("DVB device \"" + device_name + "\" not found");
        return false;
    }
}


//-----------------------------------------------------------------------------
// Close tuner.
//-----------------------------------------------------------------------------

bool ts::Tuner::close(ReportInterface& report)
{
    _is_open = false;
    _device_name.clear();
    _device_info.clear();
    _graph.release();
    _media_control.release();
    _sink_filter.release();
    _provider_filter.release();
    _provider_name.clear();
    _net_provider.release();
    _tuner.release();
    _tuning_space.release();
    _tuning_space_fname.clear();
    _tuning_space_uname.clear();
    _tuner_filter.release();
    ComVectorClear(_demods);
    ComVectorClear(_demods2);
    ComVectorClear(_sigstats);
    ComVectorClear(_tunprops);
    return true;
}


//-----------------------------------------------------------------------------
// Ugly macros to search a property both in a vector of specialized
// interfaces and a vector of IKsPropertySet.
//-----------------------------------------------------------------------------

#define SELECTPROP(terminated, found, retvalue, val, searchtype) \
    {                                                            \
        switch (searchtype) {                                    \
            case psFIRST:                                        \
                retvalue = val;                                  \
                terminated = true;                               \
                break;                                           \
            case psLAST:                                         \
                retvalue = val;                                  \
                break;                                           \
            case psHIGHEST:                                      \
                if (!found || val > retvalue) {                  \
                    retvalue = val;                              \
                }                                                \
                break;                                           \
            case psLOWEST:                                       \
                if (!found || val < retvalue) {                  \
                    retvalue = val;                              \
                }                                                \
                break;                                           \
        }                                                        \
        found = true;                                            \
    }

#define SEARCHPROP(found, retvalue, searchtype, ivector, get, ivaltype, propset, propid, pvaltype) \
    {                                                                      \
        found = false;                                                     \
        bool terminated = false;                                           \
        for (size_t i = 0; !terminated && i < ivector.size(); ++i) {       \
            ivaltype val;                                                  \
            if (SUCCEEDED (ivector[i]->get (&val))) {                      \
                SELECTPROP (terminated, found, retvalue, val, searchtype); \
            }                                                              \
        }                                                                  \
        for (size_t i = 0; !terminated && i < _tunprops.size(); ++i) {     \
            pvaltype val;                                                  \
            ::DWORD retsize = sizeof (val);                                \
            if (SUCCEEDED (_tunprops[i]->Get (propset, propid, NULL, 0, &val, retsize, &retsize))) { \
                SELECTPROP (terminated, found, retvalue, val, searchtype); \
            }                                                              \
        }                                                                  \
    }

//-----------------------------------------------------------------------------
// Search all IKsPropertySet in the tuner until the specified data is found.
// Return true if found, false if not found.
//-----------------------------------------------------------------------------

template <typename T>
bool ts::Tuner::searchTunerProperty (const ::GUID& propset, ::DWORD propid, T& value, PropSearch ps)
{
    bool found = false;
    bool terminated = false;

    for (size_t i = 0; !terminated && i < _tunprops.size(); ++i) {
        T val;
        ::DWORD retsize = sizeof (val);
        if (SUCCEEDED (_tunprops[i]->Get (propset, propid, NULL, 0, &val, retsize, &retsize))) {
            SELECTPROP (terminated, found, value, val, ps);
        }
    }
    return found;
}


//-----------------------------------------------------------------------------
// Check if a signal is present and locked
//-----------------------------------------------------------------------------

bool ts::Tuner::signalLocked (ReportInterface& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    bool found;
    ::BOOL locked = 0;
    SEARCHPROP (found, locked, psHIGHEST,
                _sigstats, get_SignalLocked, ::BOOLEAN,
                KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_LOCKED, ::BOOL);

    return found && locked != 0;
}


//-----------------------------------------------------------------------------
// Get signal strength in mdB.
// The header bdamedia.h defines carrier strength in mdB (1/1000 of a dB).
// A strength of 0 is nominal strength as expected for the given network.
// Sub-nominal strengths are reported as positive mdB
// Super-nominal strengths are reported as negative mdB
// Return true if found, false if not found.
//-----------------------------------------------------------------------------

bool ts::Tuner::getSignalStrength_mdB (::LONG& strength)
{
    bool found;
    SEARCHPROP (found, strength, psHIGHEST,
                _sigstats, get_SignalStrength, ::LONG,
                KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_STRENGTH, ::LONG);
    return found;
}


//-----------------------------------------------------------------------------
// Return signal strength, in percent (0=bad, 100=good)
// Return a negative value on error.
//-----------------------------------------------------------------------------

int ts::Tuner::signalStrength (ReportInterface& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    // Use -100 dB (-100 000 mdB) as zero (null strength)
    // Avoid returning negative value on success.
    ::LONG strength;
    return getSignalStrength_mdB (strength) ? std::max (0, 100 + int (strength) / 1000) : -1;
}


//-----------------------------------------------------------------------------
// Return signal quality, in percent (0=bad, 100=good)
// Return a negative value on error.
//-----------------------------------------------------------------------------

int ts::Tuner::signalQuality (ReportInterface& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    bool found;
    ::LONG quality = 0;
    SEARCHPROP (found, quality, psHIGHEST,
                _sigstats, get_SignalQuality, ::LONG,
                KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_QUALITY, ::LONG);

    return found ? int (quality) : -1;
}


//-----------------------------------------------------------------------------
// Get the current tuning parameters
//-----------------------------------------------------------------------------

bool ts::Tuner::getCurrentTuning (TunerParameters& params, bool reset_unknown, ReportInterface& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    // Check subclass of TunerParameters
    if (params.tunerType() != _tuner_type) {
        report.error ("inconsistent tuner parameter type");
        return false;
    }

    // Search individual tuning parameters
    bool found;    
    switch (_tuner_type) {

        case DVB_S: {
            TunerParametersDVBS* tpp = dynamic_cast<TunerParametersDVBS*> (&params);
            assert (tpp != 0);
            if (reset_unknown) {
                tpp->frequency = 0;
                tpp->symbol_rate = 0;
                tpp->polarity = TunerParametersDVBS::DEFAULT_POLARITY;
                tpp->satellite_number = TunerParametersDVBS::DEFAULT_SATELLITE_NUMBER;
                tpp->lnb.setUniversalLNB();
            }
            // Spectral inversion
            ::SpectralInversion spinv = ::BDA_SPECTRAL_INVERSION_NOT_SET;
            SEARCHPROP (found, spinv, psFIRST,
                        _demods, get_SpectralInversion, ::SpectralInversion,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION, ::SpectralInversion);
            tpp->inversion = found ? ts::SpectralInversion (spinv) : SPINV_AUTO;
            // Symbol rate
            ::ULONG symrate = 0;
            SEARCHPROP (found, symrate, psHIGHEST,
                        _demods, get_SymbolRate, ::ULONG,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SYMBOL_RATE, ::ULONG);
            if (found) {
                tpp->symbol_rate = uint32_t(symrate);
            }
            // Inner FEC
            ::BinaryConvolutionCodeRate fec = ::BDA_BCC_RATE_NOT_SET;
            SEARCHPROP (found, fec, psFIRST,
                        _demods, get_InnerFECRate, ::BinaryConvolutionCodeRate,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE, ::BinaryConvolutionCodeRate);
            tpp->inner_fec = found ? ts::InnerFEC (fec) : ts::FEC_AUTO;
            // Modulation
            ::ModulationType mod = ::BDA_MOD_NOT_SET;
            SEARCHPROP (found, mod, psFIRST,
                        _demods, get_ModulationType, ::ModulationType,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE, ::ModulationType);
            tpp->modulation = found ? ts::Modulation (mod) : QPSK;
            // Delivery system.
            // Found no way to get DVB-S vs. DVB-S2 on Windows.
            // Make a not quite correct assumption, based on modulation type.
            tpp->delivery_system = tpp->modulation == QPSK ? DS_DVB_S : DS_DVB_S2;
            // DVB-S2 pilot
            ::Pilot pilot = ::BDA_PILOT_NOT_SET;
            SEARCHPROP (found, pilot, psFIRST,
                        _demods2, get_Pilot, ::Pilot,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_PILOT, ::Pilot);
            tpp->pilots = found ? ts::Pilot (pilot) : PILOT_AUTO;
            // DVB-S2 roll-off factor
            ::RollOff roff = ::BDA_ROLL_OFF_NOT_SET;
            SEARCHPROP (found, roff, psFIRST,
                        _demods2, get_RollOff, ::RollOff,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_ROLL_OFF, ::RollOff);
            tpp->roll_off = found ? ts::RollOff (roff) : ROLLOFF_AUTO;
            break;
        }

        case DVB_C: {
            TunerParametersDVBC* tpp = dynamic_cast<TunerParametersDVBC*> (&params);
            assert (tpp != 0);
            if (reset_unknown) {
                tpp->frequency = 0;
                tpp->symbol_rate = 0;
            }
            // Spectral inversion
            ::SpectralInversion spinv = ::BDA_SPECTRAL_INVERSION_NOT_SET;
            SEARCHPROP (found, spinv, psFIRST,
                        _demods, get_SpectralInversion, ::SpectralInversion,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION, ::SpectralInversion);
            tpp->inversion = found ? ts::SpectralInversion (spinv) : SPINV_AUTO;
            // Symbol rate
            ::ULONG symrate = 0;
            SEARCHPROP (found, symrate, psHIGHEST,
                        _demods, get_SymbolRate, ::ULONG,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SYMBOL_RATE, ::ULONG);
            if (found) {
                tpp->symbol_rate = uint32_t(symrate);
            }
            // Inner FEC
            ::BinaryConvolutionCodeRate fec = ::BDA_BCC_RATE_NOT_SET;
            SEARCHPROP (found, fec, psFIRST,
                        _demods, get_InnerFECRate, ::BinaryConvolutionCodeRate,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE, ::BinaryConvolutionCodeRate);
            tpp->inner_fec = found ? ts::InnerFEC (fec) : ts::FEC_AUTO;
            // Modulation
            ::ModulationType mod = ::BDA_MOD_NOT_SET;
            SEARCHPROP (found, mod, psFIRST,
                        _demods, get_ModulationType, ::ModulationType,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE, ::ModulationType);
            tpp->modulation = found ? ts::Modulation (mod) : QAM_AUTO;
            break;
        }

        case DVB_T: {
            TunerParametersDVBT* tpp = dynamic_cast<TunerParametersDVBT*> (&params);
            assert (tpp != 0);
            if (reset_unknown) {
                tpp->frequency = 0;
            }
            // Spectral inversion
            ::SpectralInversion spinv = ::BDA_SPECTRAL_INVERSION_NOT_SET;
            SEARCHPROP (found, spinv, psFIRST,
                        _demods, get_SpectralInversion, ::SpectralInversion,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION, ::SpectralInversion);
            tpp->inversion = found ? ts::SpectralInversion (spinv) : SPINV_AUTO;
            // High priority FEC
            ::BinaryConvolutionCodeRate fec = ::BDA_BCC_RATE_NOT_SET;
            SEARCHPROP (found, fec, psFIRST,
                        _demods, get_InnerFECRate, ::BinaryConvolutionCodeRate,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE, ::BinaryConvolutionCodeRate);
            tpp->fec_hp = found ? ts::InnerFEC (fec) : ts::FEC_AUTO;
            // Modulation
            ::ModulationType mod = ::BDA_MOD_NOT_SET;
            SEARCHPROP (found, mod, psFIRST,
                        _demods, get_ModulationType, ::ModulationType,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE, ::ModulationType);
            tpp->modulation = found ? ts::Modulation (mod) : QAM_AUTO;
            // Other DVB-T parameters, not supported in IBDA_DigitalDemodulator
            // but which may be supported as properties.
            ::TransmissionMode tm = ::BDA_XMIT_MODE_NOT_SET;
            tpp->transmission_mode = 
                searchTunerProperty (KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_TRANSMISSION_MODE, tm, psFIRST) ?
                ts::TransmissionMode (tm) : TM_AUTO;
            ::GuardInterval gi = ::BDA_GUARD_NOT_SET;
            tpp->guard_interval = 
                searchTunerProperty (KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_GUARD_INTERVAL, gi, psFIRST) ?
                GuardInterval (gi) : GUARD_AUTO;
            // Other DVB-T parameters, not supported at all
            tpp->bandwidth = BW_AUTO;
            tpp->hierarchy = HIERARCHY_AUTO;
            tpp->fec_lp = FEC_AUTO;
            break;
        }

        case ATSC: {
            TunerParametersATSC* tpp = dynamic_cast<TunerParametersATSC*> (&params);
            assert (tpp != 0);
            if (reset_unknown) {
                tpp->frequency = 0;
            }
            // Spectral inversion
            ::SpectralInversion spinv = ::BDA_SPECTRAL_INVERSION_NOT_SET;
            SEARCHPROP (found, spinv, psFIRST,
                        _demods, get_SpectralInversion, ::SpectralInversion,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION, ::SpectralInversion);
            tpp->inversion = found ? ts::SpectralInversion (spinv) : SPINV_AUTO;
            // Modulation
            ::ModulationType mod = ::BDA_MOD_NOT_SET;
            SEARCHPROP (found, mod, psFIRST,
                        _demods, get_ModulationType, ::ModulationType,
                        KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE, ::ModulationType);
            tpp->modulation = found ? ts::Modulation (mod) : QAM_AUTO;
            break;
        }

        default: {
            report.error ("cannot convert BDA tuning parameters to " + TunerTypeEnum.name (_tuner_type) + " parameters");
            return false;
        }
    }

    return true;
}


//-----------------------------------------------------------------------------
// Tune to the specified parameters and start receiving.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::tune (const TunerParameters& params, ReportInterface& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }
    else {
        return internalTune (params, report);
    }
}


//-----------------------------------------------------------------------------
// Internal tune method, works also if the tuner is not in open state.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::internalTune (const TunerParameters& params, ReportInterface& report)
{
    // Check subclass of TunerParameters
    if (params.tunerType() != _tuner_type) {
        report.error ("inconsistent tuner parameter type");
        return false;
    }

    // Create a DirectShow tune request
    ComPtr <::ITuneRequest> tune_request;
    assert (!_tuning_space.isNull());
    ::HRESULT hr = _tuning_space->CreateTuneRequest (tune_request.creator());
    if (!ComSuccess (hr, "cannot create DirectShow tune request", report)) {
        return false;
    }
    assert (!tune_request.isNull());

    // Extract DVB interface of tune request and set ids to wildcards
    ComPtr <::IDVBTuneRequest> dvb_request;
    dvb_request.queryInterface (tune_request.pointer(), ::IID_IDVBTuneRequest, report);
    if (!dvb_request.isNull() &&
        (!PUT (dvb_request, ONID, -1) ||
         !PUT (dvb_request, TSID, -1) ||
         !PUT (dvb_request, SID, -1))) {
        return false;
    }

    // Create a locator (where to find the physical TS, ie. tuning params).
    ComPtr <::IDigitalLocator> locator;
    switch (_tuner_type) {

        case DVB_S: {
            const TunerParametersDVBS* tpp = dynamic_cast<const TunerParametersDVBS*> (&params);
            assert (tpp != 0);
            if (!createLocatorDVBS (locator, *tpp, report)) {
                return false;
            }
            break;
        }

        case DVB_T: {
            const TunerParametersDVBT* tpp = dynamic_cast<const TunerParametersDVBT*> (&params);
            assert (tpp != 0);
            if (!createLocatorDVBT (locator, *tpp, report)) {
                return false;
            }
            break;
        }

        case DVB_C: {
            const TunerParametersDVBC* tpp = dynamic_cast<const TunerParametersDVBC*> (&params);
            assert (tpp != 0);
            if (!createLocatorDVBC (locator, *tpp, report)) {
                return false;
            }
            break;
        }

        default: {
            report.error ("cannot convert " + TunerTypeEnum.name (_tuner_type) + " parameters to DirectShow tuning parameters");
            return false;
        }
    }

    // Set the locator in the tune request    
    if (!locator.isNull()) {
        hr = tune_request->put_Locator (locator.pointer());
        if (!ComSuccess (hr, "ITuneRequest::put_Locator", report)) {
            return false;
        }
    }

    // Tune to transponder
    hr = _tuner->put_TuneRequest (tune_request.pointer());
    return ComSuccess (hr, "DirectShow tuning error", report);
}


//-----------------------------------------------------------------------------
// Create an IDigitalLocator object for DVB-T parameters
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::createLocatorDVBT (ComPtr<::IDigitalLocator>& locator,
                                     const TunerParametersDVBT& params,
                                     ReportInterface& report)
{
    ComPtr <::IDVBTLocator> loc (CLSID_DVBTLocator, ::IID_IDVBTLocator, report);

    if (loc.isNull() ||
        !CheckModEnum (params.inversion, "spectral inversion", SpectralInversionEnum, report) ||
        !CheckModEnum (params.bandwidth, "bandwidth", BandWidthEnum, report) ||
        !CheckModEnum (params.fec_hp, "FEC", InnerFECEnum, report) ||
        !CheckModEnum (params.fec_lp, "FEC", InnerFECEnum, report) ||
        !CheckModEnum (params.modulation, "constellation", ModulationEnum, report) ||
        !CheckModEnum (params.transmission_mode, "transmission mode", TransmissionModeEnum, report) ||
        !CheckModEnum (params.guard_interval, "guard interval", GuardIntervalEnum, report) ||
        !CheckModEnum (params.hierarchy, "hierarchy", HierarchyEnum, report) ||
        !PUT (loc, CarrierFrequency, long (params.frequency / 1000)) ||  // frequency in kHz
        !PUT (loc, Modulation, ::ModulationType (params.modulation)) ||
        !PUT (loc, Bandwidth, long (params.bandwidth)) ||
        !PUT (loc, Guard, ::GuardInterval (params.guard_interval)) ||
        !PUT (loc, LPInnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT (loc, LPInnerFECRate, ::BinaryConvolutionCodeRate (params.fec_lp)) ||
        !PUT (loc, Mode, ::TransmissionMode (params.transmission_mode)) ||
        !PUT (loc, HAlpha, ::HierarchyAlpha (params.hierarchy))) {
        return false;
    }

    // Pending questions:
    // - Shall we call loc->put_OtherFrequencyInUse ? Documented as
    //   "specifies whether the frequency is being used by another
    //   DVB-T broadcaster". No idea what this means...
    // - No way to set params.inversion and params.fec_hp in IDVBTLocator

    locator.assign (loc);
    return true;
}


//-----------------------------------------------------------------------------
// Create an IDigitalLocator object for DVB-C parameters
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::createLocatorDVBC (ComPtr<::IDigitalLocator>& locator,
                                     const TunerParametersDVBC& params,
                                     ReportInterface& report)
{
    ComPtr<::IDVBCLocator> loc (CLSID_DVBCLocator, ::IID_IDVBCLocator, report);

    if (loc.isNull() ||
        !CheckModEnum (params.inversion, "spectral inversion", SpectralInversionEnum, report) ||
        !CheckModEnum (params.inner_fec, "FEC", InnerFECEnum, report) ||
        !CheckModEnum (params.modulation, "modulation", ModulationEnum, report) ||
        !PUT (loc, CarrierFrequency, long (params.frequency / 1000)) ||  // frequency in kHz
        !PUT (loc, Modulation, ::ModulationType (params.modulation)) ||
        !PUT (loc, InnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT (loc, InnerFECRate, ::BinaryConvolutionCodeRate (params.inner_fec)) ||
        !PUT (loc, SymbolRate, long (params.symbol_rate))) {
        return false;
    }

    // Pending questions:
    // - No way to set params.inversion in IDVBCLocator

    locator.assign (loc);
    return true;
}


//-----------------------------------------------------------------------------
// Create an IDigitalLocator object for DVB-S/S2 parameters
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::createLocatorDVBS (ComPtr<::IDigitalLocator>& locator,
                                     const TunerParametersDVBS& params,
                                     ReportInterface& report)
{
    // Microsoft oddity, part 1...
    // The locator interface for DVB-S is IDVBSLocator. However, this interface did
    // not implement LNB control and DVB-S2. Starting with Windows 7, a new interface
    // IDVBSLocator2 is introduced to support LNB control and DVB-S2. However, unlike
    // all other locator interfaces, CLSID_DVBSLocator2 is not defined anywhere, not
    // in tuner.h and not even in the Windows 7 registry. So, since IDVBSLocator2 is
    // a subinterface of IDVBSLocator, we create an object of class CLSID_DVBSLocator
    // and we hope that on Windows 7 this object will also implement IDVBSLocator2.
    // So, beware that loc may be valid while loc2 can be null (typically before Windows 7).

    ComPtr <::IDVBSLocator> loc (CLSID_DVBSLocator, ::IID_IDVBSLocator, report);
    if (loc.isNull()) {
        return false;
    }

    ComPtr<::IDVBSLocator2> loc2;
    loc2.queryInterface(loc.pointer(), ::IID_IDVBSLocator2, NULLREP);
    report.debug("IDVBSLocator = %" FMT_INT64 "d, IDVBSLocator2 = %" FMT_INT64 "d", uint64_t(loc.pointer()), uint64_t(loc2.pointer()));

    // Microsoft oddity, part 2...
    // Unlike other modulations, with pre-Windows 7 systems, some of the DVB-S
    // parameters must be set in the tuning space (IDVBSTuningSpace interface)
    // and not in the locator (IDVBSLocator interface). However, Microsoft seemed
    // to understand the mistake in Windows 7 and finally added these parameters
    // in IDVBSLocator2.

    if (loc2.isNull()) {

        // No IDVBSLocator2 support, using tuning space for LNB control
        ComPtr <::IDVBSTuningSpace> stspace;
        stspace.queryInterface (_tuning_space.pointer(), ::IID_IDVBSTuningSpace, report);
        if (stspace.isNull() ||
            !CheckModEnum (params.inversion, "spectral inversion", SpectralInversionEnum, report) ||
            !PUT (stspace, SpectralInversion, ::SpectralInversion (params.inversion)) ||
            !PUT (stspace, LowOscillator, long (params.lnb.lowFrequency() / 1000)) ||    // frequency in kHz
            !PUT (stspace, HighOscillator, long (params.lnb.highFrequency() / 1000)) ||  // frequency in kHz
            !PUT (stspace, LNBSwitch, long (params.lnb.switchFrequency() / 1000))) {     // frequency in kHz
            return false;
        }

        // Without IDVBSLocator2, there is no standard way to specify the DiSEqC satellite
        // number. Some BDA drivers use the generic InputRange property of the tuning space
        // to get the satellite number, but it is not guaranteed to work. So, we try but
        // it may not work with some devices.
        char satnum_str[80];
        ::snprintf(satnum_str, sizeof (satnum_str), "%" FMT_SIZE_T "d", params.satellite_number);
        satnum_str[sizeof (satnum_str) - 1] = '\0';
        report.debug ("setting IDVBSTuningSpace.InputRange to %s", satnum_str);
        ::BSTR satnum_bstr = _com_util::ConvertStringToBSTR(satnum_str);
        const bool satnum_ok = PUT (stspace, InputRange, satnum_bstr);
        ::SysFreeString (satnum_bstr);
        if (!satnum_ok) {
            return false;
        }
    }
    else {

        // IDVBSLocator2 is supported, use locator.
        if (!PUT (loc2, LocalSpectralInversionOverride, ::SpectralInversion (params.inversion)) ||
            !PUT (loc2, LocalOscillatorOverrideLow, long (params.lnb.lowFrequency() / 1000)) ||    // frequency in kHz
            !PUT (loc2, LocalOscillatorOverrideHigh, long (params.lnb.highFrequency() / 1000)) ||  // frequency in kHz
            !PUT (loc2, LocalLNBSwitchOverride, long (params.lnb.switchFrequency() / 1000))) {     // frequency in kHz
            return false;
        }

        // Specify DiSEqC satellite number.
        // Note however that most drivers ignore it...
        ::LNB_Source source (::BDA_LNB_SOURCE_NOT_SET);
        switch (params.satellite_number) {
            case 0: source = ::BDA_LNB_SOURCE_A; break;
            case 1: source = ::BDA_LNB_SOURCE_B; break;
            case 2: source = ::BDA_LNB_SOURCE_C; break;
            case 3: source = ::BDA_LNB_SOURCE_D; break;
        }
        if (source != ::BDA_LNB_SOURCE_NOT_SET) {
            report.debug ("setting IDVBSLocator2.DiseqLNBSource to %d", int (source));
            if (!PUT (loc2, DiseqLNBSource, source)) {
                return false;
            }
        }
    }

    // Common modulation parameters
    if (!CheckModEnum (params.modulation, "modulation", ModulationEnum, report) ||
        !CheckModEnum (params.inner_fec, "FEC", InnerFECEnum, report) ||
        !CheckModEnum (params.polarity, "polarity", PolarizationEnum, report) ||
        !PUT (loc, CarrierFrequency, long (params.frequency / 1000)) ||              // frequency in kHz
        !PUT (loc, Modulation, ::ModulationType (params.modulation)) ||
        !PUT (loc, SignalPolarisation, ::Polarisation (params.polarity)) ||
        !PUT (loc, InnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT (loc, InnerFECRate, ::BinaryConvolutionCodeRate (params.inner_fec)) ||
        !PUT (loc, SymbolRate, long (params.symbol_rate))) {
        return false;
    }

    // DVB-S2 specific parameters
    if (params.delivery_system == DS_DVB_S2) {
        if (loc2.isNull()) {
            report.error("DVB-S2 is not supported on this system, requires Windows 7 and a DVB-S2 tuner");
            return false;
        }
        if (!CheckModEnum (params.pilots, "pilot", PilotEnum, report) ||
            !CheckModEnum (params.roll_off, "roll-off factor", RollOffEnum, report) ||
            !PUT (loc2, SignalPilot, ::Pilot (params.pilots)) ||
            !PUT (loc2, SignalRollOff, ::RollOff (params.roll_off))) {
            return false;
        }
    }

    locator.assign (loc); // loc and loc2 are two interfaces of the same object
    return true;
}


//-----------------------------------------------------------------------------
// Start receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::start (ReportInterface& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    // Set media samples queue size
    _sink_filter->SetMaxMessages (_sink_queue_size);

    // Run the graph
    ::HRESULT hr = _media_control->Run();
    if (!ComSuccess (hr, "cannot start DirectShow graph", report)) {
        return false;
    }

    // If the tuner was previously started/stopped on a frequency with signal on it,
    // it has been observed that remaining packets from the previous run were still
    // there. Wait a little bit and reflush after Run() to avoid that.
    // Yes, this is a horrible hack, but if you have a better fix...
    SleepThread (50); // milliseconds
    _sink_filter->Flush();

    // If a signal timeout was specified, read a packet with timeout
    if (_signal_timeout > 0) {
        TSPacket pack;
        if (_sink_filter->Read (&pack, sizeof (pack), _signal_timeout) == 0) {
            if (!_signal_timeout_silent) {
                report.error ("no input DVB signal after %" FMT_INT64 "d milliseconds", _signal_timeout);
            }
            return false;
        }
    }

    return true;
}


//-----------------------------------------------------------------------------
// Stop receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::stop (ReportInterface& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    // Get graph state
    ::OAFilterState state;
    ::HRESULT hr = _media_control->GetState (1000, &state); // 1000 ms timeout
    bool stopped = ComSuccess (hr, "IMediaControl::GetState", report) && state == ::State_Stopped;

    // Stop the graph when necessary
    if (stopped) {
        return true;
    }
    else {
        hr = _media_control->Stop();
        return ComSuccess (hr, "IMediaControl::Stop", report);
    }
}


//-----------------------------------------------------------------------------
// Timeout for receive operation (none by default).
// If zero, no timeout is applied.
// Return true on success, false on errors.
//-----------------------------------------------------------------------------

bool ts::Tuner::setReceiveTimeout (MilliSecond timeout, ReportInterface&)
{
    _receive_timeout = timeout;
    return true;
}


//-----------------------------------------------------------------------------
// Read complete 188-byte TS packets in the buffer and return the
// number of actually received packets (in the range 1 to max_packets).
// Returning zero means error or end of input.
//-----------------------------------------------------------------------------

size_t ts::Tuner::receive (TSPacket* buffer, size_t max_packets, const AbortInterface* abort, ReportInterface& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return 0;
    }

    // Read packets from the tuner device

    size_t got_size;
    
    if (_receive_timeout <= 0) {
        got_size = _sink_filter->Read (buffer, max_packets * PKT_SIZE);
    }
    else {
        const Time limit (Time::CurrentUTC() + _receive_timeout);
        got_size = _sink_filter->Read (buffer, max_packets * PKT_SIZE, _receive_timeout);
        if (got_size == 0 && Time::CurrentUTC() >= limit) {
            report.error ("receive timeout on " + _device_name);
        }
    }

    return got_size / PKT_SIZE;
}


//-----------------------------------------------------------------------------
// Display the characteristics and status of the tuner.
//-----------------------------------------------------------------------------

std::ostream& ts::Tuner::displayStatus (std::ostream& strm, const std::string& margin, ReportInterface& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return strm;
    }

    strm << margin << "Signal locked:    " << YesNo (signalLocked (report)) << std::endl;
    int quality = signalQuality (report);
    if (quality >= 0) {
        strm << margin << "Signal quality:   " << quality << " %" << std::endl;
    }
    ::LONG strength;
    if (getSignalStrength_mdB (strength)) {
        strm << margin << "Signal strength:  " << Decimal (strength) << " milli dB" << std::endl;
    }
    strm << margin << "Network provider: " << _provider_name << std::endl
         << std::endl
         << margin << "DirectShow graph:" << std::endl;
    
    DisplayFilterGraph (strm, _graph, margin + "  ", true, report);

    return strm;
}


//-----------------------------------------------------------------------------
// Enumerate all tuner-related DirectShow devices.
//-----------------------------------------------------------------------------

std::ostream& ts::Tuner::EnumerateDevices(std::ostream& strm, const std::string& margin, ReportInterface& report)
{
#define _D_(cat) DisplayDevicesByCategory(strm, cat, margin, #cat, report)
    _D_(KSCATEGORY_BDA_NETWORK_PROVIDER);
    _D_(KSCATEGORY_BDA_TRANSPORT_INFORMATION);
    _D_(KSCATEGORY_CAPTURE);
//  _D_(KSCATEGORY_SPLITTER);
    _D_(KSCATEGORY_TVTUNER);
    _D_(KSCATEGORY_BDA_RECEIVER_COMPONENT);
    _D_(KSCATEGORY_BDA_NETWORK_TUNER);
    return strm;
#undef _D_
}


//-----------------------------------------------------------------------------
// Locate all known interfaces in a pin or node of the tuner filter.
// Ignore errors.
//-----------------------------------------------------------------------------

template <class COMCLASS>
void ts::Tuner::findTunerSubinterfaces(ComPtr<COMCLASS>& obj)
{
#define _D_(iface,vect)                                           \
    {                                                             \
        ComPtr<iface> iobj;                                       \
        iobj.queryInterface(obj.pointer(), IID_##iface, NULLREP); \
        if (!iobj.isNull()) {                                     \
            vect.push_back(iobj);                                 \
        }                                                         \
    }

    _D_(IBDA_DigitalDemodulator,  _demods);
    _D_(IBDA_DigitalDemodulator2, _demods2);
    _D_(IBDA_SignalStatistics,    _sigstats);
    _D_(IKsPropertySet,           _tunprops);
#undef _D_
}


//-----------------------------------------------------------------------------
// Private static method: Find one or more tuners.
//-----------------------------------------------------------------------------

bool ts::Tuner::FindTuners(Tuner* tuner, TunerPtrVector* tuner_list, ReportInterface& report)
{
    // ReportInterface to use when errors shall be reported in debug mode only
    ReportInterface& debug_report(report.debug() ? report : NULLREP);

    // Exactly one of Tuner* or TunerPtrVector* must be non-zero.
    assert(tuner == 0 || tuner_list == 0);
    assert(tuner != 0 || tuner_list != 0);

    // Reset content of tuner vector
    if (tuner_list != 0) {
        tuner_list->clear();
    }

    // Check if tuner device name is ":integer"
    int dvb_device_index = -1;
    if (tuner != 0 && !tuner->_device_name.empty() && tuner->_device_name[0] == ':') {
        ToInteger(dvb_device_index, tuner->_device_name.substr(1));
    }

    // Enumerate all filters with category KSCATEGORY_BDA_NETWORK_TUNER.
    // These filters are usually installed by vendors of hardware tuners
    // when they provide BDA-compatible drivers.
    std::vector<ComPtr<::IMoniker>> tuner_monikers;
    if (!EnumerateDevicesByClass(KSCATEGORY_BDA_NETWORK_TUNER, tuner_monikers, report)) {
        return false;
    }

    // Loop on all enumerated tuners.
    for (size_t dvb_device_current = 0; dvb_device_current < tuner_monikers.size(); ++dvb_device_current) {

        // Get friendly name of this tuner filter
        const std::string tuner_name(GetStringPropertyBag(tuner_monikers[dvb_device_current].pointer(), L"FriendlyName", debug_report));
        report.debug("found tuner filter \"" + tuner_name + "\"");

        // If a device name was specified, filter this name.
        if (tuner != 0 && !tuner->_device_name.empty()) {
            if (dvb_device_index >= 0 && int(dvb_device_current) != dvb_device_index) {
                // Device specified by index, but not this one, try next tuner
                continue;
            }
            else if (dvb_device_index < 0 && !SimilarStrings(tuner->_device_name, tuner_name)) {
                // Device specified by name, but not this one, try next tuner
                // Since the filter names are long and complicated, ignore case and blanks (use SimilarStrings).
                continue;
            }
            // Device found, update device name
            tuner->_device_name = tuner_name;
        }

        // Enumerate all filters with category KSCATEGORY_BDA_NETWORK_PROVIDER
        // and try to build a graph with the tuner.
        std::vector<ComPtr<::IMoniker>> provider_monikers;
        if (!EnumerateDevicesByClass(KSCATEGORY_BDA_NETWORK_PROVIDER, provider_monikers, report)) {
            return false;
        }

        // Loop on all enumerated providers.
        for (size_t provider_index = 0; provider_index < provider_monikers.size(); ++provider_index) {

            // Use specified Tuner or allocate one
            TunerPtr tptr(tuner == 0 ? new Tuner : 0);
            Tuner& tref(tuner == 0 ? *tptr : *tuner);

            // Try to build a graph from this network provider and tuner
            if (!tref.buildGraph(provider_monikers[provider_index].pointer(), tuner_monikers[dvb_device_current].pointer(), report)) {
                continue;
            }

            // Graph correctly built, we keep this one
            tref._is_open = true;
            tref._info_only = true;
            tref._device_name = tuner_name;
            tref._device_info = "";  // none on Windows

            // Add tuner it to response set
            if (tuner_list != 0) {
                // Add the tuner to the vector
                tuner_list->push_back (tptr);
                // Exit network provider loop but continue to search more tuners
                break;
            }
            else {
                // One tuner requested, one found, return
                return true;
            }
        }
    }
    return true;
}


//-----------------------------------------------------------------------------
// Try to build the graph.
// Return true on success, false on error
//-----------------------------------------------------------------------------

bool ts::Tuner::buildGraph (::IMoniker* provider_moniker, ::IMoniker* tuner_moniker, ReportInterface& report)
{
    // ReportInterface to use when errors shall be reported in debug mode only
    ReportInterface& debug_report (report.debug() ? report : NULLREP);

    // Get friendly name of this network provider
    _provider_name = GetStringPropertyBag (provider_moniker, L"FriendlyName", report);
    report.debug ("trying provider filter \"" + _provider_name + "\"");

    // Create an instance of network provider from moniker
    _provider_filter.bindToObject (provider_moniker, ::IID_IBaseFilter, report);
    if (_provider_filter.isNull()) {
        return false;
    }

    // Get class id of the network provider and convert it to our tuner type
    ::GUID provider_guid;
    ::HRESULT hr = _provider_filter->GetClassID (&provider_guid);
    if (!ComSuccess (hr, "cannot get class id of network provider", report)) {
        return false;
    }
    if (!NetworkProviderToTunerType (provider_guid, _tuner_type)) {
        report.debug ("Cannot map " + _provider_name + " (class " + NameGUID (provider_guid) + ") to known tuner type");
        return false;
    }

    // Get interfaces of the network provider filter
    _net_provider.queryInterface (_provider_filter.pointer(), ::IID_IBDA_NetworkProvider, report);
    _tuner.queryInterface (_provider_filter.pointer(), ::IID_ITuner, report);
    if (_net_provider.isNull() || _tuner.isNull()) {
        return false;
    }

    // List all tuning spaces from ITuner interface of network provider.
    // Try to find a default tuning space.
    ComPtr <::IEnumTuningSpaces> enum_tspace;
    hr = _tuner->EnumTuningSpaces (enum_tspace.creator());
    if (ComSuccess (hr, "cannot enumerate tuning spaces from " + _provider_name, report)) {
        if (hr != S_OK) {
            report.debug("no tuning space found");
        }
        else {
            assert (enum_tspace.pointer() != 0);
            ComPtr <::ITuningSpace> tspace;
            while (enum_tspace->Next (1, tspace.creator(), NULL) == S_OK) {
                // Display tuning space in debug mode
                const std::string fname (GetTuningSpaceFriendlyName (tspace.pointer(), report));
                const std::string uname (GetTuningSpaceUniqueName (tspace.pointer(), report));
                report.debug("found tuning space \"" + fname + "\" (" + uname + ")");
                // If a default tuning space is already found, nothing to do
                if (!_tuning_space.isNull()) {
                    continue;
                }
                // Check if this tuning space can be used as default. Get DVB system type:
                // first get IDVBTuningSpace interface of tuning space (may not support it)
                ComPtr <::IDVBTuningSpace> dvb_tspace;
                dvb_tspace.queryInterface (tspace.pointer(), ::IID_IDVBTuningSpace, NULLREP);
                if (dvb_tspace.isNull()) {
                    // Not a DVB tuning space, silently ignore it
                    continue;
                }
                // Get DVB system type
                ::DVBSystemType systype;
                hr = dvb_tspace->get_SystemType (&systype);
                if (!ComSuccess (hr, "cannot get DVB system type from tuning space " + fname, report)) {
                    continue;
                }
                // Check if DVB system type matches our tuner type
                if ((_tuner_type == ts::DVB_S && systype == ::DVB_Satellite) ||
                    (_tuner_type == ts::DVB_T && systype == ::DVB_Terrestrial) ||
                    (_tuner_type == ts::DVB_C && systype == ::DVB_Cable))
                {
                    // Use this tuning space
                    hr = _tuner->put_TuningSpace (tspace.pointer());
                    if (ComSuccess (hr, "fail to set default tuning space " + fname, report)) {
                        _tuning_space = tspace;
                    }
                }
            }
        }
    }

    // Add the delivery system
    switch (_tuner_type) {
        case ts::DVB_S:
            _delivery_systems.set (DS_DVB_S);
            // No way to check if DS_DVB_S2 is supported
            break;
        case ts::DVB_C:
            _delivery_systems.set (DS_DVB_C);
            break;
        case ts::DVB_T:
            _delivery_systems.set (DS_DVB_T);
            break;
        case ts::ATSC:
            _delivery_systems.set (DS_ATSC);
            break;
    }

    // Create an instance of tuner from moniker
    _tuner_filter.bindToObject (tuner_moniker, ::IID_IBaseFilter, report);
    if (_tuner_filter.isNull()) {
        return false;
    }

    // Create the Filter Graph Manager
    _graph.createInstance (::CLSID_FilterGraph, ::IID_IGraphBuilder, report);
    if (_graph.isNull()) {
        return false;
    }

    // Get required interfaces on the graph manager
    _media_control.queryInterface (_graph.pointer(), ::IID_IMediaControl, report);
    if (_media_control.isNull()) {
        return false;
    }

    // Add the filters in the graph
    hr = _graph->AddFilter (_provider_filter.pointer(), L"NetworkProvider");
    if (!ComSuccess (hr, "IFilterGraph::AddFilter (NetworkProvider)", report)) {
        return false;
    }
    hr = _graph->AddFilter (_tuner_filter.pointer(), L"Tuner");
    if (!ComSuccess (hr, "IFilterGraph::AddFilter (Tuner)", report)) {
        return false;
    }

    // Try to start a graph: network provider -> tuner
    if (!ConnectFilters (_graph.pointer(), _provider_filter.pointer(), _tuner_filter.pointer(), debug_report)) {
        // Connection fails. However, this may not be significant with some
        // tuners (ie. Hauppauge WinTV Nova-HD-S2). These tuners require
        // that a tuning request shall be provided in order to allow the
        // connection between the network provider and the tuner filters.
        // Consequently, we try to issue a tuning request with the default
        // parameters. However, before trying to tune, we must have
        // successfully found a default tuning space.
        if (_tuning_space.isNull()) {
            // No way to setup a tune request, silently give up this network provider
            return false;
        }
        TunerParameters* params = TunerParameters::Factory (_tuner_type);
        if (params == 0) {
            report.debug ("cannot build tuner parameters for these network provider");
            // The tuner is not compatible with this network provider.
            // Silently give up this network provider.
            return false;
        }
        // Issue a tuning request, ignore errors
        internalTune (*params, debug_report);
        // Then retry to start the graph
        if (!ConnectFilters (_graph.pointer(), _provider_filter.pointer(), _tuner_filter.pointer(), debug_report)) {
            // The tuner is not compatible with this network provider.
            // Silently give up this network provider.
            return false;
        }
    }

    // Get current (default?) tuning space of this network provider
    hr = _tuner->get_TuningSpace (_tuning_space.creator());
    if (!ComSuccess (hr, "ITuner::get_TuningSpace", report)) {
        return false;
    }

    // Get tuning space names
    _tuning_space_fname = GetTuningSpaceFriendlyName (_tuning_space.pointer(), report);
    _tuning_space_uname = GetTuningSpaceUniqueName (_tuning_space.pointer(), report);

    // Good start, we successfully connected an network provider to a tuner filter.
    report.debug ("using provider filter \"" + _provider_name +
                    "\", tuning space \"" + _tuning_space_uname +
                    "\" (\"" + _tuning_space_fname + "\")");

    // Try to build the rest of the graph starting at tuner filter.
    // Usually work with Terratec driver for instance.
    report.debug ("trying direct connection from tuner (no receiver)");
    bool graph_done = buildCaptureGraph (_tuner_filter, report);

    // If the tuner cannot be directly connected to the rest of the
    // graph, we need to find a specific "receiver" filter (usually
    // provided by the same vendor as the tuner filter). Needed by
    // Hauppauge or Pinnacle drivers for instance.
    if (!graph_done) {

        // Enumerate all filters with category KSCATEGORY_BDA_RECEIVER_COMPONENT
        std::vector <ComPtr <::IMoniker>> receiver_monikers;
        if (!EnumerateDevicesByClass (KSCATEGORY_BDA_RECEIVER_COMPONENT, receiver_monikers, report)) {
            return false;
        }

        // Loop on all enumerated receiver filters
        for (size_t receiver_index = 0; !graph_done && receiver_index < receiver_monikers.size(); ++receiver_index) {

            // Get friendly name of this network provider
            std::string receiver_name (GetStringPropertyBag (receiver_monikers[receiver_index].pointer(), L"FriendlyName", debug_report));
            report.debug ("trying receiver filter \"" + receiver_name + "\"");

            // Create an instance of this receiver filter from moniker
            ComPtr <::IBaseFilter> receiver_filter;
            receiver_filter.bindToObject (receiver_monikers[receiver_index].pointer(), ::IID_IBaseFilter, debug_report);
            if (receiver_filter.isNull()) {
                continue; // give up this receiver filter
            }

            // Add the filter in the graph
            hr = _graph->AddFilter (receiver_filter.pointer(), L"Receiver");
            if (!ComSuccess (hr, "IFilterGraph::AddFilter (Receiver)", report)) {
                continue; // give up this receiver filter
            }

            // Try to connect the tuner to the receiver
            if (!ConnectFilters (_graph.pointer(), _tuner_filter.pointer(), receiver_filter.pointer(), debug_report)) {
                // This receiver is not compatible, remove it from the graph
                _graph->RemoveFilter (receiver_filter.pointer());
                continue;
            }

            // Try to build the rest of the graph
            if (buildCaptureGraph (receiver_filter, report)) {
                graph_done = true;
                report.debug ("using receiver filter \"" + receiver_name + "\"");
            }
        }
    }
    if (!graph_done) {
        return false;
    }

    // Locate all instances of some interfaces in the tuner topology
    ComVectorClear (_demods);
    ComVectorClear (_demods2);
    ComVectorClear (_sigstats);
    ComVectorClear (_tunprops);

    // Lookup all internal nodes in the BDA topology
    ComPtr <::IBDA_Topology> topology;
    topology.queryInterface (_tuner_filter.pointer(), ::IID_IBDA_Topology, NULLREP);
    if (!topology.isNull()) {
        // Get node types
        static const ::ULONG MAX_NODES = 64;
        ::ULONG count = MAX_NODES;
        ::ULONG types [MAX_NODES];
        if (SUCCEEDED (topology->GetNodeTypes (&count, MAX_NODES, types))) {
            // Enumerate all node types
            for (::ULONG n = 0; n < count; ++n) {
                // Get control node for this type
                ComPtr <::IUnknown> cnode;
                if (SUCCEEDED (topology->GetControlNode (0, 1, types[n], cnode.creator()))) {
                    findTunerSubinterfaces (cnode);
                }
            }
        }
    }

    // Look at all connected pins
    ComPtr <::IEnumPins> enum_pins;
    if (SUCCEEDED (_tuner_filter->EnumPins (enum_pins.creator()))) {
        // Enumerate all pins in tuner filter
        ComPtr <::IPin> pin;
        while (enum_pins->Next (1, pin.creator(), NULL) == S_OK) {
            // Check if this pin is connected
            ComPtr <::IPin> partner;
            if (SUCCEEDED (pin->ConnectedTo (partner.creator()))) {
                findTunerSubinterfaces (pin);
            }
        }
    }

    report.debug ("IBDA_DigitalDemodulator in tuner: %" FMT_SIZE_T "u", _demods.size());
    report.debug ("IBDA_DigitalDemodulator2 in tuner: %" FMT_SIZE_T "u", _demods2.size());
    report.debug ("IBDA_SignalStatistics in tuner: %" FMT_SIZE_T "u", _sigstats.size());
    report.debug ("IKsPropertySet in tuner: %" FMT_SIZE_T "u", _tunprops.size());

    return true;
}


//-----------------------------------------------------------------------------
// Try to build the part of the graph starting at the tee filter.
// The specified base filter is either the tuner filter or some
// other intermediate receiver filter downstream the tuner.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::Tuner::buildCaptureGraph (const ComPtr <::IBaseFilter>& base_filter, ReportInterface& report)
{
    // ReportInterface to use when errors shall be reported in debug mode only
    ReportInterface& debug_report (report.debug() ? report : NULLREP);

    // Create a DirectShow System Device Enumerator
    ComPtr <::ICreateDevEnum> enum_devices (::CLSID_SystemDeviceEnum, ::IID_ICreateDevEnum, report);
    if (enum_devices.isNull()) {
        return false;
    }

    // Create an "infinite tee filter"
    ComPtr <::IBaseFilter> tee_filter (CLSID_InfTee, IID_IBaseFilter, report);
    if (tee_filter.isNull()) {
        return false;
    }

    // Add the tee filter to the graph
    ::HRESULT hr = _graph->AddFilter (tee_filter.pointer(), L"Tee");
    if (!ComSuccess (hr, "IFilterGraph::AddFilter (Tee)", report)) {
        return false;
    }

    // After this point, we cannot simply return false on error since
    // the graph needs some cleanup.

    // Try to connect the "base" filter (tuner or receiver) to the tee filter.
    bool ok = ConnectFilters (_graph.pointer(), base_filter.pointer(), tee_filter.pointer(), debug_report);
    
    // Create branch A of graph: Create a sink filter, add it to the graph and connect it to the tee.
    ComPtr <SinkFilter> sink_filter (new SinkFilter (report));
    if (ok) {
        CheckNonNull (sink_filter.pointer());
        hr = _graph->AddFilter (sink_filter.pointer(), L"Sink/Capture");
        ok = ComSuccess (hr, "IFilterGraph::AddFilter (Sink)", report);
    }
    ok = ok && ConnectFilters (_graph.pointer(), tee_filter.pointer(), sink_filter.pointer(), debug_report);

    // Create branch B of graph: Create an MPEG-2 demultiplexer
    ComPtr <::IBaseFilter> demux_filter (::CLSID_MPEG2Demultiplexer, ::IID_IBaseFilter, report);
    ok = ok && !demux_filter.isNull();
    if (ok) {
        hr = _graph->AddFilter (demux_filter.pointer(), L"Demux");
        ok = ComSuccess (hr, "IFilterGraph::AddFilter (Demux)", report);
    }
    ok = ok && ConnectFilters (_graph.pointer(), tee_filter.pointer(), demux_filter.pointer(), debug_report);

    // Now, we need to connect a Transport Information Filter (TIF).
    // There is no predefined CLSID for this one and we must loop on all
    // filters with category KSCATEGORY_BDA_TRANSPORT_INFORMATION
    ComPtr <::IEnumMoniker> enum_tif;
    if (ok) {
        hr = enum_devices->CreateClassEnumerator (KSCATEGORY_BDA_TRANSPORT_INFORMATION, enum_tif.creator(), 0);
        ok = ComSuccess (hr, "CreateClassEnumerator (for TIF)", report) && hr == S_OK;
    }

    // Loop on all enumerated TIF
    ComPtr <::IMoniker> tif_moniker;
    bool tif_found = false;
    while (ok && !tif_found && enum_tif->Next (1, tif_moniker.creator(), NULL) == S_OK) {

        // Get friendly name of this TIF
        std::string tif_name (GetStringPropertyBag (tif_moniker.pointer(), L"FriendlyName", report));
        report.debug ("trying TIF \"" + tif_name + "\"");

        // Create an instance of this TIF from moniker
        ComPtr <::IBaseFilter> tif_filter;
        tif_filter.bindToObject (tif_moniker.pointer(), ::IID_IBaseFilter, report);
        if (tif_filter.isNull()) {
            continue; // give up this TIF
        }

        // Add the TIF in the graph
        hr = _graph->AddFilter (tif_filter.pointer(), L"TIF");
        if (!ComSuccess (hr, "IFilterGraph::AddFilter (TIF)", report)) {
            continue; // give up this TIF
        }

        // Try to connect demux filter to tif
        if (ConnectFilters (_graph.pointer(), demux_filter.pointer(), tif_filter.pointer(), debug_report)) {
            tif_found = true;
            report.debug ("using TIF \"" + tif_name + "\"");
        }
        else {
            // This tif is not compatible, remove it from the graph
            _graph->RemoveFilter (tif_filter.pointer());
        }
    }

    // If successful so far, done
    if (tif_found) {
        _sink_filter = sink_filter;
        return true;
    }

    // Not successful, cleanup everything.
    // Cleanup the graph downstream the tuner filter.
    // This will also remove any optional receiver filter
    // between the tuner and the tee.
    CleanupDownstream (_graph.pointer(), _tuner_filter.pointer(), debug_report);

    // Remove all created filters from the graph. Ignore errors.
    // This is necessary if a filter was created and added to the graph but
    // not connected (if connected, it was removed by CleanupDownstream).
    if (!tee_filter.isNull()) {
        _graph->RemoveFilter (tee_filter.pointer());
    }
    if (!sink_filter.isNull()) {
        _graph->RemoveFilter (sink_filter.pointer());
    }
    if (!demux_filter.isNull()) {
        _graph->RemoveFilter (demux_filter.pointer());
    }

    return false;
}
