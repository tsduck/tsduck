//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  Windows implementation of the ts::TunerDevice class.
//
//-----------------------------------------------------------------------------

#include "tsTunerDevice.h"
#include "tsSignalState.h"
#include "tsTSPacket.h"
#include "tsTime.h"
#include "tsNullReport.h"
#include "tsSysUtils.h"
#include "tsWinUtils.h"
#include "tsComPtr.h"
#include "tsTunerGraph.h"
#include "tsDirectShowUtils.h"
#include "tsSinkFilter.h"
#include "tsTS.h"


//-----------------------------------------------------------------------------
// Constructor and destructor.
//-----------------------------------------------------------------------------

ts::TunerDevice::TunerDevice(DuckContext& duck) :
    TunerBase(duck),
    _is_open(false),
    _info_only(false),
    _device_name(),
    _device_info(),
    _device_path(),
    _signal_timeout(DEFAULT_SIGNAL_TIMEOUT),
    _signal_timeout_silent(false),
    _receive_timeout(0),
    _delivery_systems(),
    _aborted(false),
    _sink_queue_size(DEFAULT_SINK_QUEUE_SIZE),
    _graph()
{
}

ts::TunerDevice::~TunerDevice()
{
}


//-----------------------------------------------------------------------------
// Linux implementation of services from ts::TunerBase.
// Get the list of all existing DVB tuners.
//-----------------------------------------------------------------------------

bool ts::TunerBase::GetAllTuners(DuckContext& duck, TunerPtrVector& tuners)
{
    return TunerDevice::FindTuners(duck, nullptr, &tuners);
}


//-----------------------------------------------------------------------------
// Get/set basic parameters.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::isOpen() const
{
    return _is_open;
}

bool ts::TunerDevice::infoOnly() const
{
    return _info_only;
}

const ts::DeliverySystemSet& ts::TunerDevice::deliverySystems() const
{
    return _delivery_systems;
}

ts::UString ts::TunerDevice::deviceName() const
{
    return _device_name;
}

ts::UString ts::TunerDevice::deviceInfo() const
{
    return _device_info;
}

ts::UString ts::TunerDevice::devicePath() const
{
    return _device_path;
}

ts::MilliSecond ts::TunerDevice::receiveTimeout() const
{
    return _receive_timeout;
}

void ts::TunerDevice::setSignalTimeout(MilliSecond t)
{
    _signal_timeout = t;
}

void ts::TunerDevice::setSignalTimeoutSilent(bool silent)
{
    _signal_timeout_silent = silent;
}

void ts::TunerDevice::setSinkQueueSize(size_t s)
{
    _sink_queue_size = s;
}

void ts::TunerDevice::setReceiverFilterName(const UString& name)
{
    _graph.setReceiverName(name);
}


//-----------------------------------------------------------------------------
// Open the tuner.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::open(const UString& device_name, bool info_only)
{
    if (_is_open) {
        _duck.report().error(u"tuner already open");
        return false;
    }
    _device_name = device_name;
    if (!FindTuners(_duck, this, nullptr)) {
        return false;
    }
    else if (_is_open) {
        _info_only = info_only;
        return true;
    }
    else if (device_name.empty()) {
        _duck.report().error(u"No tuner device");
        return false;
    }
    else {
        _duck.report().error(u"device \"%s\" not found", {device_name});
        return false;
    }
}


//-----------------------------------------------------------------------------
// Close tuner.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::close(bool silent)
{
    _is_open = false;
    _device_name.clear();
    _device_info.clear();
    _device_path.clear();
    _delivery_systems.clear();
    _graph.clear(silent ? NULLREP : _duck.report());
    return true;
}


//-----------------------------------------------------------------------------
// Get the signal state.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::getSignalState(SignalState& state)
{
    state.clear();

    if (!_is_open) {
        _duck.report().error(u"tuner not open");
        return false;
    }

    // Get the signal locked indicator.
    bool ok = false;
    ::BOOL locked = 0;
    ok = _graph.searchProperty(locked, TunerGraph::psHIGHEST,
                               &::IBDA_SignalStatistics::get_SignalLocked,
                               KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_LOCKED);
    state.signal_locked = ok && locked != 0;

    // The header bdamedia.h defines carrier strength in mdB (1/1000 of a dB).
    // A strength of 0 is nominal strength as expected for the given network.
    // Sub-nominal strengths are reported as positive mdB
    // Super-nominal strengths are reported as negative mdB
    ::LONG strength = 0;
    ok = _graph.searchProperty(strength, TunerGraph::psHIGHEST,
                               &::IBDA_SignalStatistics::get_SignalStrength,
                               KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_STRENGTH);
    if (ok) {
        state.signal_strength = SignalState::Value(strength, SignalState::Unit::MDB);
    }

    // Other signal state parameters are not available on Windows.
    return true;
}


//-----------------------------------------------------------------------------
// Get the current tuning parameters
//-----------------------------------------------------------------------------

bool ts::TunerDevice::getCurrentTuning(ModulationArgs& params, bool reset_unknown)
{
    if (!_is_open) {
        _duck.report().error(u"tuner not open");
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

        case TT_DVB_S:
        case TT_ISDB_S: {
            // Note: it is useless to get the frequency of a DVB-S tuner since it
            // returns the intermediate frequency and there is no unique satellite
            // frequency for a given intermediate frequency.
            if (reset_unknown) {
                params.frequency.clear();
                params.satellite_number.clear();
                params.lnb.clear();
                params.polarity.clear();
            }
            // Spectral inversion
            _graph.searchVarProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // Symbol rate
            _graph.searchVarProperty<::ULONG>(
                0, params.symbol_rate, TunerGraph::psHIGHEST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SymbolRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SYMBOL_RATE);
            // Inner FEC
            _graph.searchVarProperty<::BinaryConvolutionCodeRate>(
                ::BDA_BCC_RATE_NOT_SET, params.inner_fec, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_InnerFECRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE);
            // Modulation
            _graph.searchVarProperty<::ModulationType>(
                ::BDA_MOD_NOT_SET, params.modulation, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_ModulationType,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE);
            // Delivery system. Found no way to get DVB-S vs. DVB-S2 on Windows.
            // Make a not quite correct assumption, based on modulation type.
            if (params.modulation.set()) {
                params.delivery_system = params.modulation == QPSK ? DS_DVB_S : DS_DVB_S2;
            }
            else if (reset_unknown) {
                params.delivery_system.clear();
            }
            // DVB-S2 pilot
            _graph.searchVarProperty<::Pilot>(
                ::BDA_PILOT_NOT_SET, params.pilots, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator2::get_Pilot,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_PILOT);
            // DVB-S2 roll-off factor
            _graph.searchVarProperty<::RollOff>(
                ::BDA_ROLL_OFF_NOT_SET, params.roll_off, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator2::get_RollOff,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_ROLL_OFF);
            break;
        }

        case TT_DVB_C:
        case TT_ISDB_C: {
            if (reset_unknown) {
                params.frequency.clear();
            }
            // Spectral inversion
            _graph.searchVarProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // Symbol rate
            _graph.searchVarProperty<::ULONG>(
                0, params.symbol_rate, TunerGraph::psHIGHEST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SymbolRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SYMBOL_RATE);
            // Inner FEC
            _graph.searchVarProperty<::BinaryConvolutionCodeRate>(
                ::BDA_BCC_RATE_NOT_SET, params.inner_fec, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_InnerFECRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE);
            // Modulation
            _graph.searchVarProperty<::ModulationType>(
                ::BDA_MOD_NOT_SET, params.modulation, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_ModulationType,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE);
            break;
        }

        case TT_DVB_T:
        case TT_ISDB_T: {
            if (reset_unknown) {
                params.frequency.clear();
            }
            // Spectral inversion
            _graph.searchVarProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // High priority FEC
            _graph.searchVarProperty<::BinaryConvolutionCodeRate>(
                ::BDA_BCC_RATE_NOT_SET, params.fec_hp, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_InnerFECRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE);
            // Modulation
            _graph.searchVarProperty<::ModulationType>(
                ::BDA_MOD_NOT_SET, params.modulation, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_ModulationType,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE);
            // Other DVB-T parameters, not supported in IBDA_DigitalDemodulator
            // but which may be supported as properties.
            ::TransmissionMode tm = ::BDA_XMIT_MODE_NOT_SET;
            found = _graph.searchTunerProperty(tm, TunerGraph::psFIRST, KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_TRANSMISSION_MODE);
            if (found && tm != ::BDA_XMIT_MODE_NOT_SET) {
                params.transmission_mode = ts::TransmissionMode(tm);
            }
            else if (reset_unknown) {
                params.transmission_mode.clear();
            }
            ::GuardInterval gi = ::BDA_GUARD_NOT_SET;
            found = _graph.searchTunerProperty(gi, TunerGraph::psFIRST, KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_GUARD_INTERVAL);
            if (found && gi != ::BDA_GUARD_NOT_SET) {
                params.guard_interval = ts::GuardInterval(gi);
            }
            else if (reset_unknown) {
                params.guard_interval.clear();
            }
            // Other DVB-T parameters, not supported at all
            params.bandwidth.clear();
            params.hierarchy.clear();
            params.fec_lp.clear();
            params.plp.clear();
            break;
        }

        case TT_ATSC: {
            if (reset_unknown) {
                params.frequency.clear();
            }
            // Spectral inversion
            _graph.searchVarProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // Modulation
            _graph.searchVarProperty<::ModulationType>(
                ::BDA_MOD_NOT_SET, params.modulation, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_ModulationType,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE);
            break;
        }

        case TT_UNDEFINED:
        default: {
            _duck.report().error(u"cannot convert BDA tuning parameters to %s parameters", {TunerTypeEnum.name(ttype)});
            return false;
        }
    }

    // Some drivers sometimes return weird values for spectral inversion.
    // Reset it in case of invalid value.
    if (params.inversion.set() && params.inversion.value() != SPINV_AUTO && params.inversion.value() != SPINV_ON && params.inversion.value() != SPINV_OFF) {
        params.inversion.clear();
    }

    return true;
}


//-----------------------------------------------------------------------------
// Tune to the specified parameters and start receiving.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::tune(ModulationArgs& params)
{
    return checkTuneParameters(params) && _graph.sendTuneRequest(_duck, params);
}


//-----------------------------------------------------------------------------
// Start receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::TunerDevice::start()
{
    SinkFilter* const sink = _graph.sinkFilter();

    if (!_is_open || sink == nullptr) {
        _duck.report().error(u"tuner not open");
        return false;
    }

    // Cannot restart if aborted.
    if (_aborted) {
        return false;
    }

    // Set media samples queue size.
    sink->SetMaxMessages(_sink_queue_size);

    // Run the graph.
    if (!_graph.run(_duck.report())) {
        return false;
    }

    // Check if aborted while starting.
    if (_aborted) {
        return false;
    }

    // If the tuner was previously started/stopped on a frequency with signal on it,
    // it has been observed that remaining packets from the previous run were still
    // there. Wait a little bit and reflush after Run() to avoid that.
    // Yes, this is a horrible hack, but if you have a better fix...
    SleepThread(50); // milliseconds
    sink->Flush();

    // If a signal timeout was specified, read a packet with timeout
    if (_signal_timeout > 0) {
        TSPacket pack;
        if (sink->Read(&pack, sizeof(pack), _signal_timeout) == 0) {
            if (!_signal_timeout_silent) {
                _duck.report().error(u"no input DVB signal after %'d milliseconds", {_signal_timeout});
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

bool ts::TunerDevice::stop(bool silent)
{
    return _is_open && _graph.stop(silent ? NULLREP : _duck.report());
}


//-----------------------------------------------------------------------------
// Timeout for receive operation (none by default).
// If zero, no timeout is applied.
// Return true on success, false on errors.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::setReceiveTimeout(MilliSecond timeout)
{
    _receive_timeout = timeout;
    return true;
}


//-----------------------------------------------------------------------------
// Abort any pending or blocked reception.
//-----------------------------------------------------------------------------

void ts::TunerDevice::abort(bool silent)
{
    SinkFilter* const sink = _graph.sinkFilter();
    if (_is_open && sink != nullptr) {
        _aborted = true;
        sink->Abort();
    }
}


//-----------------------------------------------------------------------------
// Read complete 188-byte TS packets in the buffer and return the
// number of actually received packets (in the range 1 to max_packets).
// Returning zero means error or end of input.
//-----------------------------------------------------------------------------

size_t ts::TunerDevice::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort)
{
    SinkFilter* const sink = _graph.sinkFilter();

    if (!_is_open || sink == nullptr) {
        _duck.report().error(u"tuner not open");
        return 0;
    }
    if (_aborted) {
        return 0;
    }

    // Read packets from the tuner device
    size_t got_size = 0;
    if (_receive_timeout <= 0) {
        got_size = sink->Read(buffer, max_packets * PKT_SIZE);
    }
    else {
        const Time limit(Time::CurrentUTC() + _receive_timeout);
        got_size = sink->Read(buffer, max_packets * PKT_SIZE, _receive_timeout);
        if (got_size == 0 && Time::CurrentUTC() >= limit) {
            _duck.report().error(u"receive timeout on " + _device_name);
        }
    }

    return got_size / PKT_SIZE;
}


//-----------------------------------------------------------------------------
// Display the characteristics and status of the tuner.
//-----------------------------------------------------------------------------

std::ostream& ts::TunerDevice::displayStatus(std::ostream& strm, const UString& margin, bool extended)
{
    if (!_is_open) {
        _duck.report().error(u"tuner not open");
        return strm;
    }

    SignalState state;
    getSignalState(state);
    strm << margin << "Signal locked:    " << UString::YesNo(state.signal_locked) << std::endl;
    if (state.signal_strength.set()) {
        strm << margin << "Signal strength:  " << state.signal_strength.value() << std::endl;
    }

    // The DirectShow graph can be very verbose.
    if (extended) {
        strm << std::endl << margin << "DirectShow graph:" << std::endl;
        _graph.display(strm, _duck.report(), margin + u"  ", true);
    }

    return strm;
}


//-----------------------------------------------------------------------------
// Private static method: Find one or more tuners.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::FindTuners(DuckContext& duck, TunerDevice* tuner, TunerPtrVector* tuner_list)
{
    // Report to use when errors shall be reported in debug mode only
    Report& report(duck.report());
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
    if (tuner != nullptr) {
        report.debug(u"looking for DVB adapter number \"%d\"", {tuner->_device_name});
        if (!tuner->_device_name.empty() && tuner->_device_name[0] == ':') {
            tuner->_device_name.substr(1).toInteger(dvb_device_index);
        }
    }

    // Get all tuner filters.
    std::vector<ComPtr<::IMoniker>> tuner_monikers;

    // First, check if tuner device name is a device path in order to get a direct moniker.
    if (tuner != nullptr && tuner->_device_name.startWith(u"@")) {
        report.debug(u"looking for DVB device path \"%s\"", {tuner->_device_name});
        IBindCtx* lpBC = nullptr;
        IMoniker* pmTuner = nullptr;
        HRESULT hr = ::CreateBindCtx(0, &lpBC);
        if (SUCCEEDED(hr)) {
            DWORD dwEaten = 0;
            hr = ::MkParseDisplayName(lpBC, tuner->deviceName().wc_str(), &dwEaten, &pmTuner);
            if (hr == S_OK) {
                tuner_monikers.emplace_back(pmTuner);
            }
            lpBC->Release();
        }
    }

    // If not directly found, enumerate all filters with category KSCATEGORY_BDA_NETWORK_TUNER.
    // These filters are usually installed by vendors of hardware tuners when they provide BDA-compatible drivers.
    if (tuner_monikers.empty() && !EnumerateDevicesByClass(KSCATEGORY_BDA_NETWORK_TUNER, tuner_monikers, report)) {
        return false;
    }

    // Loop on all enumerated tuners.
    // We need to keep two separate indexes. 'moniker_index' is a true index in 'tuner_monikers'.
    // But this is not the same thing as option --adapter because not all filters from 'tuner_monikers'
    // are valid tuners. Some of them can be skipped. So, we keep a counter named 'dvb_device_current'
    // which counts actually useable tuners. This index is synchronous with --adapter.
    int dvb_device_current = 0;
    for (size_t moniker_index = 0; moniker_index < tuner_monikers.size(); ++moniker_index) {

        // Get friendly name of this tuner filter
        const UString tuner_name(GetStringPropertyBag(tuner_monikers[moniker_index].pointer(), L"FriendlyName", debug_report));
        report.debug(u"found tuner filter \"%s\"", {tuner_name});

        // Get physical device path.
        UString device_path;
        ::WCHAR* wstring = nullptr;
        ::HRESULT hr = tuner_monikers[moniker_index]->GetDisplayName(0, 0, &wstring);
        if (ComSuccess(hr, u"IMoniker::GetDisplayName", report)) {
            device_path = ToString(wstring);
            ::CoTaskMemFree(wstring);
        }
        report.debug(u"tuner device path: %s", {device_path});

        // If a device name was specified, filter this name.
        // First case: a tuner filter name was specified. In that case, there is
        // no need to test other filters, simply skip them. Since the filter names
        // are long and complicated, ignore case and blanks, use UString::similar().
        if (tuner != nullptr &&
            !tuner->_device_name.empty() &&
            dvb_device_index < 0 &&
            !tuner_name.similar(tuner->_device_name) &&
            (device_path.empty() || !device_path.similar(tuner->_device_name)))
        {
            // Device specified by name, but not this one, try next tuner
            continue;
        }

        // If we search one specific tuner (tuner != nullptr), use this one.
        // If we are building a list of all tuners (tuner_list != nullptr), allocate a new tuner.
        TunerDevice* new_tuner = tuner == nullptr ? new TunerDevice(duck) : nullptr;
        TunerPtr tptr(new_tuner);
        TunerDevice& tref(tuner == nullptr ? *new_tuner : *tuner);

        // Try to build a graph from this network provider and tuner
        if (tref._graph.initialize(tuner_name, tuner_monikers[moniker_index].pointer(), tref._delivery_systems, report)) {

            // Graph correctly built, this is a valid tuner.
            // Check if a device was specified by adapter index.
            if (dvb_device_index >= 0 && dvb_device_current != dvb_device_index) {
                // Adapter index was specified, but not this one.
                tref._graph.clear(debug_report);
                tref._delivery_systems.clear();
            }
            else {
                // Either no adapter index was specified or this is the right one.
                // Use that tuner.
                tref._is_open = true;
                tref._aborted = false;
                tref._info_only = true;
                tref._device_name = tuner_name;
                tref._device_path = device_path;
                tref._device_info.clear();  // none on Windows
                report.debug(u"found tuner device \"%s\"", {tref._device_name});

                // Add tuner it to response set
                if (tuner_list != nullptr) {
                    // Build a list of all tuners, add this one to the vector
                    tuner_list->push_back(tptr);
                }
                else {
                    // One single tuner requested, one found, return
                    return true;
                }
            }

            // Count valid devices.
            dvb_device_current++;
        }
    }
    return true;
}
