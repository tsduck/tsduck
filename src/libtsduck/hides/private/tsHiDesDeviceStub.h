//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

ts::HiDesDevice::HiDesDevice()
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
