//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHiDesDeviceInfo.h"
#include "tsFeatures.h"


//----------------------------------------------------------------------------
// Register for options --version and --support
// (no specific version since there is no specific library).
//----------------------------------------------------------------------------

#if defined(TS_NO_HIDES)
    #define SUPPORT ts::Features::UNSUPPORTED
#else
    #define SUPPORT ts::Features::SUPPORTED
#endif

TS_REGISTER_FEATURE(u"hides", u"HiDes", SUPPORT, nullptr);


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
            s += title(index, u"Index") + UString::Format(u"%d\n", index);
        }
        if (!name.empty()) {
            s += title(index, u"Name") + UString::Format(u"\"%s\"\n", name);
        }
        if (!path.empty() && path != name) {
            s += title(index, u"Device") + path + u"\n";
        }
        if (usb_mode != 0) {
            s += title(index, u"USB mode") + UString::Format(u"0x%X\n", usb_mode);
        }
        if (vendor_id != 0) {
            s += title(index, u"Vendor id") + UString::Format(u"0x%X\n", vendor_id);
        }
        if (product_id != 0) {
            s += title(index, u"Product id") + UString::Format(u"0x%X\n", product_id);
        }
        if (chip_type != 0) {
            s += title(index, u"Chip type") + UString::Format(u"0x%X\n", chip_type);
        }
        if (device_type >= 0) {
            // TODO: replace by names if possible.
            s += title(index, u"Device type") + UString::Format(u"%d\n", device_type);
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
        s.format(u"%d: \"%s\"", index, name);
        // Add the device path if different and "not too long" (avoid ugly endless Windows device names).
        if (!path.empty() && path != name && path.length() < 40) {
            s += UString::Format(u" (%s)", path);
        }
    }

    return s;
}
