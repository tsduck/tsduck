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
//!  A simple encapsulation of Dektec device information.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsCerrReport.h"

namespace ts {

    class DektecDeviceInfo;

    //!
    //! A vector of Dektec device information.
    //!
    typedef std::vector<DektecDeviceInfo> DektecDeviceInfoVector;

    //!
    //! A simple encapsulation of Dektec device information.
    //! It is normally not possible to access DTAPI and Dektec devices from
    //! TSDuck applications. The binary DTAPI is privately isolated inside
    //! the TSDuck library. This class provides only basic device information.
    //! All access to Dektec devices is normally done using the @c tsdektec
    //! command or the @c dektec plugin.
    //! @ingroup hardware
    //!
    class TSDUCKDLL DektecDeviceInfo
    {
    public:
        //!
        //! Information on an input or output port in a Dektec device.
        //!
        class TSDUCKDLL PortInfo
        {
        public:
            PortInfo();           //!< Constructor.
            UString type;         //!< Port type.
            UString description;  //!< Port description.
        };

        //!
        //! A vector of Dektec port information.
        //!
        typedef std::vector<PortInfo> PortInfoVector;

        DektecDeviceInfo();          //!< Constructor.
        UString        model;        //!< Device model;
        UString        description;  //!< Device description.
        PortInfoVector inputPorts;   //!< Description of all input ports on this device.
        PortInfoVector outputPorts;  //!< Description of all output ports on this device.

        //!
        //! Get information on all Dektec devices in the system.
        //! @param [out] info Returned list of device information.
        //! @param [in,out] report Where to report errors.
        //! @return True in case of success, false on error.
        //!
        static bool GetAllDevices(DektecDeviceInfoVector& info, Report& report = CERR);
    };
}
