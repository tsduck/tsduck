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

#include "tsTuner.h"
#include "tsTunerDevice.h"
#include "tsTunerEmulator.h"
#include "tsDuckContext.h"


//-----------------------------------------------------------------------------
// Constructors and destructors.
// The physical tuner device object is always allocated.
// The tuner emulator is allocated on demand and used only when open.
//-----------------------------------------------------------------------------

ts::Tuner::Tuner(DuckContext& duck) :
    TunerBase(duck),
    _device(new TunerDevice(duck)),
    _emulator(nullptr),
    _current(_device)
{
    CheckNonNull(_device);
}

ts::Tuner::Tuner(DuckContext& duck, const UString& device_name, bool info_only) :
    Tuner(duck)
{
    this->open(device_name, info_only);
}

ts::Tuner::~Tuner()
{
    if (_device != nullptr) {
        _device->close(true);
        delete _device;
        _device = nullptr;
    }
    if (_emulator != nullptr) {
        _emulator->close(true);
        delete _emulator;
        _emulator = nullptr;
    }
    _current = nullptr;
}


//-----------------------------------------------------------------------------
// Open the tuner, switch to physical or emulated tuner.
//-----------------------------------------------------------------------------

bool ts::Tuner::open(const UString& device_name, bool info_only)
{
    if (_current->isOpen()) {
        _duck.report().error(u"internal error, tuner already open");
        return false;
    }
    else if (device_name.endWith(u".xml", CASE_INSENSITIVE)) {
        // The device name is an XML file, create a tuner emulator.
        if (_emulator == nullptr) {
            // First time we use an emulator, allocate one.
            _emulator = new TunerEmulator(_duck);
            CheckNonNull(_emulator);
        }
        if (_emulator->open(device_name, info_only)) {
            // Use the emulator as current device only when successfully used.
            _current = _emulator;
            return true;
        }
        else {
            // In case of failure, switch back to closed tuner device.
            _current = _device;
            return false;
        }
    }
    else {
        // Assume a physical device.
        _current = _device;
        return _device->open(device_name, info_only);
    }
}


//-----------------------------------------------------------------------------
// Close the tuner, reset to physical tuner (in closed state).
//-----------------------------------------------------------------------------

bool ts::Tuner::close(bool silent)
{
    // Close the current tuner, whichever it is.
    const bool status = _current->close(silent);

    // Switch back to (closed) physical tuner device.
    _current = _device;
    return status;
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

bool ts::Tuner::getSignalState(SignalState& state)
{
    return _current->getSignalState(state);
}

bool ts::Tuner::tune(ModulationArgs& params)
{
    return _current->tune(params);
}

bool ts::Tuner::start()
{
    return _current->start();
}

bool ts::Tuner::stop(bool silent)
{
    return _current->stop(silent);
}

void ts::Tuner::abort(bool silent)
{
    _current->abort(silent);
}

size_t ts::Tuner::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort)
{
    return _current->receive(buffer, max_packets, abort);
}

bool ts::Tuner::getCurrentTuning(ModulationArgs& params, bool reset_unknown)
{
    return _current->getCurrentTuning(params, reset_unknown);
}

void ts::Tuner::setSignalTimeout(MilliSecond t)
{
    return _current->setSignalTimeout(t);
}

void ts::Tuner::setSignalTimeoutSilent(bool silent)
{
    return _current->setSignalTimeoutSilent(silent);
}

bool ts::Tuner::setReceiveTimeout(MilliSecond t)
{
    return _current->setReceiveTimeout(t);
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

std::ostream& ts::Tuner::displayStatus(std::ostream& strm, const UString& margin, bool extended)
{
    return _current->displayStatus(strm, margin, extended);
}
