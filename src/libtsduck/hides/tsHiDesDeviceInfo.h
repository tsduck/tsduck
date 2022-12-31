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
        int      index;            //!< Adapter index.
        UString  name;             //!< Device name.
        UString  path;             //!< Device path name, can be identical to @a name.
        uint16_t usb_mode;         //!< USB mode, 0x0110 for 1.1, 0x0200 for 2.0.
        uint16_t vendor_id;        //!< Device USB vendor id.
        uint16_t product_id;       //!< Device USB product id.
        uint16_t chip_type;        //!< Chip type, eg 0x9500 for IT9500.
        int      device_type;      //!< 0 = GANYMEDE, 1 = JUPITER, 2 = GEMINI (to be confirmed by tests).
        UString  driver_version;   //!< Driver version string.
        UString  api_version;      //!< API version string.
        UString  link_fw_version;  //!< Link-level firmware version string.
        UString  ofdm_fw_version;  //!< OFDM firmware version string.
        UString  company;          //!< Vendor company.
        UString  hw_info;          //!< Additional hardware information.

        //!
        //! Constructor.
        //!
        HiDesDeviceInfo();

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
    typedef std::list<HiDesDeviceInfo> HiDesDeviceInfoList;
}
