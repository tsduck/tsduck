//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
#include "tsTunerEmulator.h"
#include "tsNullReport.h"
#include "tsDuckContext.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// Constructors and destructors.
// The physical tuner device object is always allocated.
// The tuner emulator is allocated on demand.
//-----------------------------------------------------------------------------

ts::Tuner::Tuner(DuckContext& duck) :
    TunerBase(duck),
    _device(allocateDevice()),
    _emulator(nullptr),
    _current(_device)
{
    CheckNonNull(_device);
}

ts::Tuner::Tuner(DuckContext& duck, const UString& device_name, bool info_only, Report& report) :
    Tuner(duck)
{
    this->open(device_name, info_only, report);
}

ts::Tuner::~Tuner()
{
    if (_device != nullptr) {
        _device->close(NULLREP);
        delete _device;
        _device = nullptr;
    }
    if (_emulator != nullptr) {
        _emulator->close(NULLREP);
        delete _emulator;
        _emulator = nullptr;
    }
    _current = nullptr;
}


//-----------------------------------------------------------------------------
// Open the tuner, switch to physical or emulated tuner.
//-----------------------------------------------------------------------------

bool ts::Tuner::open(const UString& device_name, bool info_only, Report& report)
{
    if (_current->isOpen()) {
        report.error(u"internal error, tuner already open");
        return false;
    }
    else if (device_name.endWith(u".xml", CASE_INSENSITIVE)) {
        // The device name is an XML file, create a tuner emulator.
        assert(_emulator == nullptr);
        _current = _emulator = new TunerEmulator(_duck);
        CheckNonNull(_emulator);
        if (_emulator->open(device_name, info_only, report)) {
            return true;
        }
        else {
            delete _emulator;
            _emulator = nullptr;
            _current = _device;
            return false;
        }
    }
    else {
        // Assume a physical device.
        _current = _device;
        return _device->open(device_name, info_only, report);
    }
}


//-----------------------------------------------------------------------------
// Close the tuner, reset to physical tuner (in closed state).
//-----------------------------------------------------------------------------

bool ts::Tuner::close(Report& report)
{
    if (_emulator != nullptr) {
        // Close and deallocate the terminal emulator.
        const bool status = _emulator->close(report);
        delete _emulator;
        _emulator = nullptr;
        // Switch back to physical tuner device.
        _current = _device;
        return status;
    }
    else {
        // Close the physical tuner and keep the allocated object.
        return _device->close(report);
    }
}


//-----------------------------------------------------------------------------
// All other calls are redirected to the tuner emulator if allocated and
// to the physical tuner device otherwise.
//-----------------------------------------------------------------------------

bool ts::Tuner::isOpen() const
{
    return _current->isOpen();
}

bool ts::Tuner::infoOnly() const
{
    return _current->infoOnly();
}

const ts::DeliverySystemSet& ts::Tuner::deliverySystems() const
{
    return _current->deliverySystems();
}

ts::UString ts::Tuner::deviceName() const
{
    return _current->deviceName();
}

ts::UString ts::Tuner::deviceInfo() const
{
    return _current->deviceInfo();
}

ts::UString ts::Tuner::devicePath() const
{
    return _current->devicePath();
}

bool ts::Tuner::signalLocked(Report& report)
{
    return _current->signalLocked(report);
}

int ts::Tuner::signalStrength(Report& report)
{
    return _current->signalStrength(report);
}

int ts::Tuner::signalQuality(Report& report)
{
    return _current->signalQuality(report);
}

bool ts::Tuner::tune(ModulationArgs& params, Report& report)
{
    return _current->tune(params, report);
}

bool ts::Tuner::start(Report& report)
{
    return _current->start(report);
}

bool ts::Tuner::stop(Report& report)
{
    return _current->stop(report);
}

void ts::Tuner::abort()
{
    _current->abort();
}

size_t ts::Tuner::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, Report& report)
{
    return _current->receive(buffer, max_packets, abort, report);
}

bool ts::Tuner::getCurrentTuning(ModulationArgs& params, bool reset_unknown, Report& report)
{
    return _current->getCurrentTuning(params, reset_unknown, report);
}

void ts::Tuner::setSignalTimeout(MilliSecond t)
{
    return _current->setSignalTimeout(t);
}

void ts::Tuner::setSignalTimeoutSilent(bool silent)
{
    return _current->setSignalTimeoutSilent(silent);
}

bool ts::Tuner::setReceiveTimeout(MilliSecond t, Report& report)
{
    return _current->setReceiveTimeout(t, report);
}

ts::MilliSecond ts::Tuner::receiveTimeout() const
{
    return _current->receiveTimeout();
}

void ts::Tuner::setSignalPoll(MilliSecond t)
{
    return _current->setSignalPoll(t);
}

void ts::Tuner::setDemuxBufferSize(size_t s)
{
    return _current->setDemuxBufferSize(s);
}

void ts::Tuner::setSinkQueueSize(size_t s)
{
    return _current->setSinkQueueSize(s);
}

void ts::Tuner::setReceiverFilterName(const UString& name)
{
    return _current->setReceiverFilterName(name);
}

std::ostream& ts::Tuner::displayStatus(std::ostream& strm, const UString& margin, Report& report, bool extended)
{
    return _current->displayStatus(strm, margin, report, extended);
}
