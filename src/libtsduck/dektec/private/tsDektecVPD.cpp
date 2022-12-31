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

#include "tsDektecVPD.h"
#include "tsMemory.h"

#if defined(TS_NO_DTAPI)
TS_LLVM_NOWARNING(missing-variable-declarations)
bool tsDektecVPDIsEmpty = true; // Avoid warning about empty module.
#else


//-----------------------------------------------------------------------------
// Clear one entry from Vital Product Data.
//-----------------------------------------------------------------------------

void ts::DektecVPD::clearOne(char* data)
{
    ts::Zero(data, ts::DektecVPD::VPD_SIZE);
}


//-----------------------------------------------------------------------------
// Read one entry from Vital Product Data. Make sure it is nul-terminated
//-----------------------------------------------------------------------------

void ts::DektecVPD::getOneVPD(Dtapi::DtDevice& dev, const char* keyword, char* data)
{
    clearOne(data);
    const Dtapi::DTAPI_RESULT status = dev.VpdRead(keyword, data);
    if (status == DTAPI_OK) {
        data[ts::DektecVPD::VPD_SIZE - 1] = 0;
    }
    else {
        data[0] = 0;
    }
}


//-----------------------------------------------------------------------------
// Clear content
//-----------------------------------------------------------------------------

void ts::DektecVPD::clear()
{
    clearOne(vpdid);
    clearOne(cl);
    clearOne(ec);
    clearOne(mn);
    clearOne(pd);
    clearOne(pn);
    clearOne(sn);
    clearOne(xt);
    clearOne(bo);
}


//-----------------------------------------------------------------------------
// Get VPD from a device
//-----------------------------------------------------------------------------

void ts::DektecVPD::get(const Dtapi::DtDeviceDesc& dev)
{
    Dtapi::DtDevice dtdev;

    if (dtdev.AttachToSerial(dev.m_Serial) != DTAPI_OK) {
        clear();
    }
    else {
        get(dtdev);
        dtdev.Detach();
    }
}


//-----------------------------------------------------------------------------
// Get VPD from a device
//-----------------------------------------------------------------------------

void ts::DektecVPD::get(Dtapi::DtDevice& dev)
{
    getOneVPD(dev, "VPDID", vpdid);
    getOneVPD(dev, "CL", cl);
    getOneVPD(dev, "EC", ec);
    getOneVPD(dev, "MN", mn);
    getOneVPD(dev, "PD", pd);
    getOneVPD(dev, "PN", pn);
    getOneVPD(dev, "SN", sn);
    getOneVPD(dev, "XT", xt);
    getOneVPD(dev, "BO", bo);
}

#endif // TS_NO_DTAPI
