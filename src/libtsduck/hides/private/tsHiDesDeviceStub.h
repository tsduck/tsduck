//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of stub methods for HiDesDevice class.
//!  To be included when this class is not implemented.
//!
//----------------------------------------------------------------------------

#pragma once

#include "tsHiDesDevice.h"

namespace {
    bool NotImplemented(ts::Report& report)
    {
        report.error(u"HiDes devices are not implemented on this system");
        return false;
    }
}

ts::HiDesDevice::HiDesDevice() :
    _is_open(false),
    _guts(nullptr)
{
}

ts::HiDesDevice::~HiDesDevice()
{
}

bool ts::HiDesDevice::GetAllDevices(HiDesDeviceInfoList& devices, Report& report)
{
    devices.clear();
    return NotImplemented(report);
}

bool ts::HiDesDevice::open(int index, Report& report)
{
    return NotImplemented(report);
}

bool ts::HiDesDevice::open(const UString& name, Report& report)
{
    return NotImplemented(report);
}

bool ts::HiDesDevice::getInfo(HiDesDeviceInfo& info, Report& report) const
{
    info.clear();
    return NotImplemented(report);
}

bool ts::HiDesDevice::close(Report& report)
{
    return NotImplemented(report);
}

bool ts::HiDesDevice::setGain(int& gain, Report& report)
{
    gain = 0;
    return NotImplemented(report);
}

bool ts::HiDesDevice::getGain(int& gain, Report& report)
{
    gain = 0;
    return NotImplemented(report);
}

bool ts::HiDesDevice::getGainRange(int& minGain, int& maxGain, uint64_t frequency, BandWidth bandwidth, Report& report)
{
    minGain = maxGain = 0;
    return NotImplemented(report);
}

bool ts::HiDesDevice::setDCCalibration(int dcI, int dcQ, ts::Report &report)
{
    return NotImplemented(report);
}

bool ts::HiDesDevice::tune(const ModulationArgs& params, Report& report)
{
    return NotImplemented(report);
}

bool ts::HiDesDevice::startTransmission(Report& report)
{
    return NotImplemented(report);
}

bool ts::HiDesDevice::stopTransmission(Report& report)
{
    return NotImplemented(report);
}

bool ts::HiDesDevice::send(const TSPacket* packets, size_t packet_count, Report& report, AbortInterface* abort)
{
    return NotImplemented(report);
}
