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

#include "tsHiDesDeviceInfo.h"


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
// Left part of full listing.
//----------------------------------------------------------------------------

ts::UString ts::HiDesDeviceInfo::title(size_t indent, const UString& title_name) const
{
    return UString(indent, SPACE) + title_name.toJustifiedLeft(17, u'.', false, 1) + SPACE;
}


//----------------------------------------------------------------------------
// Format the structure as a string.
//----------------------------------------------------------------------------

ts::UString ts::HiDesDeviceInfo::toString(bool full, size_t indent) const
{
    UString s;

    if (full) {
        // One line per characteristics, when present.
        if (index >= 0) {
            s += title(index, u"Index") + UString::Format(u"%d\n", {index});
        }
        if (!name.empty()) {
            s += title(index, u"Name") + UString::Format(u"\"%s\"\n", {name});
        }
        if (!path.empty() && path != name) {
            s += title(index, u"Device") + path + u"\n";
        }
        if (usb_mode != 0) {
            s += title(index, u"USB mode") + UString::Format(u"0x%X\n", {usb_mode});
        }
        if (vendor_id != 0) {
            s += title(index, u"Vendor id") + UString::Format(u"0x%X\n", {vendor_id});
        }
        if (product_id != 0) {
            s += title(index, u"Product id") + UString::Format(u"0x%X\n", {product_id});
        }
        if (chip_type != 0) {
            s += title(index, u"Chip type") + UString::Format(u"0x%X\n", {chip_type});
        }
        if (device_type >= 0) {
            // TODO: replace by names if possible.
            s += title(index, u"Device type") + UString::Format(u"%d\n", {device_type});
        }
        if (!driver_version.empty()) {
            s += title(index, u"Driver version") + driver_version + u"\n";
        }
        if (!api_version.empty()) {
            s += title(index, u"API version") + api_version + u"\n";
        }
        if (!link_fw_version.empty()) {
            s += title(index, u"Link firmware") + link_fw_version + u"\n";
        }
        if (!ofdm_fw_version.empty()) {
            s += title(index, u"OFDM firmware") + ofdm_fw_version + u"\n";
        }
        if (!company.empty()) {
            s += title(index, u"Company") + company + u"\n";
        }
        if (!hw_info.empty()) {
            s += title(index, u"Hardware info") + hw_info + u"\n";
        }
    }
    else {
        // Short form.
        s.format(u"%d: \"%s\"", {index, name});
        // Add the device path if different and "not too long" (avoid ugly endless Windows device names).
        if (!path.empty() && path != name && path.length() < 40) {
            s += UString::Format(u" (%s)", {path});
        }
    }

    return s;
}
