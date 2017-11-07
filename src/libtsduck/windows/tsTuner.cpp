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
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const ts::MilliSecond ts::Tuner::DEFAULT_SIGNAL_TIMEOUT;
const size_t ts::Tuner::DEFAULT_SINK_QUEUE_SIZE;
#endif


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

ts::Tuner::Tuner(const std::string& device_name) :
    _is_open(false),
    _info_only(true),
    _tuner_type(DVB_T),
    _device_name(device_name),
    _device_info(),
    _signal_timeout(DEFAULT_SIGNAL_TIMEOUT),
    _signal_timeout_silent(false),
    _receive_timeout(0),
    _delivery_systems(),
    _sink_queue_size(DEFAULT_SINK_QUEUE_SIZE),
    _graph(),
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
    Tuner(device_name)
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
    _graph.clear(report);
    _sink_filter.release();
    _provider_filter.release();
    _provider_name.clear();
    _net_provider.release();
    _tuner.release();
    _tuning_space.release();
    _tuning_space_fname.clear();
    _tuning_space_uname.clear();
    _tuner_filter.release();
    _demods.clear();
    _demods2.clear();
    _sigstats.clear();
    _tunprops.clear();
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
            if (SUCCEEDED(ivector[i]->get (&val))) {                       \
                SELECTPROP(terminated, found, retvalue, val, searchtype);  \
            }                                                              \
        }                                                                  \
        for (size_t i = 0; !terminated && i < _tunprops.size(); ++i) {     \
            pvaltype val;                                                  \
            ::DWORD retsize = sizeof(val);                                 \
            if (SUCCEEDED(_tunprops[i]->Get(propset, propid, NULL, 0, &val, retsize, &retsize))) { \
                SELECTPROP(terminated, found, retvalue, val, searchtype);  \
            }                                                              \
        }                                                                  \
    }


//-----------------------------------------------------------------------------
// Search all IKsPropertySet in the tuner until the specified data is found.
// Return true if found, false if not found.
//-----------------------------------------------------------------------------

template <typename T>
bool ts::Tuner::searchTunerProperty(const ::GUID& propset, ::DWORD propid, T& value, PropSearch ps)
{
    bool found = false;
    bool terminated = false;

    for (size_t i = 0; !terminated && i < _tunprops.size(); ++i) {
        T val;
        ::DWORD retsize = sizeof(val);
        if (SUCCEEDED(_tunprops[i]->Get(propset, propid, NULL, 0, &val, retsize, &retsize))) {
            SELECTPROP(terminated, found, value, val, ps);
        }
    }
    return found;
}


//-----------------------------------------------------------------------------
// Check if a signal is present and locked
//-----------------------------------------------------------------------------

bool ts::Tuner::signalLocked(ReportInterface& report)
{
    if (!_is_open) {
        report.error("DVB tuner not open");
        return false;
    }

    bool found;
    ::BOOL locked = 0;
    SEARCHPROP(found, locked, psHIGHEST,
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

bool ts::Tuner::getSignalStrength_mdB(::LONG& strength)
{
    bool found;
    SEARCHPROP(found, strength, psHIGHEST,
               _sigstats, get_SignalStrength, ::LONG,
               KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_STRENGTH, ::LONG);
    return found;
}


//-----------------------------------------------------------------------------
// Return signal strength, in percent (0=bad, 100=good)
// Return a negative value on error.
//-----------------------------------------------------------------------------

int ts::Tuner::signalStrength(ReportInterface& report)
{
    if (!_is_open) {
        report.error("DVB tuner not open");
        return false;
    }

    // Use -100 dB (-100 000 mdB) as zero (null strength)
    // Avoid returning negative value on success.
    ::LONG strength;
    return getSignalStrength_mdB(strength) ? std::max(0, 100 + int(strength) / 1000) : -1;
}


//-----------------------------------------------------------------------------
// Return signal quality, in percent (0=bad, 100=good)
// Return a negative value on error.
//-----------------------------------------------------------------------------

int ts::Tuner::signalQuality(ReportInterface& report)
{
    if (!_is_open) {
        report.error("DVB tuner not open");
        return false;
    }

    bool found;
    ::LONG quality = 0;
    SEARCHPROP(found, quality, psHIGHEST,
               _sigstats, get_SignalQuality, ::LONG,
               KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_QUALITY, ::LONG);

    return found ? int(quality) : -1;
}


//-----------------------------------------------------------------------------
// Get the current tuning parameters
//-----------------------------------------------------------------------------

bool ts::Tuner::getCurrentTuning(TunerParameters& params, bool reset_unknown, ReportInterface& report)
{
    if (!_is_open) {
        report.error("DVB tuner not open");
        return false;
    }

    // Check subclass of TunerParameters
    if (params.tunerType() != _tuner_type) {
        report.error("inconsistent tuner parameter type");
        return false;
    }

    // Search individual tuning parameters
    bool found;
    switch (_tuner_type) {

        case DVB_S: {
            TunerParametersDVBS* tpp = dynamic_cast<TunerParametersDVBS*>(&params);
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
            SEARCHPROP(found, spinv, psFIRST,
                       _demods, get_SpectralInversion, ::SpectralInversion,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION, ::SpectralInversion);
            tpp->inversion = found ? ts::SpectralInversion (spinv) : SPINV_AUTO;
            // Symbol rate
            ::ULONG symrate = 0;
            SEARCHPROP(found, symrate, psHIGHEST,
                       _demods, get_SymbolRate, ::ULONG,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SYMBOL_RATE, ::ULONG);
            if (found) {
                tpp->symbol_rate = uint32_t(symrate);
            }
            // Inner FEC
            ::BinaryConvolutionCodeRate fec = ::BDA_BCC_RATE_NOT_SET;
            SEARCHPROP(found, fec, psFIRST,
                       _demods, get_InnerFECRate, ::BinaryConvolutionCodeRate,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE, ::BinaryConvolutionCodeRate);
            tpp->inner_fec = found ? ts::InnerFEC (fec) : ts::FEC_AUTO;
            // Modulation
            ::ModulationType mod = ::BDA_MOD_NOT_SET;
            SEARCHPROP(found, mod, psFIRST,
                       _demods, get_ModulationType, ::ModulationType,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE, ::ModulationType);
            tpp->modulation = found ? ts::Modulation(mod) : QPSK;
            // Delivery system.
            // Found no way to get DVB-S vs. DVB-S2 on Windows.
            // Make a not quite correct assumption, based on modulation type.
            tpp->delivery_system = tpp->modulation == QPSK ? DS_DVB_S : DS_DVB_S2;
            // DVB-S2 pilot
            ::Pilot pilot = ::BDA_PILOT_NOT_SET;
            SEARCHPROP(found, pilot, psFIRST,
                       _demods2, get_Pilot, ::Pilot,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_PILOT, ::Pilot);
            tpp->pilots = found ? ts::Pilot(pilot) : PILOT_AUTO;
            // DVB-S2 roll-off factor
            ::RollOff roff = ::BDA_ROLL_OFF_NOT_SET;
            SEARCHPROP(found, roff, psFIRST,
                       _demods2, get_RollOff, ::RollOff,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_ROLL_OFF, ::RollOff);
            tpp->roll_off = found ? ts::RollOff(roff) : ROLLOFF_AUTO;
            break;
        }

        case DVB_C: {
            TunerParametersDVBC* tpp = dynamic_cast<TunerParametersDVBC*>(&params);
            assert(tpp != 0);
            if (reset_unknown) {
                tpp->frequency = 0;
                tpp->symbol_rate = 0;
            }
            // Spectral inversion
            ::SpectralInversion spinv = ::BDA_SPECTRAL_INVERSION_NOT_SET;
            SEARCHPROP(found, spinv, psFIRST,
                       _demods, get_SpectralInversion, ::SpectralInversion,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION, ::SpectralInversion);
            tpp->inversion = found ? ts::SpectralInversion (spinv) : SPINV_AUTO;
            // Symbol rate
            ::ULONG symrate = 0;
            SEARCHPROP(found, symrate, psHIGHEST,
                       _demods, get_SymbolRate, ::ULONG,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SYMBOL_RATE, ::ULONG);
            if (found) {
                tpp->symbol_rate = uint32_t(symrate);
            }
            // Inner FEC
            ::BinaryConvolutionCodeRate fec = ::BDA_BCC_RATE_NOT_SET;
            SEARCHPROP(found, fec, psFIRST,
                       _demods, get_InnerFECRate, ::BinaryConvolutionCodeRate,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE, ::BinaryConvolutionCodeRate);
            tpp->inner_fec = found ? ts::InnerFEC(fec) : ts::FEC_AUTO;
            // Modulation
            ::ModulationType mod = ::BDA_MOD_NOT_SET;
            SEARCHPROP(found, mod, psFIRST,
                       _demods, get_ModulationType, ::ModulationType,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE, ::ModulationType);
            tpp->modulation = found ? ts::Modulation(mod) : QAM_AUTO;
            break;
        }

        case DVB_T: {
            TunerParametersDVBT* tpp = dynamic_cast<TunerParametersDVBT*>(&params);
            assert(tpp != 0);
            if (reset_unknown) {
                tpp->frequency = 0;
            }
            // Spectral inver   sion
            ::SpectralInversion spinv = ::BDA_SPECTRAL_INVERSION_NOT_SET;
            SEARCHPROP(found, spinv, psFIRST,
                       _demods, get_SpectralInversion, ::SpectralInversion,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION, ::SpectralInversion);
            tpp->inversion = found ? ts::SpectralInversion(spinv) : SPINV_AUTO;
            // High priority FEC
            ::BinaryConvolutionCodeRate fec = ::BDA_BCC_RATE_NOT_SET;
            SEARCHPROP(found, fec, psFIRST,
                       _demods, get_InnerFECRate, ::BinaryConvolutionCodeRate,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_INNER_FEC_RATE, ::BinaryConvolutionCodeRate);
            tpp->fec_hp = found ? ts::InnerFEC(fec) : ts::FEC_AUTO;
            // Modulation
            ::ModulationType mod = ::BDA_MOD_NOT_SET;
            SEARCHPROP(found, mod, psFIRST,
                       _demods, get_ModulationType, ::ModulationType,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE, ::ModulationType);
            tpp->modulation = found ? ts::Modulation(mod) : QAM_AUTO;
            // Other DVB-T parameters, not supported in IBDA_DigitalDemodulator
            // but which may be supported as properties.
            ::TransmissionMode tm = ::BDA_XMIT_MODE_NOT_SET;
            tpp->transmission_mode =
                searchTunerProperty(KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_TRANSMISSION_MODE, tm, psFIRST) ?
                ts::TransmissionMode(tm) : TM_AUTO;
            ::GuardInterval gi = ::BDA_GUARD_NOT_SET;
            tpp->guard_interval =
                searchTunerProperty(KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_GUARD_INTERVAL, gi, psFIRST) ?
                GuardInterval(gi) : GUARD_AUTO;
            // Other DVB-T parameters, not supported at all
            tpp->bandwidth = BW_AUTO;
            tpp->hierarchy = HIERARCHY_AUTO;
            tpp->fec_lp = FEC_AUTO;
            break;
        }

        case ATSC: {
            TunerParametersATSC* tpp = dynamic_cast<TunerParametersATSC*>(&params);
            assert(tpp != 0);
            if (reset_unknown) {
                tpp->frequency = 0;
            }
            // Spectral inversion
            ::SpectralInversion spinv = ::BDA_SPECTRAL_INVERSION_NOT_SET;
            SEARCHPROP(found, spinv, psFIRST,
                       _demods, get_SpectralInversion, ::SpectralInversion,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_SPECTRAL_INVERSION, ::SpectralInversion);
            tpp->inversion = found ? ts::SpectralInversion(spinv) : SPINV_AUTO;
            // Modulation
            ::ModulationType mod = ::BDA_MOD_NOT_SET;
            SEARCHPROP(found, mod, psFIRST,
                       _demods, get_ModulationType, ::ModulationType,
                       KSPROPSETID_BdaDigitalDemodulator, KSPROPERTY_BDA_MODULATION_TYPE, ::ModulationType);
            tpp->modulation = found ? ts::Modulation(mod) : QAM_AUTO;
            break;
        }

        default: {
            report.error("cannot convert BDA tuning parameters to " + TunerTypeEnum.name(_tuner_type) + " parameters");
            return false;
        }
    }

    return true;
}


//-----------------------------------------------------------------------------
// Tune to the specified parameters and start receiving.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::tune(const TunerParameters& params, ReportInterface& report)
{
    if (!_is_open) {
        report.error("DVB tuner not open");
        return false;
    }
    else {
        return internalTune(params, report);
    }
}


//-----------------------------------------------------------------------------
// Internal tune method, works also if the tuner is not in open state.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::internalTune(const TunerParameters& params, ReportInterface& report)
{
    // Check subclass of TunerParameters
    if (params.tunerType() != _tuner_type) {
        report.error("inconsistent tuner parameter type");
        return false;
    }

    // Create a DirectShow tune request
    ComPtr<::ITuneRequest> tune_request;
    if (!CreateTuneRequest(tune_request, _tuning_space.pointer(), params, report)) {
        return false;
    }
    assert(!tune_request.isNull());

    // Tune to transponder
    ::HRESULT hr = _tuner->put_TuneRequest(tune_request.pointer());
    return ComSuccess(hr, "DirectShow tuning error", report);
}


//-----------------------------------------------------------------------------
// Start receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::start(ReportInterface& report)
{
    if (!_is_open) {
        report.error("DVB tuner not open");
        return false;
    }

    // Set media samples queue size.
    _sink_filter->SetMaxMessages(_sink_queue_size);

    // Run the graph.
    if (!_graph.run(report)) {
        return false;
    }

    // If the tuner was previously started/stopped on a frequency with signal on it,
    // it has been observed that remaining packets from the previous run were still
    // there. Wait a little bit and reflush after Run() to avoid that.
    // Yes, this is a horrible hack, but if you have a better fix...
    SleepThread(50); // milliseconds
    _sink_filter->Flush();

    // If a signal timeout was specified, read a packet with timeout
    if (_signal_timeout > 0) {
        TSPacket pack;
        if (_sink_filter->Read(&pack, sizeof(pack), _signal_timeout) == 0) {
            if (!_signal_timeout_silent) {
                report.error("no input DVB signal after %" FMT_INT64 "d milliseconds", _signal_timeout);
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

bool ts::Tuner::stop(ReportInterface& report)
{
    return _is_open && _graph.stop(report);
}


//-----------------------------------------------------------------------------
// Timeout for receive operation (none by default).
// If zero, no timeout is applied.
// Return true on success, false on errors.
//-----------------------------------------------------------------------------

bool ts::Tuner::setReceiveTimeout(MilliSecond timeout, ReportInterface&)
{
    _receive_timeout = timeout;
    return true;
}


//-----------------------------------------------------------------------------
// Read complete 188-byte TS packets in the buffer and return the
// number of actually received packets (in the range 1 to max_packets).
// Returning zero means error or end of input.
//-----------------------------------------------------------------------------

size_t ts::Tuner::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, ReportInterface& report)
{
    if (!_is_open) {
        report.error("DVB tuner not open");
        return 0;
    }

    // Read packets from the tuner device

    size_t got_size;

    if (_receive_timeout <= 0) {
        got_size = _sink_filter->Read(buffer, max_packets * PKT_SIZE);
    }
    else {
        const Time limit(Time::CurrentUTC() + _receive_timeout);
        got_size = _sink_filter->Read(buffer, max_packets * PKT_SIZE, _receive_timeout);
        if (got_size == 0 && Time::CurrentUTC() >= limit) {
            report.error("receive timeout on " + _device_name);
        }
    }

    return got_size / PKT_SIZE;
}


//-----------------------------------------------------------------------------
// Display the characteristics and status of the tuner.
//-----------------------------------------------------------------------------

std::ostream& ts::Tuner::displayStatus(std::ostream& strm, const std::string& margin, ReportInterface& report)
{
    if (!_is_open) {
        report.error("DVB tuner not open");
        return strm;
    }

    strm << margin << "Signal locked:    " << YesNo(signalLocked(report)) << std::endl;
    int quality = signalQuality(report);
    if (quality >= 0) {
        strm << margin << "Signal quality:   " << quality << " %" << std::endl;
    }
    ::LONG strength;
    if (getSignalStrength_mdB(strength)) {
        strm << margin << "Signal strength:  " << Decimal(strength) << " milli dB" << std::endl;
    }
    strm << margin << "Network provider: " << _provider_name << std::endl
         << std::endl
         << margin << "DirectShow graph:" << std::endl;
    _graph.display(strm, report, margin + "  ", true);

    return strm;
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
            TunerPtr tptr(tuner == 0 ? new Tuner(tuner_name) : 0);
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
                tuner_list->push_back(tptr);
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

bool ts::Tuner::buildGraph(::IMoniker* provider_moniker, ::IMoniker* tuner_moniker, ReportInterface& report)
{
    // ReportInterface to use when errors shall be reported in debug mode only
    ReportInterface& debug_report(report.debug() ? report : NULLREP);

    // Get friendly name of this network provider
    _provider_name = GetStringPropertyBag(provider_moniker, L"FriendlyName", report);
    report.debug("trying provider filter \"" + _provider_name + "\"");

    // Create an instance of network provider from moniker
    _provider_filter.bindToObject(provider_moniker, ::IID_IBaseFilter, report);
    if (_provider_filter.isNull()) {
        report.debug("failed to create an instance of \"" + _provider_name + "\"");
        return false;
    }

    // Get class id of the network provider and convert it to our tuner type
    ::GUID provider_guid;
    ::HRESULT hr = _provider_filter->GetClassID(&provider_guid);
    if (!ComSuccess(hr, "cannot get class id of network provider \"" + _provider_name + "\"", report)) {
        return false;
    }
    if (!NetworkProviderToTunerType(provider_guid, _tuner_type)) {
        report.debug("Cannot map  \"" + _provider_name + "\" (class " + NameGUID(provider_guid) + ") to known tuner type");
        return false;
    }

    // Get interfaces of the network provider filter
    _net_provider.queryInterface(_provider_filter.pointer(), ::IID_IBDA_NetworkProvider, report);
    _tuner.queryInterface(_provider_filter.pointer(), ::IID_ITuner, report);
    if (_net_provider.isNull() || _tuner.isNull()) {
        report.debug("Interface IID_IBDA_NetworkProvider of \"%s\": %sfound", _provider_name.c_str(), _net_provider.isNull() ? "NOT " : "");
        report.debug("Interface IID_ITuner of \"%s\": %sfound", _provider_name.c_str(), _tuner.isNull() ? "NOT " : "");
        return false;
    }

    // List all tuning spaces from ITuner interface of network provider.
    // Try to find a default tuning space.
    ComPtr<::IEnumTuningSpaces> enum_tspace;
    hr = _tuner->EnumTuningSpaces(enum_tspace.creator());
    if (ComSuccess(hr, "cannot enumerate tuning spaces from \"" + _provider_name + "\"", report)) {
        if (hr != S_OK) {
            report.debug("no tuning space found");
        }
        else {
            assert(enum_tspace.pointer() != 0);
            ComPtr<::ITuningSpace> tspace;
            while (enum_tspace->Next(1, tspace.creator(), NULL) == S_OK) {
                // Display tuning space in debug mode
                const std::string fname(GetTuningSpaceFriendlyName(tspace.pointer(), report));
                const std::string uname(GetTuningSpaceUniqueName(tspace.pointer(), report));
                report.debug("found tuning space \"%s\" (%s) for provider \"%s\"", fname.c_str(), uname .c_str(), _provider_name.c_str());
                // If a default tuning space is already found, nothing to do
                if (!_tuning_space.isNull()) {
                    continue;
                }
                // Check if this tuning space can be used as default. Get DVB system type:
                // first get IDVBTuningSpace interface of tuning space (may not support it)
                ComPtr<::IDVBTuningSpace> dvb_tspace;
                dvb_tspace.queryInterface(tspace.pointer(), ::IID_IDVBTuningSpace, debug_report);
                if (dvb_tspace.isNull()) {
                    // Not a DVB tuning space, silently ignore it.
                    report.debug("tuning space \"%s\" does not support IID_IDVBTuningSpace interface", fname.c_str());
                    continue;
                }
                // Get DVB system type
                ::DVBSystemType systype;
                hr = dvb_tspace->get_SystemType(&systype);
                if (!ComSuccess(hr, "cannot get DVB system type from tuning space \"" + fname + "\"", report)) {
                    continue;
                }
                report.debug("DVB system type is %d for tuning space \"%s\"", int(systype), fname.c_str());
                // Check if DVB system type matches our tuner type
                if ((_tuner_type == ts::DVB_S && systype == ::DVB_Satellite) ||
                    (_tuner_type == ts::DVB_T && systype == ::DVB_Terrestrial) ||
                    (_tuner_type == ts::DVB_C && systype == ::DVB_Cable))
                {
                    // Use this tuning space
                    hr = _tuner->put_TuningSpace(tspace.pointer());
                    if (ComSuccess(hr, "fail to set default tuning space \"" + fname + "\" to provider \"" + _provider_name + "\"", debug_report)) {
                        _tuning_space = tspace;
                    }
                }
            }
        }
    }

    // Add the delivery system
    switch (_tuner_type) {
        case ts::DVB_S:
            _delivery_systems.set(DS_DVB_S);
            // No way to check if DS_DVB_S2 is supported
            break;
        case ts::DVB_C:
            _delivery_systems.set(DS_DVB_C);
            break;
        case ts::DVB_T:
            _delivery_systems.set(DS_DVB_T);
            break;
        case ts::ATSC:
            _delivery_systems.set(DS_ATSC);
            break;
    }

    // Create an instance of tuner from moniker
    _tuner_filter.bindToObject(tuner_moniker, ::IID_IBaseFilter, report);
    if (_tuner_filter.isNull()) {
        return false;
    }

    // Create the Filter Graph and add the filters.
    if (!_graph.initialize(report) ||
        !_graph.addFilter(_provider_filter.pointer(), L"NetworkProvider", report) ||
        !_graph.addFilter(_tuner_filter.pointer(), L"Tuner", report))
    {
        return false;
    }

    // Try to start a graph: network provider -> tuner
    if (!_graph.connectFilters(_provider_filter.pointer(), _tuner_filter.pointer(), debug_report)) {
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
        TunerParameters* params = TunerParameters::Factory(_tuner_type);
        if (params == 0) {
            report.debug("cannot build tuner parameters for these network provider");
            // The tuner is not compatible with this network provider.
            // Silently give up this network provider.
            return false;
        }
        // Issue a tuning request, ignore errors
        internalTune(*params, debug_report);
        // Then retry to start the graph
        if (!_graph.connectFilters(_provider_filter.pointer(), _tuner_filter.pointer(), debug_report)) {
            // The tuner is not compatible with this network provider.
            // Silently give up this network provider.
            return false;
        }
    }

    // Get current (default?) tuning space of this network provider
    hr = _tuner->get_TuningSpace(_tuning_space.creator());
    if (!ComSuccess(hr, "ITuner::get_TuningSpace", report)) {
        return false;
    }

    // Get tuning space names
    _tuning_space_fname = GetTuningSpaceFriendlyName(_tuning_space.pointer(), report);
    _tuning_space_uname = GetTuningSpaceUniqueName(_tuning_space.pointer(), report);

    // Good start, we successfully connected an network provider to a tuner filter.
    report.debug("using provider filter \"" + _provider_name +
                 "\", tuning space \"" + _tuning_space_uname +
                 "\" (\"" + _tuning_space_fname + "\")");

    // Try to build the rest of the graph starting at tuner filter.
    // Usually work with Terratec driver for instance.
    report.debug("trying direct connection from tuner (no receiver)");
    bool graph_done = buildCaptureGraph(_tuner_filter, report);

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
            std::string receiver_name(GetStringPropertyBag(receiver_monikers[receiver_index].pointer(), L"FriendlyName", debug_report));
            report.debug("trying receiver filter \"" + receiver_name + "\"");

            // Create an instance of this receiver filter from moniker
            ComPtr<::IBaseFilter> receiver_filter;
            receiver_filter.bindToObject(receiver_monikers[receiver_index].pointer(), ::IID_IBaseFilter, debug_report);
            if (receiver_filter.isNull()) {
                continue; // give up this receiver filter
            }

            // Add the filter in the graph
            if (!_graph.addFilter(receiver_filter.pointer(), L"Receiver", report)) {
                continue; // give up this receiver filter
            }

            // Try to connect the tuner to the receiver
            if (!_graph.connectFilters(_tuner_filter.pointer(), receiver_filter.pointer(), debug_report)) {
                // This receiver is not compatible, remove it from the graph
                _graph.removeFilter(receiver_filter.pointer(), debug_report);
                continue;
            }

            // Try to build the rest of the graph
            if (buildCaptureGraph(receiver_filter, report)) {
                graph_done = true;
                report.debug("using receiver filter \"" + receiver_name + "\"");
            }
        }
    }
    if (!graph_done) {
        return false;
    }

    // Locate all instances of some interfaces in the tuner topology
    _demods.clear();
    _demods2.clear();
    _sigstats.clear();
    _tunprops.clear();

    // Lookup all internal nodes in the BDA topology
    ComPtr<::IBDA_Topology> topology;
    topology.queryInterface(_tuner_filter.pointer(), ::IID_IBDA_Topology, NULLREP);
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
    if (SUCCEEDED(_tuner_filter->EnumPins(enum_pins.creator()))) {
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

    report.debug("IBDA_DigitalDemodulator in tuner: %" FMT_SIZE_T "u", _demods.size());
    report.debug("IBDA_DigitalDemodulator2 in tuner: %" FMT_SIZE_T "u", _demods2.size());
    report.debug("IBDA_SignalStatistics in tuner: %" FMT_SIZE_T "u", _sigstats.size());
    report.debug("IKsPropertySet in tuner: %" FMT_SIZE_T "u", _tunprops.size());

    return true;
}


//-----------------------------------------------------------------------------
// Try to build the part of the graph starting at the tee filter.
// The specified base filter is either the tuner filter or some
// other intermediate receiver filter downstream the tuner.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::Tuner::buildCaptureGraph(const ComPtr<::IBaseFilter>& base_filter, ReportInterface& report)
{
    // ReportInterface to use when errors shall be reported in debug mode only
    ReportInterface& debug_report(report.debug() ? report : NULLREP);

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
    if (!_graph.addFilter(tee_filter.pointer(), L"Tee", report)) {
        return false;
    }

    // After this point, we cannot simply return false on error since
    // the graph needs some cleanup.

    // Try to connect the "base" filter (tuner or receiver) to the tee filter.
    bool ok = _graph.connectFilters(base_filter.pointer(), tee_filter.pointer(), debug_report);

    // Create branch A of graph: Create a sink filter, add it to the graph and connect it to the tee.
    ComPtr<SinkFilter> sink_filter(new SinkFilter(report));
    CheckNonNull(sink_filter.pointer());
    ok = ok && 
        _graph.addFilter(sink_filter.pointer(), L"Sink/Capture", report) &&
        _graph.connectFilters(tee_filter.pointer(), sink_filter.pointer(), debug_report);

    // Create branch B of graph: Create an MPEG-2 demultiplexer
    ComPtr<::IBaseFilter> demux_filter(::CLSID_MPEG2Demultiplexer, ::IID_IBaseFilter, report);
    ok = ok &&
        !demux_filter.isNull() &&
        _graph.addFilter(demux_filter.pointer(), L"Demux", report) &&
        _graph.connectFilters(tee_filter.pointer(), demux_filter.pointer(), debug_report);

    // Now, we need to connect a Transport Information Filter (TIF).
    // There is no predefined CLSID for this one and we must loop on all
    // filters with category KSCATEGORY_BDA_TRANSPORT_INFORMATION
    ComPtr<::IEnumMoniker> enum_tif;
    if (ok) {
        ::HRESULT hr = enum_devices->CreateClassEnumerator(KSCATEGORY_BDA_TRANSPORT_INFORMATION, enum_tif.creator(), 0);
        ok = ComSuccess(hr, "CreateClassEnumerator (for TIF)", report) && hr == S_OK;
    }

    // Loop on all enumerated TIF
    ComPtr<::IMoniker> tif_moniker;
    bool tif_found = false;
    while (ok && !tif_found && enum_tif->Next(1, tif_moniker.creator(), NULL) == S_OK) {

        // Get friendly name of this TIF
        std::string tif_name(GetStringPropertyBag(tif_moniker.pointer(), L"FriendlyName", report));
        report.debug("trying TIF \"" + tif_name + "\"");

        // Create an instance of this TIF from moniker
        ComPtr<::IBaseFilter> tif_filter;
        tif_filter.bindToObject(tif_moniker.pointer(), ::IID_IBaseFilter, report);
        if (tif_filter.isNull()) {
            continue; // give up this TIF
        }

        // Add the TIF in the graph
        if (!_graph.addFilter(tif_filter.pointer(), L"TIF", report)) {
            continue; // give up this TIF
        }

        // Try to connect demux filter to tif
        if (_graph.connectFilters(demux_filter.pointer(), tif_filter.pointer(), debug_report)) {
            tif_found = true;
            report.debug("using TIF \"" + tif_name + "\"");
        }
        else {
            // This tif is not compatible, remove it from the graph
            _graph.removeFilter(tif_filter.pointer(), report);
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
    _graph.cleanupDownstream(_tuner_filter.pointer(), debug_report);

    // Remove all created filters from the graph. Ignore errors.
    // This is necessary if a filter was created and added to the graph but
    // not connected (if connected, it was removed by CleanupDownstream).
    if (!tee_filter.isNull()) {
        _graph.removeFilter(tee_filter.pointer(), report);
    }
    if (!sink_filter.isNull()) {
        _graph.removeFilter(sink_filter.pointer(), report);
    }
    if (!demux_filter.isNull()) {
        _graph.removeFilter(demux_filter.pointer(), report);
    }

    return false;
}
