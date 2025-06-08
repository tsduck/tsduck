//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsDektecVPD.h"
#include "tsMemory.h"


//-----------------------------------------------------------------------------
// Clear one entry from Vital Product Data.
//-----------------------------------------------------------------------------

void ts::DektecVPD::clearOne(char* data)
{
    ts::MemZero(data, ts::DektecVPD::VPD_SIZE);
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
