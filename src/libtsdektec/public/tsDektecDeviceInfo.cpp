//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDektecDeviceInfo.h"
#include "tsDektecDevice.h"


//----------------------------------------------------------------------------
// Get information on all Dektec devices in the system.
//----------------------------------------------------------------------------

bool ts::DektecDeviceInfo::GetAllDevices(DektecDeviceInfoVector& info, Report& report)
{
    info.clear();

    // Get all devices.
    DektecDeviceVector devlist;
    if (!DektecDevice::GetAllDevices(devlist, report)) {
        return false;
    }

    // Build the list of descriptions.
    info.resize(devlist.size());
    for (size_t devindex = 0; devindex < devlist.size(); ++devindex) {

        DektecDeviceInfo& inf(info[devindex]);
        const DektecDevice& dev(devlist[devindex]);

        // Device characteristics.
        inf.model = dev.model;
        inf.description = DektecDevice::GetDeviceDescription(dev.desc);

        // Input ports characteristics.
        inf.inputPorts.resize(dev.input.size());
        for (size_t n = 0; n < dev.input.size(); ++n) {
            inf.inputPorts[n].type = DektecDevice::GetInterfaceDescription(dev.input[n]);
            inf.inputPorts[n].description = DektecDevice::GetPortDescription(dev.input[n]);
        }

        // Output ports characteristics.
        inf.outputPorts.resize(dev.output.size());
        for (size_t n = 0; n < dev.output.size(); ++n) {
            inf.outputPorts[n].type = DektecDevice::GetInterfaceDescription(dev.output[n]);
            inf.outputPorts[n].description = DektecDevice::GetPortDescription(dev.output[n]);
        }
    }
    return true;
}
