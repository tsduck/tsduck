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

#include "tsTuner.h"
#include "tsNullReport.h"
#include "tsDuckContext.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr ts::MilliSecond ts::Tuner::DEFAULT_SIGNAL_TIMEOUT;
#endif


//-----------------------------------------------------------------------------
// Constructors
//-----------------------------------------------------------------------------

ts::Tuner::Tuner(DuckContext& duck) :
    _duck(duck),
    _is_open(false),
    _info_only(true),
    _device_name(),
    _device_info(),
    _device_path(),
    _signal_timeout(DEFAULT_SIGNAL_TIMEOUT),
    _signal_timeout_silent(false),
    _receive_timeout(0),
    _delivery_systems(),
    _guts(nullptr)
{
    allocateGuts();
    CheckNonNull(_guts);
}

ts::Tuner::Tuner(DuckContext& duck, const UString& device_name, bool info_only, Report& report) :
    Tuner(duck)
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
    if (params.delivery_system.value(DS_UNDEFINED) == DS_UNDEFINED) {
        params.delivery_system = _delivery_systems.preferred();
        if (params.delivery_system == DS_UNDEFINED) {
            report.error(u"no tuning delivery system specified");
            return false;
        }
        else if (_delivery_systems.size() > 1) {
            report.verbose(u"using default deliver system %s", {DeliverySystemEnum.name(params.delivery_system.value())});
        }
    }

    // Check if the delivery system is supported by this tuner.
    if (!_delivery_systems.contains(params.delivery_system.value())) {
        report.error(u"deliver system %s not supported on tuner %s", {DeliverySystemEnum.name(params.delivery_system.value()), _device_name});
        return false;
    }

    // Set all unset tuning parameters to their default value.
    params.setDefaultValues();

    // Add the tuner's standards to the execution context.
    _duck.addStandards(StandardsOf(params.delivery_system.value()));

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
