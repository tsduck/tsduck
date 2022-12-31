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

#include "tsDektecDeviceInfo.h"
#include "tsDektecDevice.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DektecDeviceInfo::DektecDeviceInfo() :
    model(),
    description(),
    inputPorts(),
    outputPorts()
{
}

ts::DektecDeviceInfo::PortInfo::PortInfo() :
    type(),
    description()
{
}


//----------------------------------------------------------------------------
// Get information on all Dektec devices in the system.
//----------------------------------------------------------------------------

bool ts::DektecDeviceInfo::GetAllDevices(DektecDeviceInfoVector& info, Report& report)
{
    info.clear();

#if !defined(TS_NO_DTAPI)

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

#endif // TS_NO_DTAPI

    return true;
}
