//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    //! @ingroup hardware
    //!
    using DektecDeviceInfoVector = std::vector<DektecDeviceInfo>;

    //!
    //! A simple encapsulation of Dektec device information.
    //! It is normally not possible to access DTAPI and Dektec devices from
    //! TSDuck applications. The binary DTAPI is privately isolated inside
    //! the TSDuck library. This class provides only basic device information.
    //! All access to Dektec devices is normally done using the @c tsdektec
    //! command or the @c dektec plugin.
    //! @ingroup libtsduck hardware
    //!
    class TSDEKTECDLL DektecDeviceInfo
    {
    public:
        //!
        //! Information on an input or output port in a Dektec device.
        //!
        class TSDEKTECDLL PortInfo
        {
        public:
            PortInfo() = default;    //!< Constructor.
            UString type {};         //!< Port type.
            UString description {};  //!< Port description.
        };

        //!
        //! A vector of Dektec port information.
        //!
        using PortInfoVector = std::vector<PortInfo>;

        DektecDeviceInfo() = default;   //!< Constructor.
        UString        model {};        //!< Device model;
        UString        description {};  //!< Device description.
        PortInfoVector inputPorts {};   //!< Description of all input ports on this device.
        PortInfoVector outputPorts {};  //!< Description of all output ports on this device.

        //!
        //! Get information on all Dektec devices in the system.
        //! @param [out] info Returned list of device information.
        //! @param [in,out] report Where to report errors.
        //! @return True in case of success, false on error.
        //!
        static bool GetAllDevices(DektecDeviceInfoVector& info, Report& report = CERR);
    };
}
