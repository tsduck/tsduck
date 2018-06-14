//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsHiDesDeviceInfo.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::HiDesDeviceInfo::HiDesDeviceInfo() :
    index(-1),
    name(),
    path(),
    usb_mode(0),
    vendor_id(0),
    product_id(0),
    chip_type(0),
    device_type(-1),
    driver_version(),
    api_version(),
    link_fw_version(),
    ofdm_fw_version(),
    company(),
    hw_info()
{
}


//----------------------------------------------------------------------------
// Clear all information.
//----------------------------------------------------------------------------

void ts::HiDesDeviceInfo::clear()
{
    index = -1;
    name.clear();
    path.clear();
    usb_mode = 0;
    vendor_id = 0;
    product_id = 0;
    chip_type = 0;
    device_type = -1;
    driver_version.clear();
    api_version.clear();
    link_fw_version.clear();
    ofdm_fw_version.clear();
    company.clear();
    hw_info.clear();
}

//----------------------------------------------------------------------------
// Format the structure as a string.
//----------------------------------------------------------------------------

ts::UString ts::HiDesDeviceInfo::toString(size_t indent) const
{
    UString s;

    if (index >= 0) {
        s += UString::Format(u"%*sIndex: %d\n", {indent, u"", index});
    }
    if (!name.empty()) {
        s += UString::Format(u"%*sName: \"%s\"\n", {indent, u"", name});
    }
    if (!path.empty() && path != name) {
        s += UString::Format(u"%*sDevice: %s\n", {indent, u"", path});
    }
    if (usb_mode != 0) {
        s += UString::Format(u"%*sUSB mode: 0x%X\n", {indent, u"", usb_mode});
    }
    if (vendor_id != 0) {
        s += UString::Format(u"%*sVendor id: 0x%X\n", {indent, u"", vendor_id});
    }
    if (product_id != 0) {
        s += UString::Format(u"%*sProduct id: 0x%X\n", {indent, u"", product_id});
    }
    if (chip_type != 0) {
        s += UString::Format(u"%*sChip type: 0x%X\n", {indent, u"", chip_type});
    }
    if (device_type >= 0) {
        // TODO: replace by names if possible.
        s += UString::Format(u"%*sDevice type: %d\n", {indent, u"", device_type});
    }
    if (!driver_version.empty()) {
        s += UString::Format(u"%*sDriver version: %s\n", {indent, u"", driver_version});
    }
    if (!api_version.empty()) {
        s += UString::Format(u"%*sAPI version: %s\n", {indent, u"", api_version});
    }
    if (!link_fw_version.empty()) {
        s += UString::Format(u"%*sLink-level firmware version: %s\n", {indent, u"", link_fw_version});
    }
    if (!ofdm_fw_version.empty()) {
        s += UString::Format(u"%*sOFDM firmware version: %s\n", {indent, u"", ofdm_fw_version});
    }
    if (!company.empty()) {
        s += UString::Format(u"%*sCompany: %s\n", {indent, u"", company});
    }
    if (!hw_info.empty()) {
        s += UString::Format(u"%*sHardware info: %s\n", {indent, u"", hw_info});
    }

    return s;
}
