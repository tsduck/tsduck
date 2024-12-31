//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Information on Linux DVB tuner device.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Information on Linux DVB tuner device.
    //! @ingroup unix
    //!
    class TunerDeviceInfo
    {
    public:
        int      adapter_number = 0;   //!< DVB adapter number.
        int      frontend_number = 0;  //!< DVB frontend number.
        uint16_t vendor_id = 0;        //!< Vendor id (USB, PCI).
        uint16_t product_id = 0;       //!< Product id (USB, PCI).
        UString  manufacturer {};      //!< Manufacturer name.
        UString  product {};           //!< Product name.
        UString  version {};           //!< Product version string.
        UString  serial {};            //!< Device serial number.

        //!
        //! Default constructor.
        //!
        TunerDeviceInfo() = default;

        //!
        //! Constructor which loads tuner information from adapter and frontend numbers.
        //! @param [in] adapter Adapter number.
        //! @param [in] frontend Frontend number.
        //! @param [in,out] report Where to report errors.
        //!
        TunerDeviceInfo(int adapter, int frontend, Report& report);

        //!
        //! Constructor which loads tuner information from a node tree in /sys/devices.
        //! @param [in] devname Device name, for instance /sys/devices/pci0000:00/0000:00:0c.0/usb1/1-1/dvb/dvb0.frontend0
        //! @param [in,out] report Where to report errors.
        //!
        TunerDeviceInfo(const UString& devname, Report& report);

        //!
        //! Get a full display name for the tuner
        //! @return A full display name.
        //!
        UString fullName() const;

        //!
        //! Load the description of all tuner devices.
        //! @param [out] devices Description of all tuner devices.
        //! @param [in,out] report Where to report errors.
        //!
        static void LoadAll(std::vector<TunerDeviceInfo>& devices, Report& report);

    private:
        // Load a one-line text file.
        static bool LoadText(UString& line, const UString& directory, const UString& file, Report& report);

        // Build a name component by component.
        static void BuildName(UString& name, const UString& prefix, const UString& value);

        // Search the /sys/devices for a given DVB adapter.
        static UString SearchSysdevice(int adapter, int frontend, Report& report);

        // Find all files matching a pattern under a directory. Skip known dead ends.
        static void SearchFiles(UStringList& files, const UString& root, const UString& pattern, size_t levels);
    };
}
