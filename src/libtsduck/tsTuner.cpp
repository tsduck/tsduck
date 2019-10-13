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

#include "tsTuner.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr ts::MilliSecond ts::Tuner::DEFAULT_SIGNAL_TIMEOUT;
#endif

// List of delivery systems, from most preferred to least preferred.
const std::list<ts::DeliverySystem> ts::Tuner::_preferred_order
({
     // On a tuner, we consider terrestrial capabilities first.
     DS_DVB_T,
     DS_DVB_T2,
     DS_ATSC,
     DS_ISDB_T,
     DS_DTMB,
     DS_CMMB,
     // Then satellite capabilities.
     DS_DVB_S,
     DS_DVB_S2,
     DS_DVB_S_TURBO,
     DS_ISDB_S,
     DS_DSS,
     // Then cable capabilities.
     DS_DVB_C_ANNEX_A,
     DS_DVB_C_ANNEX_B,
     DS_DVB_C_ANNEX_C,
     DS_DVB_C2,
     DS_ISDB_C,
     // Exotic capabilities come last.
     DS_DVB_H,
     DS_ATSC_MH,
     DS_DAB,
     DS_UNDEFINED
});


//-----------------------------------------------------------------------------
// Constructors
//-----------------------------------------------------------------------------

ts::Tuner::Tuner() :
    _is_open(false),
    _info_only(true),
    _device_name(),
    _device_info(),
    _signal_timeout(DEFAULT_SIGNAL_TIMEOUT),
    _signal_timeout_silent(false),
    _receive_timeout(0),
    _delivery_systems(),
    _guts(nullptr)
{
    allocateGuts();
    CheckNonNull(_guts);
}

ts::Tuner::Tuner(const UString& device_name, bool info_only, Report& report) :
    Tuner()
{
    this->open(device_name, info_only, report);
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

ts::Tuner::~Tuner()
{
    if (_guts != nullptr) {
        close(NULLREP);
        deleteGuts();
        _guts = nullptr;
    }
}


//-----------------------------------------------------------------------------
// Delivery systems
//-----------------------------------------------------------------------------

ts::DeliverySystemSet ts::Tuner::deliverySystems() const
{
    return _delivery_systems;
}

void ts::Tuner::clearDeliverySystems()
{
    _delivery_systems.clear();
}

void ts::Tuner::addDeliverySystem(DeliverySystem ds)
{
    _delivery_systems.insert(ds);
}

bool ts::Tuner::hasDeliverySystem() const
{
    return !_delivery_systems.empty();
}

bool ts::Tuner::hasDeliverySystem(DeliverySystem ds) const
{
    return _delivery_systems.find(ds) != _delivery_systems.end();
}

ts::DeliverySystem ts::Tuner::defaultDeliverySystem() const
{
    // Inspect delivery systems in decreasing order of preference.
    for (auto it = _preferred_order.begin(); it != _preferred_order.end(); ++it) {
        if (hasDeliverySystem(*it)) {
            return *it;
        }
    }
    return DS_UNDEFINED;
}

ts::UString ts::Tuner::deliverySystemsString() const
{
    UString str;
    // Build list of delivery systems in decreasing order of preference.
    for (auto it = _preferred_order.begin(); it != _preferred_order.end(); ++it) {
        if (hasDeliverySystem(*it)) {
            if (!str.empty()) {
                str += u", ";
            }
            str += DeliverySystemEnum.name(int(*it));
        }
    }
    return str.empty() ? u"none" : str;
}


//-----------------------------------------------------------------------------
// Set portable timeout properties.
//-----------------------------------------------------------------------------

void ts::Tuner::setSignalTimeout(MilliSecond t)
{
    _signal_timeout = t;
}

void ts::Tuner::setSignalTimeoutSilent(bool silent)
{
    _signal_timeout_silent = silent;
}


//-----------------------------------------------------------------------------
// Check the consistency of tune() parameters from in_params.
//-----------------------------------------------------------------------------

bool ts::Tuner::checkTuneParameters(ModulationArgs& params, Report& report) const
{
    // Cannot tune if the device is not open.
    if (!_is_open) {
        report.error(u"tuner not open");
        return false;
    }

    // Get default (preferred) delivery system from tuner when needed.
    if (!params.delivery_system.set()) {
        params.delivery_system = defaultDeliverySystem();
        if (params.delivery_system == DS_UNDEFINED) {
            report.error(u"no tuning delivery system specified");
            return false;
        }
        else if (_delivery_systems.size() > 1) {
            report.verbose(u"using default deliver system %s", {DeliverySystemEnum.name(params.delivery_system.value())});
        }
    }

    // Set all unset tuning parameters to their default value.
    params.setDefaultValues();

    // Check if all specified values are supported on the operating system.
    return
        CheckModVar(params.inversion, u"spectral inversion", SpectralInversionEnum, report) &&
        CheckModVar(params.inner_fec, u"FEC", InnerFECEnum, report) &&
        CheckModVar(params.modulation, u"modulation", ModulationEnum, report) &&
        CheckModVar(params.bandwidth, u"bandwidth", BandWidthEnum, report) &&
        CheckModVar(params.fec_hp, u"FEC", InnerFECEnum, report) &&
        CheckModVar(params.fec_lp, u"FEC", InnerFECEnum, report) &&
        CheckModVar(params.transmission_mode, u"transmission mode", TransmissionModeEnum, report) &&
        CheckModVar(params.guard_interval, u"guard interval", GuardIntervalEnum, report) &&
        CheckModVar(params.hierarchy, u"hierarchy", HierarchyEnum, report) &&
        CheckModVar(params.pilots, u"pilots", PilotEnum, report) &&
        CheckModVar(params.roll_off, u"roll-off factor", RollOffEnum, report);
}
