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

#include "tsTunerDeviceInfo.h"
#include "tsFileUtils.h"


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

ts::TunerDeviceInfo::TunerDeviceInfo() :
    adapter_number(0),
    frontend_number(0),
    vendor_id(0),
    product_id(0),
    manufacturer(),
    product(),
    version(),
    serial()
{
}


//-----------------------------------------------------------------------------
// Constructor which loads tuner information from adapter and frontend numbers.
//-----------------------------------------------------------------------------

ts::TunerDeviceInfo::TunerDeviceInfo(int adapter, int frontend, Report& report) :
    TunerDeviceInfo(SearchSysdevice(adapter, frontend, report), report)
{
}


//-----------------------------------------------------------------------------
// Load tuner information from a node tree in /sys/devices.
//-----------------------------------------------------------------------------

ts::TunerDeviceInfo::TunerDeviceInfo(const UString& devname, Report& report) :
    TunerDeviceInfo()
{
    // The device name is a directory of the form:
    //   /sys/devices/pci0000:00/0000:00:0c.0/usb1/1-1/dvb/dvb0.frontend0
    // It contains a symbolic link named "device" to somewhere above:
    //   lrwxrwxrwx 1 root root    0 Jun 30 11:28 device -> ../../../1-1
    // The referenced directory contains text files for the various info.

    // Directory where all info files are found.
    const UString infodir(devname + u"/device");

    // Adapter and frontend numbers are extracted from the basename.
    BaseName(devname).scan(u"dvb%d.frontend%d", {&adapter_number, &frontend_number});

    // Vendor and product id are hexadecimal strings.
    UString str_vendor;
    UString str_product;
    LoadText(str_vendor, infodir, u"idVendor", report) && str_vendor.scan(u"%x", {&vendor_id});
    LoadText(str_product, infodir, u"idProduct", report) && str_product.scan(u"%x", {&product_id});

    // Other informations are pure strings.
    LoadText(manufacturer, infodir, u"manufacturer", report);
    LoadText(product, infodir, u"product", report);
    LoadText(version, infodir, u"version", report);
    LoadText(serial, infodir, u"serial", report);
}


//-----------------------------------------------------------------------------
// Load a one-line text file.
//-----------------------------------------------------------------------------

bool ts::TunerDeviceInfo::LoadText(UString& line, const UString& directory, const UString& file, Report& report)
{
    const UString name(directory + PathSeparator + file);
    std::ifstream strm(name.toUTF8().c_str());
    line.clear();
    bool ok = line.getLine(strm);
    strm.close();
    line.trim();
    report.debug(u"%s = \"%s\" (%s)", {name, line, ok ? u"success" : u"failure"});
    return ok;
}


//-----------------------------------------------------------------------------
// Get a full display name for the tuner
//-----------------------------------------------------------------------------

ts::UString ts::TunerDeviceInfo::fullName() const
{
    UString name;
    BuildName(name, u"", manufacturer);
    if (product != manufacturer) {
        BuildName(name, u"", product);
    }
    BuildName(name, u"", version);
    BuildName(name, u"SN:", serial);
    return name;
}


//-----------------------------------------------------------------------------
// Build a name component by component.
//-----------------------------------------------------------------------------

void ts::TunerDeviceInfo::BuildName(UString& name, const UString& prefix, const UString& value)
{
    if (!value.empty()) {
        if (!name.empty()) {
            name.append(u" ");
        }
        name.append(prefix);
        name.append(value);
    }
}


//-----------------------------------------------------------------------------
// Search the /sys/devices for a given DVB adapter.
//-----------------------------------------------------------------------------

ts::UString ts::TunerDeviceInfo::SearchSysdevice(int adapter, int frontend, Report& report)
{
    UString pattern;
    pattern.format(u"dvb%d.frontend%d", {adapter, frontend});

    UStringList files;
    SearchFiles(files, u"/sys/devices", pattern, 16);

    return files.empty() ? pattern : files.front();
}


//-----------------------------------------------------------------------------
// Load the description of all tuner devices.
//-----------------------------------------------------------------------------

void ts::TunerDeviceInfo::LoadAll(std::vector<TunerDeviceInfo>& devices, Report& report)
{
    UStringList files;
    SearchFiles(files, u"/sys/devices", u"dvb*.frontend*", 16);
    devices.clear();
    for (const auto& it : files) {
        devices.push_back(TunerDeviceInfo(it, report));
    }
}


//-----------------------------------------------------------------------------
// Find all files matching a pattern under a directory. Skip known dead ends.
//-----------------------------------------------------------------------------

void ts::TunerDeviceInfo::SearchFiles(UStringList& files, const UString& root, const UString& pattern, size_t levels)
{
    // Append all files directly matching the wildcard in root directory.
    ExpandWildcardAndAppend(files, root + PathSeparator + pattern);

    // If the maximum number of recursion levels is not reached, recurse in all subdirectories.
    if (levels > 0) {
        // Search all files under root.
        UStringList locals;
        ExpandWildcard(locals, root + PathSeparator + u"*");
        for (const auto& loc : locals) {
            // Keep only directories which are not symbolic links (could loop).
            if (IsDirectory(loc) && !IsSymbolicLink(loc)) {
                // Filter out names which are known to be dead-ends with many files under.
                const UString name(BaseName(loc));
                if (name != u"breakpoint" &&
                    name != u"tracepoint" &&
                    name != u"kprobe" &&
                    name != u"msr" &&
                    name != u"power" &&
                    name != u"software" &&
                    name != u"platform" &&
                    name != u"system" &&
                    name != u"uprobe" &&
                    name != u"virtual" &&
                    !name.startWith(u"LNXSYS"))
                {
                    // We can recurse.
                    SearchFiles(files, loc, pattern, levels - 1);
                }
            }
        }
    }
}
