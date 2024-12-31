//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Information about a HiDes modulator device.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Information about a HiDes modulator device.
    //! Some public fields are available on Windows or Linux only.
    //! @ingroup hardware
    //!
    class TSDUCKDLL HiDesDeviceInfo
    {
    public:
        int      index = -1;          //!< Adapter index.
        UString  name {};             //!< Device name.
        UString  path {};             //!< Device path name, can be identical to @a name.
        uint16_t usb_mode = 0;        //!< USB mode, 0x0110 for 1.1, 0x0200 for 2.0.
        uint16_t vendor_id = 0;       //!< Device USB vendor id.
        uint16_t product_id = 0;      //!< Device USB product id.
        uint16_t chip_type = 0;       //!< Chip type, eg 0x9500 for IT9500.
        int      device_type = -1;    //!< 0 = GANYMEDE, 1 = JUPITER, 2 = GEMINI (to be confirmed by tests).
        UString  driver_version {};   //!< Driver version string.
        UString  api_version {};      //!< API version string.
        UString  link_fw_version {};  //!< Link-level firmware version string.
        UString  ofdm_fw_version {};  //!< OFDM firmware version string.
        UString  company {};          //!< Vendor company.
        UString  hw_info {};          //!< Additional hardware information.

        //!
        //! Constructor.
        //!
        HiDesDeviceInfo() = default;

        //!
        //! Clear all information.
        //!
        void clear();

        //!
        //! Format the structure as a string.
        //! @param [in] full If true, display all characteristics in multi-line format.
        //! @param [in] indent Margin width (when @a full is true).
        //! @return A multi-line description of this object.
        //!
        UString toString(bool full = false, size_t indent = 0) const;

    private:
        // Left part of full listing.
        UString title(size_t indent, const UString& title_name) const;
    };

    //!
    //! A list of HiDes device information.
    //!
    using HiDesDeviceInfoList = std::list<HiDesDeviceInfo>;
}
