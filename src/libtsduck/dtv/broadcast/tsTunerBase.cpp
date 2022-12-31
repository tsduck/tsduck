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

#include "tsTunerBase.h"
#include "tsReport.h"
#include "tsModulationArgs.h"
#include "tsSignalState.h"
#include "tsDeliverySystem.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr ts::MilliSecond ts::TunerBase::DEFAULT_SIGNAL_TIMEOUT;
constexpr ts::MilliSecond ts::TunerBase::DEFAULT_SIGNAL_POLL;
constexpr size_t ts::TunerBase::DEFAULT_DEMUX_BUFFER_SIZE;
#endif


//-----------------------------------------------------------------------------
// Constructors and destructors.
//-----------------------------------------------------------------------------

ts::TunerBase::TunerBase(DuckContext& duck) :
    _duck(duck)
{
}

ts::TunerBase::~TunerBase()
{
}


//-----------------------------------------------------------------------------
// Unimplemented methods, return an error.
//-----------------------------------------------------------------------------

bool ts::TunerBase::unimplemented() const
{
    _duck.report().error(u"Digital tuners are not implemented");
    return false;
}

bool ts::TunerBase::open(const UString& device_name, bool info_only)
{
    return unimplemented();
}

bool ts::TunerBase::tune(ModulationArgs& params)
{
    return unimplemented();
}

bool ts::TunerBase::start()
{
    return unimplemented();
}

size_t ts::TunerBase::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort)
{
    return unimplemented();
}

bool ts::TunerBase::getCurrentTuning(ModulationArgs& params, bool reset_unknown)
{
    return unimplemented();
}


//-----------------------------------------------------------------------------
// Default methods which silently return nothing.
//-----------------------------------------------------------------------------

bool ts::TunerBase::isOpen() const
{
    return false;
}

bool ts::TunerBase::infoOnly() const
{
    return false;
}

const ts::DeliverySystemSet& ts::TunerBase::deliverySystems() const
{
    // A static unused empty value.
    static const DeliverySystemSet empty;
    return empty;
}

ts::UString ts::TunerBase::deviceName() const
{
    return UString();
}

ts::UString ts::TunerBase::deviceInfo() const
{
    return UString();
}

ts::UString ts::TunerBase::devicePath() const
{
    return UString();
}

bool ts::TunerBase::stop(bool silent)
{
    return false;
}

void ts::TunerBase::abort(bool silent)
{
}

bool ts::TunerBase::close(bool silent)
{
    return false;
}

void ts::TunerBase::setSignalTimeout(MilliSecond t)
{
}

void ts::TunerBase::setSignalTimeoutSilent(bool silent)
{
}

bool ts::TunerBase::setReceiveTimeout(MilliSecond timeout)
{
    return true;
}

ts::MilliSecond ts::TunerBase::receiveTimeout() const
{
    return 0;
}

bool ts::TunerBase::getSignalState(SignalState& state)
{
    state.clear();
    return true;
}

std::ostream& ts::TunerBase::displayStatus(std::ostream& strm, const UString& margin, bool extended)
{
    return strm;
}


//-----------------------------------------------------------------------------
// Set the Linux-specific parameters. Overriden on Linux only.
//-----------------------------------------------------------------------------

void ts::TunerBase::setSignalPoll(MilliSecond)
{
}

void ts::TunerBase::setDemuxBufferSize(size_t)
{
}


//-----------------------------------------------------------------------------
// Set the Windows-specific parameters. Overriden on Windows only.
//-----------------------------------------------------------------------------

void ts::TunerBase::setSinkQueueSize(size_t)
{
}

void ts::TunerBase::setReceiverFilterName(const UString&)
{
}


//-----------------------------------------------------------------------------
// Check the consistency of tune() parameters from in_params.
//-----------------------------------------------------------------------------

bool ts::TunerBase::checkTuneParameters(ModulationArgs& params) const
{
    // Cannot tune if the device is not open.
    if (!isOpen()) {
        _duck.report().error(u"tuner not open");
        return false;
    }

    // Get default (preferred) delivery system from tuner when needed.
    const DeliverySystemSet& delivery_systems(deliverySystems());
    if (params.delivery_system.value(DS_UNDEFINED) == DS_UNDEFINED) {
        params.delivery_system = delivery_systems.preferred();
        if (params.delivery_system == DS_UNDEFINED) {
            _duck.report().error(u"no tuning delivery system specified");
            return false;
        }
        else if (delivery_systems.size() > 1) {
            _duck.report().verbose(u"using default deliver system %s", {DeliverySystemEnum.name(params.delivery_system.value())});
        }
    }

    // Check if the delivery system is supported by this tuner.
    if (!delivery_systems.contains(params.delivery_system.value())) {
        _duck.report().error(u"deliver system %s not supported on tuner %s", {DeliverySystemEnum.name(params.delivery_system.value()), deviceName()});
        return false;
    }

    // Set all unset tuning parameters to their default value.
    params.setDefaultValues();

    // Add the tuner's standards to the execution context.
    _duck.addStandards(StandardsOf(params.delivery_system.value()));

    // Check if all specified values are supported on the operating system.
    return
        CheckModVar(params.inversion, u"spectral inversion", SpectralInversionEnum, _duck.report()) &&
        CheckModVar(params.inner_fec, u"FEC", InnerFECEnum, _duck.report()) &&
        CheckModVar(params.modulation, u"modulation", ModulationEnum, _duck.report()) &&
        CheckModVar(params.fec_hp, u"FEC", InnerFECEnum, _duck.report()) &&
        CheckModVar(params.fec_lp, u"FEC", InnerFECEnum, _duck.report()) &&
        CheckModVar(params.transmission_mode, u"transmission mode", TransmissionModeEnum, _duck.report()) &&
        CheckModVar(params.guard_interval, u"guard interval", GuardIntervalEnum, _duck.report()) &&
        CheckModVar(params.hierarchy, u"hierarchy", HierarchyEnum, _duck.report()) &&
        CheckModVar(params.pilots, u"pilots", PilotEnum, _duck.report()) &&
        CheckModVar(params.roll_off, u"roll-off factor", RollOffEnum, _duck.report());
}
