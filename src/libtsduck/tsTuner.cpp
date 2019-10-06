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


//-----------------------------------------------------------------------------
// Constructors
//-----------------------------------------------------------------------------

ts::Tuner::Tuner(const UString& device_name) :
    _is_open(false),
    _info_only(true),
    _tuner_type(DVB_T),
    _device_name(device_name),
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
    Tuner(device_name)
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

bool ts::Tuner::hasDeliverySystem(DeliverySystem ds) const
{
    return _delivery_systems.find(ds) != _delivery_systems.end();
}

ts::UString ts::Tuner::deliverySystemsString() const
{
    UStringVector str;
    for (auto ds = _delivery_systems.begin(); ds != _delivery_systems.end(); ++ds) {
        str.push_back(ts::DeliverySystemEnum.name(int(*ds)));
    }
    std::sort(str.begin(), str.end());
    return str.empty() ? u"none" : UString::Join(str);
}
