//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//
// Windows implementation of the ts::Tuner class.
//
//-----------------------------------------------------------------------------

#include "tsTuner.h"
#include "tsTime.h"
#include "tsNullReport.h"
#include "tsSysUtils.h"
#include "tsWinUtils.h"
#include "tsComPtr.h"
#include "tsDirectShowGraph.h"
#include "tsDirectShowUtils.h"
#include "tsSinkFilter.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr size_t ts::Tuner::DEFAULT_SINK_QUEUE_SIZE;
#endif


//-----------------------------------------------------------------------------
// Windows version of the syste guts class.
//-----------------------------------------------------------------------------
//
// A DirectShow graph for TS capture is usually made of the following filters:
// - Network provider (typically "Microsoft DVBx Network Provider")
// - Tuner (typically provided by tuner hardware vendor as "BDA driver")
// - Receiver (optional, also provided by tuner hardware vendor)
// - Tee filter, creating two branches:
// - Branch A: actual capture of TS packets
//   - SinkFiler (provided by TSDuck)
// - Branch B: MPEG-2 demux, actually unused but required by the graph
//   - MPEG-2 demultiplexer
//   - TIF (Transport Information Filter)

class ts::Tuner::Guts
{
    TS_NOBUILD_NOCOPY(Guts);
private:
    Tuner*                         _parent;             // Parent tuner.
public:
    size_t                         sink_queue_size;     // Media sample queue size
    DirectShowGraph                graph;               // The filter graph
    ComPtr<SinkFilter>             sink_filter;         // Sink filter to TSDuck
    ComPtr<::IBaseFilter>          provider_filter;     // Network provider filter
    ComPtr<::IBDA_NetworkProvider> net_provider;        // ... interface of provider_filter
    ComPtr<::ITuner>               tuner;               // ... interface of provider_filter
    ComPtr<::ITuningSpace>         tuning_space;        // ... associated to provider_filter
    UString                        tuning_space_fname;  // ... friendly name
    UString                        tuning_space_uname;  // ... unique name
    ComPtr<::IBaseFilter>          tuner_filter;        // Tuner filter
    std::vector<ComPtr<::IBDA_DigitalDemodulator>>  demods;   // ... all its demod interfaces
    std::vector<ComPtr<::IBDA_DigitalDemodulator2>> demods2;  // ... all its demod (2nd gen) interfaces
    std::vector<ComPtr<::IBDA_SignalStatistics>>    sigstats; // ... all its signal stat interfaces
    std::vector<ComPtr<::IKsPropertySet>>           tunprops; // ... all its property set interfaces

    // Constructor and destructor.
    Guts(Tuner* tuner);
    ~Guts();

    // Try to build the graph.
    // Return true on success, false on error
    bool buildGraph(::IMoniker* tuner_moniker, Report&);

    // Try to build the part of the graph starting at the tee filter.
    // The specified base filter is either the tuner filter or some
    // other intermediate receiver filter downstream the tuner.
    // Return true on success, false on error.
    bool buildCaptureGraph(const ComPtr <::IBaseFilter>&, Report&);

    // Internal tune method, works also if the tuner is not in open state.
    // Return true on success, false on errors
    bool internalTune(ModulationArgs&, Report&);

    // Get signal strength in mdB.
    // Return true if found, false if not found.
    bool getSignalStrength_mdB(::LONG&);

    // Locate all known interfaces in a pin or node of the tuner filter.
    // Add found interfaces in demods, demods2, sigstats, _tunprops.
    // Ignore errors.
    template <class COMCLASS>
    void findTunerSubinterfaces(ComPtr<COMCLASS>&);

    // Find one or more tuners. Exactly one of Tuner* or TunerPtrVector* must be non-zero.
    // If Tuner* is non-zero, find the first tuner (matching _device_name if not empty).
    // If _device_name is ":integer", use integer as device index in list of DVB devices.
    // If TunerPtrVector* is non- zero, find all tuners in the system.
    // Return true on success, false on error.
    static bool FindTuners(DuckContext& duck, Tuner*, TunerPtrVector*, Report&);

    // Search criteria for properties.
    enum PropSearch {psFIRST, psLAST, psLOWEST, psHIGHEST};

    // Search all IKsPropertySet in the tuner until the specified data is found.
    // Return true if found, false if not found.
    template <typename VALTYPE>
    bool searchTunerProperty(VALTYPE& retvalue, PropSearch searchtype, const ::GUID& propset, ::DWORD propid);

    // Search a property, until found, in all filters of "ivector" (typically one of demods, demods2, sigstats)
    // and then in tuner properties (_tunprops). When searching in "ivector", use the method "get". When searching
    // in _tunprops, use "propset" and "propid". Return true when the property is found (property value returned
    // in "retvalue") or false when the property is not found.
    template <typename VALTYPE, typename IVALTYPE, class FILTER>
    bool searchProperty(VALTYPE& retvalue,
                        PropSearch searchtype,
                        const std::vector<ComPtr<FILTER>>& ivector,
                        ::HRESULT (FILTER::*get)(IVALTYPE*),
                        const ::GUID& propset,
                        ::DWORD propid);

    // Same one with additional handling of unknown return value.
    template <typename VALTYPE, typename ARGTYPE, typename IVALTYPE, class FILTER>
    bool searchProperty(VALTYPE unset,
                        Variable<ARGTYPE>& parameter,
                        PropSearch searchtype,
                        bool reset_unknown,
                        const std::vector<ComPtr<FILTER>>& ivector,
                        ::HRESULT (FILTER::*getmethod)(IVALTYPE*),
                        const ::GUID& propset,
                        ::DWORD propid);

private:
    // Repeatedly called when searching for a propery.
    // Each "val" is proposed until "terminated" is returned as true.
    template <typename T>
    static void SelectProperty(bool& terminated, bool& found, T& retvalue, T val, PropSearch);
};


//-----------------------------------------------------------------------------
// System guts constructor and destructor.
//-----------------------------------------------------------------------------

ts::Tuner::Guts::Guts(Tuner* tuner) :
    _parent(tuner),
    sink_queue_size(DEFAULT_SINK_QUEUE_SIZE),
    graph(),
    sink_filter(),
    provider_filter(),
    net_provider(),
    tuner(),
    tuning_space(),
    tuning_space_fname(),
    tuning_space_uname(),
    tuner_filter(),
    demods(),
    demods2(),
    sigstats(),
    tunprops()
{
}

ts::Tuner::Guts::~Guts()
{
}


//-----------------------------------------------------------------------------
// System guts allocation.
//-----------------------------------------------------------------------------

void ts::Tuner::allocateGuts()
{
    _guts = new Guts(this);
}

void ts::Tuner::deleteGuts()
{
    delete _guts;
}


//-----------------------------------------------------------------------------
// Set the max number of queued media samples (Windows-specific).
//-----------------------------------------------------------------------------

void ts::Tuner::setSinkQueueSize(size_t s)
{
    _guts->sink_queue_size = s;
}


//-----------------------------------------------------------------------------
// Get the list of all existing DVB tuners.
//-----------------------------------------------------------------------------

bool ts::Tuner::GetAllTuners(DuckContext& duck, TunerPtrVector& tuners, Report& report)
{
    return Guts::FindTuners(duck, nullptr, &tuners, report);
}


//-----------------------------------------------------------------------------
// Open the tuner.
//-----------------------------------------------------------------------------

bool ts::Tuner::open(const UString& device_name, bool info_only, Report& report)
{
    if (_is_open) {
        report.error(u"tuner already open");
        return false;
    }
    _device_name = device_name;
    if (!Guts::FindTuners(_duck, this, nullptr, report)) {
        return false;
    }
    else if (_is_open) {
        _info_only = info_only;
        return true;
    }
    else if (device_name.empty()) {
        report.error(u"No tuner device");
        return false;
    }
    else {
        report.error(u"device \"%s\" not found", {device_name});
        return false;
    }
}


//-----------------------------------------------------------------------------
// Close tuner.
//-----------------------------------------------------------------------------

bool ts::Tuner::close(Report& report)
{
    _is_open = false;
    _device_name.clear();
    _device_info.clear();
    _guts->graph.clear(report);
    _guts->sink_filter.release();
    _guts->provider_filter.release();
    _guts->net_provider.release();
    _guts->tuner.release();
    _guts->tuning_space.release();
    _guts->tuning_space_fname.clear();
    _guts->tuning_space_uname.clear();
    _guts->tuner_filter.release();
    _guts->demods.clear();
    _guts->demods2.clear();
    _guts->sigstats.clear();
    _guts->tunprops.clear();
    return true;
}


//-----------------------------------------------------------------------------
// Search all IKsPropertySet in the tuner until the specified data is found.
// Return true if found, false if not found.
//-----------------------------------------------------------------------------

template <typename VALTYPE>
bool ts::Tuner::Guts::searchTunerProperty(VALTYPE& retvalue, PropSearch searchtype, const ::GUID& propset, ::DWORD propid)
{
    bool found = false;
    bool terminated = false;

    for (size_t i = 0; !terminated && i < tunprops.size(); ++i) {
        VALTYPE val;
        ::DWORD retsize = sizeof(val);
        if (SUCCEEDED(tunprops[i]->Get(propset, propid, NULL, 0, &val, retsize, &retsize))) {
            SelectProperty(terminated, found, retvalue, val, searchtype);
        }
    }
    return found;
}


//-----------------------------------------------------------------------------
// Search a property, until found, in "ivector" and then _tunprops.
//-----------------------------------------------------------------------------

template <typename VALTYPE, typename IVALTYPE, class FILTER>
bool ts::Tuner::Guts::searchProperty(VALTYPE& retvalue,
                                     PropSearch searchtype,
                                     const std::vector<ComPtr<FILTER>>& ivector,
                                     ::HRESULT (FILTER::*getmethod)(IVALTYPE*),
                                     const ::GUID& propset,
                                     ::DWORD propid)
{
    bool found = false;
    bool terminated = false;
    for (size_t i = 0; !terminated && i < ivector.size(); ++i) {
        IVALTYPE val;
        FILTER* filter = ivector[i].pointer();
        if (SUCCEEDED((filter->*getmethod)(&val))) {
            SelectProperty<VALTYPE>(terminated, found, retvalue, val, searchtype);
        }
    }
    for (size_t i = 0; !terminated && i < tunprops.size(); ++i) {
        VALTYPE val;
        ::DWORD retsize = sizeof(val);
        if (SUCCEEDED(tunprops[i]->Get(propset, propid, NULL, 0, &val, retsize, &retsize))) {
            SelectProperty<VALTYPE>(terminated, found, retvalue, val, searchtype);
        }
    }
    return found;
}


//-----------------------------------------------------------------------------
// Same one with additional handling of unknown return value.
//-----------------------------------------------------------------------------

template <typename VALTYPE, typename ARGTYPE, typename IVALTYPE, class FILTER>
bool ts::Tuner::Guts::searchProperty(VALTYPE unset,
                                     Variable<ARGTYPE>& parameter,
                                     PropSearch searchtype,
                                     bool reset_unknown,
                                     const std::vector<ComPtr<FILTER>>& ivector,
                                     ::HRESULT (FILTER::*getmethod)(IVALTYPE*),
                                     const ::GUID& propset,
                                     ::DWORD propid)
{
    VALTYPE retvalue = unset;
    bool found = searchProperty(retvalue, searchtype, ivector, getmethod, propset, propid);
    if (found && retvalue != unset) {
        parameter = ARGTYPE(retvalue);
    }
    else if (reset_unknown) {
        parameter.reset();
    }
    return found;
}


//-----------------------------------------------------------------------------
// Repeatedly called when searching for a propery.
//-----------------------------------------------------------------------------

template <typename T>
void ts::Tuner::Guts::SelectProperty(bool& terminated, bool& found, T& retvalue, T val, PropSearch searchtype)
{
    switch (searchtype) {
        case psFIRST:
            retvalue = val;
            terminated = true;
            break;
        case psLAST:
            retvalue = val;
            break;
        case psHIGHEST:
            if (!found || val > retvalue) {
                retvalue = val;
            }
            break;
        case psLOWEST:
            if (!found || val < retvalue) {
                retvalue = val;
            }
            break;
    }
    found = true;
}


//-----------------------------------------------------------------------------
// Check if a signal is present and locked
//-----------------------------------------------------------------------------

bool ts::Tuner::signalLocked(Report& report)
{
    if (!_is_open) {
        report.error(u"tuner not open");
        return false;
    }

    ::BOOL locked = 0;
    const bool found = _guts->searchProperty(locked, Guts::psHIGHEST,
                                             _guts->sigstats, &::IBDA_SignalStatistics::get_SignalLocked,
                                             KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_LOCKED);
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

bool ts::Tuner::Guts::getSignalStrength_mdB(::LONG& strength)
{
    const bool found = searchProperty(strength, psHIGHEST,
                                      sigstats, &::IBDA_SignalStatistics::get_SignalStrength,
                                      KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_STRENGTH);
    return found;
}


//-----------------------------------------------------------------------------
// Return signal strength, in percent (0=bad, 100=good)
// Return a negative value on error.
//-----------------------------------------------------------------------------

int ts::Tuner::signalStrength(Report& report)
{
    if (!_is_open) {
        report.error(u"tuner not open");
        return false;
    }

    // Use -100 dB (-100 000 mdB) as zero (null strength)
    // Avoid returning negative value on success.
    ::LONG strength;
    return _guts->getSignalStrength_mdB(strength) ? std::max(0, 100 + int(strength) / 1000) : -1;
}


//-----------------------------------------------------------------------------
// Return signal quality, in percent (0=bad, 100=good)
// Return a negative value on error.
//-----------------------------------------------------------------------------

int ts::Tuner::signalQuality(Report& report)
{
    if (!_is_open) {
        report.error(u"tuner not open");
        return false;
    }

    ::LONG quality = 0;
    const bool found = _guts->searchProperty(quality, Guts::psHIGHEST,
                                             _guts->sigstats, &::IBDA_SignalStatistics::get_SignalQuality,
                                             KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_QUALITY);
    return found ? int(quality) : -1;
}


//-----------------------------------------------------------------------------
// Get the current tuning parameters
//-----------------------------------------------------------------------------

bool ts::Tuner::getCurrentTuning(ModulationArgs& params, bool reset_unknown, Report& report)
{
    if (!_is_open) {
        report.error(u"tuner not open");
        return false;
    }

    // We do not know which delivery system is current. Use default one.
    if (!params.delivery_system.set() || !_delivery_systems.contains(params.delivery_system.value())) {
        params.delivery_system = _delivery_systems.preferred();
    }
    const TunerType ttype = TunerTypeOf(params.delivery_system.value());

    // Search individual tuning parameters
    bool found = false;
    switch (ttype) {

        case TT_DVB_S: {
            // Note: it is useless to get the frequency of a DVB-S tuner since it
            // returns the intermediate frequency and there is no unique satellite
            // frequency for a given intermediate frequency.
            if (reset_unknown) {
                params.frequency.reset();
                params.satellite_number.reset();
                params.lnb.reset();
            }
            // Spectral inversion
            _guts->searchProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // Symbol rate
            _guts->searchProperty<::ULONG>(
                0, params.symbol_rate, Guts::psHIGHEST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_SymbolRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SYMBOL_RATE);
            // Inner FEC
            _guts->searchProperty<::BinaryConvolutionCodeRate>(
                ::BDA_BCC_RATE_NOT_SET, params.inner_fec, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_InnerFECRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE);
            // Modulation
            _guts->searchProperty<::ModulationType>(
                ::BDA_MOD_NOT_SET, params.modulation, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_ModulationType,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE);
            // Delivery system. Found no way to get DVB-S vs. DVB-S2 on Windows.
            // Make a not quite correct assumption, based on modulation type.
            if (params.modulation.set()) {
                params.delivery_system = params.modulation == QPSK ? DS_DVB_S : DS_DVB_S2;
            }
            else if (reset_unknown) {
                params.delivery_system.reset();
            }
            // DVB-S2 pilot
            _guts->searchProperty<::Pilot>(
                ::BDA_PILOT_NOT_SET, params.pilots, Guts::psFIRST, reset_unknown,
                _guts->demods2, &::IBDA_DigitalDemodulator2::get_Pilot,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_PILOT);
            // DVB-S2 roll-off factor
            _guts->searchProperty<::RollOff>(
                ::BDA_ROLL_OFF_NOT_SET, params.roll_off, Guts::psFIRST, reset_unknown,
                _guts->demods2, &::IBDA_DigitalDemodulator2::get_RollOff,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_ROLL_OFF);
            break;
        }

        case TT_DVB_C: {
            if (reset_unknown) {
                params.frequency.reset();
            }
            // Spectral inversion
            _guts->searchProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // Symbol rate
            _guts->searchProperty<::ULONG>(
                0, params.symbol_rate, Guts::psHIGHEST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_SymbolRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SYMBOL_RATE);
            // Inner FEC
            _guts->searchProperty<::BinaryConvolutionCodeRate>(
                ::BDA_BCC_RATE_NOT_SET, params.inner_fec, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_InnerFECRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE);
            // Modulation
            _guts->searchProperty<::ModulationType>(
                ::BDA_MOD_NOT_SET, params.modulation, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_ModulationType,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE);
            break;
        }

        case TT_DVB_T: {
            if (reset_unknown) {
                params.frequency.reset();
            }
            // Spectral inversion
            _guts->searchProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // High priority FEC
            _guts->searchProperty<::BinaryConvolutionCodeRate>(
                ::BDA_BCC_RATE_NOT_SET, params.fec_hp, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_InnerFECRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE);
            // Modulation
            _guts->searchProperty<::ModulationType>(
                ::BDA_MOD_NOT_SET, params.modulation, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_ModulationType,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE);
            // Other DVB-T parameters, not supported in IBDA_DigitalDemodulator
            // but which may be supported as properties.
            ::TransmissionMode tm = ::BDA_XMIT_MODE_NOT_SET;
            found = _guts->searchTunerProperty(tm, Guts::psFIRST, KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_TRANSMISSION_MODE);
            if (found && tm != ::BDA_XMIT_MODE_NOT_SET) {
                params.transmission_mode = ts::TransmissionMode(tm);
            }
            else if (reset_unknown) {
                params.transmission_mode.reset();
            }
            ::GuardInterval gi = ::BDA_GUARD_NOT_SET;
            found = _guts->searchTunerProperty(gi, Guts::psFIRST, KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_GUARD_INTERVAL);
            if (found && gi != ::BDA_GUARD_NOT_SET) {
                params.guard_interval = ts::GuardInterval(gi);
            }
            else if (reset_unknown) {
                params.guard_interval.reset();
            }
            // Other DVB-T parameters, not supported at all
            params.bandwidth.reset();
            params.hierarchy.reset();
            params.fec_lp.reset();
            params.plp.reset();
            break;
        }

        case TT_ATSC: {
            if (reset_unknown) {
                params.frequency.reset();
            }
            // Spectral inversion
            _guts->searchProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // Modulation
            _guts->searchProperty<::ModulationType>(
                ::BDA_MOD_NOT_SET, params.modulation, Guts::psFIRST, reset_unknown,
                _guts->demods, &::IBDA_DigitalDemodulator::get_ModulationType,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE);
            break;
        }

        case TT_UNDEFINED:
        default: {
            report.error(u"cannot convert BDA tuning parameters to %s parameters", {TunerTypeEnum.name(ttype)});
            return false;
        }
    }

    return true;
}


//-----------------------------------------------------------------------------
// Tune to the specified parameters and start receiving.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::tune(ModulationArgs& params, Report& report)
{
    // Initial parameter checks.
    return checkTuneParameters(params, report) && _guts->internalTune(params, report);
}


//-----------------------------------------------------------------------------
// Internal tune method, works also if the tuner is not in open state.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::Guts::internalTune(ModulationArgs& params, Report& report)
{
    // Create a DirectShow tune request
    ComPtr<::ITuneRequest> tune_request;
    if (!CreateTuneRequest(_parent->_duck, tune_request, tuning_space.pointer(), params, report)) {
        return false;
    }
    assert(!tune_request.isNull());

    // Tune to transponder
    ::HRESULT hr = tuner->put_TuneRequest(tune_request.pointer());
    return ComSuccess(hr, u"DirectShow tuning error", report);
}


//-----------------------------------------------------------------------------
// Start receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::start(Report& report)
{
    if (!_is_open) {
        report.error(u"tuner not open");
        return false;
    }

    // Set media samples queue size.
    _guts->sink_filter->SetMaxMessages(_guts->sink_queue_size);

    // Run the graph.
    if (!_guts->graph.run(report)) {
        return false;
    }

    // If the tuner was previously started/stopped on a frequency with signal on it,
    // it has been observed that remaining packets from the previous run were still
    // there. Wait a little bit and reflush after Run() to avoid that.
    // Yes, this is a horrible hack, but if you have a better fix...
    SleepThread(50); // milliseconds
    _guts->sink_filter->Flush();

    // If a signal timeout was specified, read a packet with timeout
    if (_signal_timeout > 0) {
        TSPacket pack;
        if (_guts->sink_filter->Read(&pack, sizeof(pack), _signal_timeout) == 0) {
            if (!_signal_timeout_silent) {
                report.error(u"no input DVB signal after %'d milliseconds", {_signal_timeout});
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

bool ts::Tuner::stop(Report& report)
{
    return _is_open && _guts->graph.stop(report);
}


//-----------------------------------------------------------------------------
// Timeout for receive operation (none by default).
// If zero, no timeout is applied.
// Return true on success, false on errors.
//-----------------------------------------------------------------------------

bool ts::Tuner::setReceiveTimeout(MilliSecond timeout, Report&)
{
    _receive_timeout = timeout;
    return true;
}


//-----------------------------------------------------------------------------
// Read complete 188-byte TS packets in the buffer and return the
// number of actually received packets (in the range 1 to max_packets).
// Returning zero means error or end of input.
//-----------------------------------------------------------------------------

size_t ts::Tuner::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, Report& report)
{
    if (!_is_open) {
        report.error(u"tuner not open");
        return 0;
    }

    // Read packets from the tuner device

    size_t got_size;

    if (_receive_timeout <= 0) {
        got_size = _guts->sink_filter->Read(buffer, max_packets * PKT_SIZE);
    }
    else {
        const Time limit(Time::CurrentUTC() + _receive_timeout);
        got_size = _guts->sink_filter->Read(buffer, max_packets * PKT_SIZE, _receive_timeout);
        if (got_size == 0 && Time::CurrentUTC() >= limit) {
            report.error(u"receive timeout on " + _device_name);
        }
    }

    return got_size / PKT_SIZE;
}


//-----------------------------------------------------------------------------
// Display the characteristics and status of the tuner.
//-----------------------------------------------------------------------------

std::ostream& ts::Tuner::displayStatus(std::ostream& strm, const UString& margin, Report& report)
{
    if (!_is_open) {
        report.error(u"tuner not open");
        return strm;
    }

    strm << margin << "Signal locked:    " << UString::YesNo(signalLocked(report)) << std::endl;
    int quality = signalQuality(report);
    if (quality >= 0) {
        strm << margin << "Signal quality:   " << quality << " %" << std::endl;
    }
    ::LONG strength;
    if (_guts->getSignalStrength_mdB(strength)) {
        strm << margin << "Signal strength:  " << UString::Decimal(strength) << " milli dB" << std::endl;
    }
    strm << std::endl << margin << "DirectShow graph:" << std::endl;
    _guts->graph.display(strm, report, margin + u"  ", true);

    return strm;
}


//-----------------------------------------------------------------------------
// Locate all known interfaces in a pin or node of the tuner filter.
// Ignore errors.
//-----------------------------------------------------------------------------

template <class COMCLASS>
void ts::Tuner::Guts::findTunerSubinterfaces(ComPtr<COMCLASS>& obj)
{
#define _D_(iface,vect)                                           \
    {                                                             \
        ComPtr<iface> iobj;                                       \
        iobj.queryInterface(obj.pointer(), IID_##iface, NULLREP); \
        if (!iobj.isNull()) {                                     \
            vect.push_back(iobj);                                 \
        }                                                         \
    }

    _D_(IBDA_DigitalDemodulator,  demods);
    _D_(IBDA_DigitalDemodulator2, demods2);
    _D_(IBDA_SignalStatistics,    sigstats);
    _D_(IKsPropertySet,           tunprops);
#undef _D_
}


//-----------------------------------------------------------------------------
// Private static method: Find one or more tuners.
//-----------------------------------------------------------------------------

bool ts::Tuner::Guts::FindTuners(DuckContext& duck, Tuner* tuner, TunerPtrVector* tuner_list, Report& report)
{
    // Report to use when errors shall be reported in debug mode only
    Report& debug_report(report.debug() ? report : NULLREP);

    // Exactly one of Tuner* or TunerPtrVector* must be non-zero.
    assert(tuner == nullptr || tuner_list == nullptr);
    assert(tuner != nullptr || tuner_list != nullptr);

    // Reset content of tuner vector
    if (tuner_list != nullptr) {
        tuner_list->clear();
    }

    // Check if tuner device name is ":integer"
    int dvb_device_index = -1;
    if (tuner != nullptr && !tuner->_device_name.empty() && tuner->_device_name[0] == ':') {
        tuner->_device_name.substr(1).toInteger(dvb_device_index);
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
        const UString tuner_name(GetStringPropertyBag(tuner_monikers[dvb_device_current].pointer(), L"FriendlyName", debug_report));
        report.debug(u"found tuner filter \"%s\"", {tuner_name});

        // If a device name was specified, filter this name.
        if (tuner != nullptr && !tuner->_device_name.empty()) {
            if (dvb_device_index >= 0 && int(dvb_device_current) != dvb_device_index) {
                // Device specified by index, but not this one, try next tuner
                continue;
            }
            else if (dvb_device_index < 0 && !tuner_name.similar(tuner->_device_name)) {
                // Device specified by name, but not this one, try next tuner
                // Since the filter names are long and complicated, ignore case and blanks, use UString::similar().
                continue;
            }
            // Device found, update device name
            tuner->_device_name = tuner_name;
        }

        // If we search one specific tuner (tuner != nullptr), use this one.
        // If we are building a list of all tuners (tuner_list != nullptr), allocate a new tuner.
        TunerPtr tptr(tuner == nullptr ? new Tuner(duck) : 0);
        Tuner& tref(tuner == nullptr ? *tptr : *tuner);

        // Try to build a graph from this network provider and tuner
        if (tref._guts->buildGraph(tuner_monikers[dvb_device_current].pointer(), report)) {

            // Graph correctly built, we can use this tuner.
            tref._is_open = true;
            tref._info_only = true;
            tref._device_name = tuner_name;
            tref._device_info.clear();  // none on Windows

            // Add tuner it to response set
            if (tuner_list != nullptr) {
                // Add the tuner to the vector
                tuner_list->push_back(tptr);
            }
            else {
                // One single tuner requested, one found, return
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

bool ts::Tuner::Guts::buildGraph(::IMoniker* tuner_moniker, Report& report)
{
    // Report to use when errors shall be reported in debug mode only
    Report& debug_report(report.debug() ? report : NULLREP);

    // Instantiate the "Microsoft Network Provider". In the past, we tried all specific providers
    // like "Microsoft DVBT Network Provider". However, these are now deprecated and Microsoft
    // advises to use the new generic one. This provider can work with all tuners. It will accept
    // only the tuning spaces which are compatible with the connected tuner.
    // Also get a few interfaces interfaces of the network provider filter

    provider_filter.createInstance(::CLSID_NetworkProvider, ::IID_IBaseFilter, report);
    net_provider.queryInterface(provider_filter.pointer(), ::IID_IBDA_NetworkProvider, report);
    tuner.queryInterface(provider_filter.pointer(), ::IID_ITuner, report);
    if (provider_filter.isNull() || net_provider.isNull() || tuner.isNull()) {
        report.debug(u"failed to create an instance of network provider");
        return false;
    }

    // Create an instance of tuner from moniker
    tuner_filter.bindToObject(tuner_moniker, ::IID_IBaseFilter, report);
    if (tuner_filter.isNull()) {
        report.debug(u"failed to create an instance of BDA tuner");
        return false;
    }

    // Create the Filter Graph, add the filters and connect network provider to tuner.
    if (!graph.initialize(report) ||
        !graph.addFilter(provider_filter.pointer(), L"NetworkProvider", report) ||
        !graph.addFilter(tuner_filter.pointer(), L"Tuner", report) ||
        !graph.connectFilters(provider_filter.pointer(), tuner_filter.pointer(), report))
    {
        report.debug(u"failed to initiate the graph with network provider => tuner");
        return false;
    }

    // Now, the network provider is connected to the tuner. Now, we are going to try all
    // tuning spaces. Normally, the network provider will reject the tuning spaces which
    // are not compatible with the tuner.

    // Enumerate all tuning spaces in the system.
    ComPtr<::ITuningSpaceContainer> tsContainer(::CLSID_SystemTuningSpaces, ::IID_ITuningSpaceContainer, report);
    if (tsContainer.isNull()) {
        return false;
    }
    ComPtr<::IEnumTuningSpaces> tsEnum;
    ::HRESULT hr = tsContainer->get_EnumTuningSpaces(tsEnum.creator());
    if (!ComSuccess(hr, u"ITuningSpaceContainer::get_EnumTuningSpaces", report)) {
        return false;
    }

    // Loop on all tuning spaces.
    bool tspace_found = false;
    ts::ComPtr<::ITuningSpace> tspace;
    while (!tspace_found && tsEnum->Next(1, tspace.creator(), nullptr) == S_OK) {

        // Display tuning space in debug mode
        const UString fname(GetTuningSpaceFriendlyName(tspace.pointer(), report));
        const UString uname(GetTuningSpaceUniqueName(tspace.pointer(), report));
        report.debug(u"found tuning space \"%s\" (%s)", {fname, uname});

        // Try to use this tuning space with our tuner.
        hr = tuner->put_TuningSpace(tspace.pointer());
        if (ComSuccess(hr, u"fail to set tuning space \"" + fname + u"\"", debug_report)) {

            // In debug mode, get all supported network types.
            // For debug only, sometimes it does not work.
            if (report.debug()) {
                ComPtr<::ITunerCap> tuner_cap;
                tuner_cap.queryInterface(tuner.pointer(), ::IID_ITunerCap, debug_report);
                if (tuner_cap.isNull()) {
                    report.error(u"failed to get ITunerCap interface");
                    // Debug only: return false;
                }
                else {
                    std::array<::GUID,10> net_types;
                    ::ULONG net_count = ::LONG(net_types.size());
                    hr = tuner_cap->get_SupportedNetworkTypes(net_count, &net_count, net_types.data());
                    if (!ComSuccess(hr, u"ITunerCap::get_SupportedNetworkTypes", report)) {
                        // Debug only: return false;
                    }
                    else if (net_count == 0) {
                        report.error(u"tuner did not return any supported network types");
                        // Debug only: return false;
                    }
                    else {
                        report.debug(u"Supported Network Types:");
                        for (size_t n = 0; n < net_count; n++) {
                            report.debug(u"  %d) %s", {n, NameGUID(net_types[n])});
                        }
                    }
                }
            }

            // This tuning space is compatible with our tuner.
            // Check if this is a tuning space we can support by getting its DVB system type:
            // first get IDVBTuningSpace interface of tuning space (may not support it)
            ComPtr<::IDVBTuningSpace> dvb_tspace;
            dvb_tspace.queryInterface(tspace.pointer(), ::IID_IDVBTuningSpace, debug_report);
            if (!dvb_tspace.isNull()) {

                // Get DVB system type
                ::DVBSystemType systype;
                hr = dvb_tspace->get_SystemType(&systype);
                if (!ComSuccess(hr, u"cannot get DVB system type from tuning space \"" + fname + u"\"", report)) {
                    continue;
                }
                report.debug(u"DVB system type is %d for tuning space \"%s\"", { systype, fname });

                // Check if DVB system type matches our tuner type.
                switch (systype) {
                    case ::DVB_Satellite: {
                        tspace_found = true;
                        _parent->_delivery_systems.insert(DS_DVB_S);
                        _parent->_delivery_systems.insert(DS_DVB_S2);
                        // No way to check if DS_DVB_S2 is supported, assume it.
                        break;
                    }
                    case ::DVB_Terrestrial: {
                        tspace_found = true;
                        _parent->_delivery_systems.insert(DS_DVB_T);
                        _parent->_delivery_systems.insert(DS_DVB_T2);
                        // No way to check if DS_DVB_T2 is supported, assume it.
                        break;
                    }
                    case ::DVB_Cable: {
                        tspace_found = true;
                        _parent->_delivery_systems.insert(DS_DVB_C_ANNEX_A);
                        _parent->_delivery_systems.insert(DS_DVB_C_ANNEX_C);
                        // No way to check which annex is supported. Skip annex B (too special).
                        break;
                    }
                    case ::ISDB_Terrestrial:
                    case ::ISDB_Satellite:
                    default: {
                        // Not a kind of tuning space we support.
                        break;
                    }
                }
            }
            else {
                // Not a DVB tuning space, silently ignore it.
                report.debug(u"tuning space \"%s\" does not support IID_IDVBTuningSpace interface", {fname});
            }

            // Check if this is a tuning space we can support by getting its ATSC network type:
            // first get IATSCTuningSpace interface of tuning space (may not support it)
            ComPtr<::IATSCTuningSpace> atsc_tspace;
            atsc_tspace.queryInterface(tspace.pointer(), ::IID_IATSCTuningSpace, debug_report);
            if (!atsc_tspace.isNull()) {

                // Get ATSC network type
                ::GUID nettype;
                hr = atsc_tspace->get__NetworkType(&nettype);
                if (!ComSuccess(hr, u"cannot get ATSC network type from tuning space \"" + fname + u"\"", report)) {
                    continue;
                }
                report.debug(u"ATSC network type is \"%s\" for tuning space \"%s\"", { GetTuningSpaceNetworkType(tspace.pointer(), report), fname });

                // Check if ATSC network type matches our tuner type.
                if (nettype == CLSID_ATSCNetworkProvider) {
                    tspace_found = true;
                    _parent->_delivery_systems.insert(DS_ATSC);
                }
            }
            else {
                // Not an ATSC tuning space, silently ignore it.
                report.debug(u"tuning space \"%s\" does not support IID_IATSCTuningSpace interface", {fname});
            }
        }
    }

    // Give up the tuner if no tuning space was found.
    if (!tspace_found) {
        report.debug(u"no supported tuning space found for this tuner");
        return false;
    }

    // Keep this tuning space.
    tuning_space = tspace;
    tuning_space_fname = GetTuningSpaceFriendlyName(tuning_space.pointer(), report);
    tuning_space_uname = GetTuningSpaceUniqueName(tuning_space.pointer(), report);
    report.debug(u"using tuning space \"%s\" (\"%s\")", {tuning_space_uname, tuning_space_fname});

    // Try to build the rest of the graph starting at tuner filter.
    // Usually work with Terratec driver for instance.
    report.debug(u"trying direct connection from tuner (no receiver)");
    bool graph_done = buildCaptureGraph(tuner_filter, report);

    // If the tuner cannot be directly connected to the rest of the
    // graph, we need to find a specific "receiver" filter (usually
    // provided by the same vendor as the tuner filter). Needed by
    // Hauppauge or Pinnacle drivers for instance.
    if (!graph_done) {

        // Enumerate all filters with category KSCATEGORY_BDA_RECEIVER_COMPONENT
        std::vector <ComPtr<::IMoniker>> receiver_monikers;
        if (!EnumerateDevicesByClass(KSCATEGORY_BDA_RECEIVER_COMPONENT, receiver_monikers, report)) {
            return false;
        }

        // Loop on all enumerated receiver filters
        for (size_t receiver_index = 0; !graph_done && receiver_index < receiver_monikers.size(); ++receiver_index) {

            // Get friendly name of this network provider
            UString receiver_name(GetStringPropertyBag(receiver_monikers[receiver_index].pointer(), L"FriendlyName", debug_report));
            report.debug(u"trying receiver filter \"%s\"", {receiver_name});

            // Create an instance of this receiver filter from moniker
            ComPtr<::IBaseFilter> receiver_filter;
            receiver_filter.bindToObject(receiver_monikers[receiver_index].pointer(), ::IID_IBaseFilter, debug_report);
            if (receiver_filter.isNull()) {
                continue; // give up this receiver filter
            }

            // Add the filter in the graph
            if (!graph.addFilter(receiver_filter.pointer(), L"Receiver", report)) {
                continue; // give up this receiver filter
            }

            // Try to connect the tuner to the receiver
            if (!graph.connectFilters(tuner_filter.pointer(), receiver_filter.pointer(), debug_report)) {
                // This receiver is not compatible, remove it from the graph
                graph.removeFilter(receiver_filter.pointer(), debug_report);
                continue;
            }

            // Try to build the rest of the graph
            if (buildCaptureGraph(receiver_filter, report)) {
                graph_done = true;
                report.debug(u"using receiver filter \"%s\"", {receiver_name});
            }
        }
    }
    if (!graph_done) {
        return false;
    }

    // Locate all instances of some interfaces in the tuner topology
    demods.clear();
    demods2.clear();
    sigstats.clear();
    tunprops.clear();

    // Lookup all internal nodes in the BDA topology
    ComPtr<::IBDA_Topology> topology;
    topology.queryInterface(tuner_filter.pointer(), ::IID_IBDA_Topology, NULLREP);
    if (!topology.isNull()) {
        // Get node types
        static const ::ULONG MAX_NODES = 64;
        ::ULONG count = MAX_NODES;
        ::ULONG types[MAX_NODES];
        if (SUCCEEDED(topology->GetNodeTypes(&count, MAX_NODES, types))) {
            // Enumerate all node types
            for (::ULONG n = 0; n < count; ++n) {
                // Get control node for this type
                ComPtr<::IUnknown> cnode;
                if (SUCCEEDED(topology->GetControlNode(0, 1, types[n], cnode.creator()))) {
                    findTunerSubinterfaces(cnode);
                }
            }
        }
    }

    // Look at all connected pins
    ComPtr<::IEnumPins> enum_pins;
    if (SUCCEEDED(tuner_filter->EnumPins(enum_pins.creator()))) {
        // Enumerate all pins in tuner filter
        ComPtr<::IPin> pin;
        while (enum_pins->Next(1, pin.creator(), NULL) == S_OK) {
            // Check if this pin is connected
            ComPtr<::IPin> partner;
            if (SUCCEEDED(pin->ConnectedTo(partner.creator()))) {
                findTunerSubinterfaces(pin);
            }
        }
    }

    report.debug(u"IBDA_DigitalDemodulator in tuner: %d", {demods.size()});
    report.debug(u"IBDA_DigitalDemodulator2 in tuner: %d", {demods2.size()});
    report.debug(u"IBDA_SignalStatistics in tuner: %d", {sigstats.size()});
    report.debug(u"IKsPropertySet in tuner: %d", {tunprops.size()});

    return true;
}


//-----------------------------------------------------------------------------
// Try to build the part of the graph starting at the tee filter.
// The specified base filter is either the tuner filter or some
// other intermediate receiver filter downstream the tuner.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::Tuner::Guts::buildCaptureGraph(const ComPtr<::IBaseFilter>& base_filter, Report& report)
{
    // Report to use when errors shall be reported in debug mode only
    Report& debug_report(report.debug() ? report : NULLREP);

    // Create a DirectShow System Device Enumerator
    ComPtr<::ICreateDevEnum> enum_devices(::CLSID_SystemDeviceEnum, ::IID_ICreateDevEnum, report);
    if (enum_devices.isNull()) {
        return false;
    }

    // Create an "infinite tee filter"
    ComPtr<::IBaseFilter> tee_filter(CLSID_InfTee, IID_IBaseFilter, report);
    if (tee_filter.isNull()) {
        return false;
    }

    // Add the tee filter to the graph
    if (!graph.addFilter(tee_filter.pointer(), L"Tee", report)) {
        return false;
    }

    // After this point, we cannot simply return false on error since
    // the graph needs some cleanup.

    // Try to connect the "base" filter (tuner or receiver) to the tee filter.
    bool ok = graph.connectFilters(base_filter.pointer(), tee_filter.pointer(), debug_report);

    // Create branch A of graph: Create a sink filter, add it to the graph and connect it to the tee.
    ComPtr<SinkFilter> sink(new SinkFilter(report));
    CheckNonNull(sink.pointer());
    ok = ok &&
        graph.addFilter(sink.pointer(), L"Sink/Capture", report) &&
        graph.connectFilters(tee_filter.pointer(), sink.pointer(), debug_report);

    // Create branch B of graph: Create an MPEG-2 demultiplexer
    ComPtr<::IBaseFilter> demux_filter(::CLSID_MPEG2Demultiplexer, ::IID_IBaseFilter, report);
    ok = ok &&
        !demux_filter.isNull() &&
        graph.addFilter(demux_filter.pointer(), L"Demux", report) &&
        graph.connectFilters(tee_filter.pointer(), demux_filter.pointer(), debug_report);

    // Now, we need to connect a Transport Information Filter (TIF).
    // There is no predefined CLSID for this one and we must loop on all
    // filters with category KSCATEGORY_BDA_TRANSPORT_INFORMATION
    ComPtr<::IEnumMoniker> enum_tif;
    if (ok) {
        ::HRESULT hr = enum_devices->CreateClassEnumerator(KSCATEGORY_BDA_TRANSPORT_INFORMATION, enum_tif.creator(), 0);
        ok = ComSuccess(hr, u"CreateClassEnumerator (for TIF)", report) && hr == S_OK;
    }

    // Loop on all enumerated TIF
    ComPtr<::IMoniker> tif_moniker;
    bool tif_found = false;
    while (ok && !tif_found && enum_tif->Next(1, tif_moniker.creator(), NULL) == S_OK) {

        // Get friendly name of this TIF
        UString tif_name(GetStringPropertyBag(tif_moniker.pointer(), L"FriendlyName", report));
        report.debug(u"trying TIF \"%s\"", {tif_name});

        // Create an instance of this TIF from moniker
        ComPtr<::IBaseFilter> tif_filter;
        tif_filter.bindToObject(tif_moniker.pointer(), ::IID_IBaseFilter, report);
        if (tif_filter.isNull()) {
            continue; // give up this TIF
        }

        // Add the TIF in the graph
        if (!graph.addFilter(tif_filter.pointer(), L"TIF", report)) {
            continue; // give up this TIF
        }

        // Try to connect demux filter to tif
        if (graph.connectFilters(demux_filter.pointer(), tif_filter.pointer(), debug_report)) {
            tif_found = true;
            report.debug(u"using TIF \"%s\"", {tif_name});
        }
        else {
            // This tif is not compatible, remove it from the graph
            graph.removeFilter(tif_filter.pointer(), report);
        }
    }

    // If successful so far, done
    if (tif_found) {
        sink_filter = sink;
        return true;
    }

    // Not successful, cleanup everything.
    // Cleanup the graph downstream the tuner filter.
    // This will also remove any optional receiver filter
    // between the tuner and the tee.
    graph.cleanupDownstream(tuner_filter.pointer(), debug_report);

    // Remove all created filters from the graph. Ignore errors.
    // This is necessary if a filter was created and added to the graph but
    // not connected (if connected, it was removed by CleanupDownstream).
    if (!tee_filter.isNull()) {
        graph.removeFilter(tee_filter.pointer(), report);
    }
    if (!sink.isNull()) {
        graph.removeFilter(sink.pointer(), report);
    }
    if (!demux_filter.isNull()) {
        graph.removeFilter(demux_filter.pointer(), report);
    }

    return false;
}
