//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
#include "tsTunerGraph.h"
#include "tsDirectShowUtils.h"
#include "tsSinkFilter.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr size_t ts::Tuner::DEFAULT_SINK_QUEUE_SIZE;
#endif


//-----------------------------------------------------------------------------
// Windows version of the syste guts class.
//-----------------------------------------------------------------------------

class ts::Tuner::Guts
{
    TS_NOCOPY(Guts);
public:
    size_t     sink_queue_size;   // Media sample queue size
    TunerGraph graph;             // The filter graph

    // Constructor and destructor.
    Guts();
    ~Guts();

    // Get signal strength in mdB.
    // Return true if found, false if not found.
    bool getSignalStrength_mdB(::LONG&);

    // Find one or more tuners. Exactly one of Tuner* or TunerPtrVector* must be non-zero.
    // If Tuner* is non-zero, find the first tuner (matching _device_name if not empty).
    // If _device_name is ":integer", use integer as device index in list of DVB devices.
    // If TunerPtrVector* is non- zero, find all tuners in the system.
    // Return true on success, false on error.
    static bool FindTuners(DuckContext& duck, Tuner*, TunerPtrVector*, Report&);
};


//-----------------------------------------------------------------------------
// System guts constructor and destructor.
//-----------------------------------------------------------------------------

ts::Tuner::Guts::Guts() :
    sink_queue_size(DEFAULT_SINK_QUEUE_SIZE),
    graph()
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
    _guts = new Guts();
}

void ts::Tuner::deleteGuts()
{
    delete _guts;
}


//-----------------------------------------------------------------------------
// Set the Windows-specific parameters.
//-----------------------------------------------------------------------------

void ts::Tuner::setSinkQueueSize(size_t s)
{
    _guts->sink_queue_size = s;
}

void ts::Tuner::setReceiverFilterName(const UString& name)
{
    _guts->graph.setReceiverName(name);
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
    return true;
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
    const bool found = _guts->graph.searchProperty(locked, TunerGraph::psHIGHEST,
                                                   &::IBDA_SignalStatistics::get_SignalLocked,
                                                   KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_LOCKED);
    return found && locked != 0;
}


//-----------------------------------------------------------------------------
// Get signal strength in mdB.
//-----------------------------------------------------------------------------

bool ts::Tuner::Guts::getSignalStrength_mdB(::LONG& strength)
{
    // The header bdamedia.h defines carrier strength in mdB (1/1000 of a dB).
    // A strength of 0 is nominal strength as expected for the given network.
    // Sub-nominal strengths are reported as positive mdB
    // Super-nominal strengths are reported as negative mdB

    return graph.searchProperty(strength, TunerGraph::psHIGHEST,
                                &::IBDA_SignalStatistics::get_SignalStrength,
                                KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_STRENGTH);
}


//-----------------------------------------------------------------------------
// Return signal strength, in percent (0=bad, 100=good)
//-----------------------------------------------------------------------------

int ts::Tuner::signalStrength(Report& report)
{
    if (!_is_open) {
        report.error(u"tuner not open");
        return false;
    }
    else {
        ::LONG strength = 0;
        return _guts->getSignalStrength_mdB(strength) ? std::max(0, 100 + int(strength) / 1000) : -1;
    }
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
    const bool found = _guts->graph.searchProperty(quality, TunerGraph::psHIGHEST,
                                                   &::IBDA_SignalStatistics::get_SignalQuality,
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

        case TT_DVB_S:
        case TT_ISDB_S: {
            // Note: it is useless to get the frequency of a DVB-S tuner since it
            // returns the intermediate frequency and there is no unique satellite
            // frequency for a given intermediate frequency.
            if (reset_unknown) {
                params.frequency.clear();
                params.satellite_number.clear();
                params.lnb.clear();
            }
            // Spectral inversion
            _guts->graph.searchVarProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // Symbol rate
            _guts->graph.searchVarProperty<::ULONG>(
                0, params.symbol_rate, TunerGraph::psHIGHEST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SymbolRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SYMBOL_RATE);
            // Inner FEC
            _guts->graph.searchVarProperty<::BinaryConvolutionCodeRate>(
                ::BDA_BCC_RATE_NOT_SET, params.inner_fec, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_InnerFECRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE);
            // Modulation
            _guts->graph.searchVarProperty<::ModulationType>(
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
            _guts->graph.searchVarProperty<::Pilot>(
                ::BDA_PILOT_NOT_SET, params.pilots, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator2::get_Pilot,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_PILOT);
            // DVB-S2 roll-off factor
            _guts->graph.searchVarProperty<::RollOff>(
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
            _guts->graph.searchVarProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // Symbol rate
            _guts->graph.searchVarProperty<::ULONG>(
                0, params.symbol_rate, TunerGraph::psHIGHEST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SymbolRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SYMBOL_RATE);
            // Inner FEC
            _guts->graph.searchVarProperty<::BinaryConvolutionCodeRate>(
                ::BDA_BCC_RATE_NOT_SET, params.inner_fec, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_InnerFECRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE);
            // Modulation
            _guts->graph.searchVarProperty<::ModulationType>(
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
            _guts->graph.searchVarProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // High priority FEC
            _guts->graph.searchVarProperty<::BinaryConvolutionCodeRate>(
                ::BDA_BCC_RATE_NOT_SET, params.fec_hp, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_InnerFECRate,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE);
            // Modulation
            _guts->graph.searchVarProperty<::ModulationType>(
                ::BDA_MOD_NOT_SET, params.modulation, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_ModulationType,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE);
            // Other DVB-T parameters, not supported in IBDA_DigitalDemodulator
            // but which may be supported as properties.
            ::TransmissionMode tm = ::BDA_XMIT_MODE_NOT_SET;
            found = _guts->graph.searchTunerProperty(tm, TunerGraph::psFIRST, KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_TRANSMISSION_MODE);
            if (found && tm != ::BDA_XMIT_MODE_NOT_SET) {
                params.transmission_mode = ts::TransmissionMode(tm);
            }
            else if (reset_unknown) {
                params.transmission_mode.clear();
            }
            ::GuardInterval gi = ::BDA_GUARD_NOT_SET;
            found = _guts->graph.searchTunerProperty(gi, TunerGraph::psFIRST, KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_GUARD_INTERVAL);
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
            _guts->graph.searchVarProperty<::SpectralInversion>(
                ::BDA_SPECTRAL_INVERSION_NOT_SET, params.inversion, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_SpectralInversion,
                KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION);
            // Modulation
            _guts->graph.searchVarProperty<::ModulationType>(
                ::BDA_MOD_NOT_SET, params.modulation, TunerGraph::psFIRST, reset_unknown,
                &::IBDA_DigitalDemodulator::get_ModulationType,
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
//-----------------------------------------------------------------------------

bool ts::Tuner::tune(ModulationArgs& params, Report& report)
{
    return checkTuneParameters(params, report) && _guts->graph.sendTuneRequest(_duck, params, report);
}


//-----------------------------------------------------------------------------
// Start receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::start(Report& report)
{
    SinkFilter* const sink = _guts->graph.sinkFilter();

    if (!_is_open || sink == nullptr) {
        report.error(u"tuner not open");
        return 0;
    }

    // Set media samples queue size.
    sink->SetMaxMessages(_guts->sink_queue_size);

    // Run the graph.
    if (!_guts->graph.run(report)) {
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
    SinkFilter* const sink = _guts->graph.sinkFilter();

    if (!_is_open || sink == nullptr) {
        report.error(u"tuner not open");
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
    if (tuner != nullptr) {
        report.debug(u"looking for DVB adapter number \"%d\"", {tuner->_device_name});
        if (!tuner->_device_name.empty() && tuner->_device_name[0] == ':') {
            tuner->_device_name.substr(1).toInteger(dvb_device_index);
        }
    }

    // Enumerate all filters with category KSCATEGORY_BDA_NETWORK_TUNER.
    // These filters are usually installed by vendors of hardware tuners
    // when they provide BDA-compatible drivers.
    std::vector<ComPtr<::IMoniker>> tuner_monikers;
    if (!EnumerateDevicesByClass(KSCATEGORY_BDA_NETWORK_TUNER, tuner_monikers, report)) {
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
        TunerPtr tptr(tuner == nullptr ? new Tuner(duck) : 0);
        Tuner& tref(tuner == nullptr ? *tptr : *tuner);

        // Try to build a graph from this network provider and tuner
        if (tref._guts->graph.initialize(tuner_name, tuner_monikers[moniker_index].pointer(), tref._delivery_systems, report)) {

            // Graph correctly built, this is a valid tuner.
            // Check if a device was specified by adapter index.
            if (dvb_device_index >= 0 && dvb_device_current != dvb_device_index) {
                // Adapter index was specified, but not this one.
                tref._guts->graph.clear(debug_report);
                tref._delivery_systems.clear();
            }
            else {
                // Either no adapter index was specified or this is the right one.
                // Use that tuner.
                tref._is_open = true;
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
