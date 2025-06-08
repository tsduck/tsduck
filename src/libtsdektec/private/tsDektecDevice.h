//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the class ts::DektecDevice.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsDektec.h"
#include "tsCerrReport.h"
#include "tsNames.h"

namespace ts {

    class DektecDevice;
    //!
    //! Vector of ts::DektecDevice.
    //!
    using DektecDeviceVector = std::vector<DektecDevice>;
    //!
    //! Vector of Dtapi::DtDeviceDesc.
    //!
    using DektecDeviceDescVector = std::vector<Dtapi::DtDeviceDesc>;
    //!
    //! Vector of Dtapi::DtHwFuncDesc.
    //!
    using DektecPortDescVector = std::vector<Dtapi::DtHwFuncDesc>;

    //!
    //! Get the error message corresponding to a DTAPI error code
    //! @param [in] code DTAPI error code.
    //! @return An error message.
    //!
    UString DektecStrError(Dtapi::DTAPI_RESULT code);

    //!
    //! Description of a Dektec device.
    //!
    class DektecDevice
    {
    public:
        UString              model {};    //!< Device model name.
        Dtapi::DtDeviceDesc  desc {};     //!< Device description, as returned by DTAPI.
        DektecPortDescVector input {};    //!< Vector of input ports.
        DektecPortDescVector output {};   //!< Vector of output ports.

        //!
        //! Constructor
        //!
        DektecDevice() = default;

        //!
        //! Load the description of a Dektec device into this object.
        //! @param [in,out] dev_index Index of the Dektec device to load.
        //! If negative, search for the first device with the input or output capability
        //! as specified by @a is_input and update @a dev_index.
        //! @param [in,out] chan_index Index of the channel to load.
        //! If negative, search for the first channel with the input or output capability
        //! as specified by @a is_input and update @a chan_index.
        //! @param [in] is_input If true, make sure the channel has input capability.
        //! If false, make sure it has output capability.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool getDevice(int& dev_index, int& chan_index, bool is_input, Report& report = CERR);

        // Get the list of all Dektec devices in the system.
        // Return true in case of success, false on error.
        // Report error messages through report
        static bool GetAllDevices(DektecDeviceVector&, Report& = CERR);

        // Get the list of all Dektec ports in the system.
        // If is_input and/or is_output are true, return only the ports which are currently in the right direction.
        // If is_bidirectional is true, also report bidirectional ASI ports which are currently not in the right direction.
        // TS-over-IP ports are always considered as both input and output ports. Remote network devices (DTE-xxxx) are not returned.
        // Return true in case of success, false on error. Report error messages through report
        static bool GetAllPorts(DektecPortDescVector&, bool is_input, bool is_output, bool is_bidirectional, Report& = CERR);

        // Get a string description of a Dektec device or port.
        static UString GetDeviceDescription(const Dtapi::DtDeviceDesc&);
        static UString GetPortDescription(const Dtapi::DtHwFuncDesc&);

        // Get a string description of a Dektec interface type
        static UString GetInterfaceDescription(const Dtapi::DtHwFuncDesc&);

        // Get a string description of Dektec capabilities.
        static UString DtCapsToString(const Dtapi::DtCaps&);

        // Display various Dektec data structure for debug
        static void ReportDvbT2Pars(const Dtapi::DtDvbT2Pars&, Report& = CERR, int severity = Severity::Info, const UString& margin = UString());
        static void ReportDvbT2PlpPars(const Dtapi::DtDvbT2PlpPars&, Report& = CERR, int severity = Severity::Info, const UString& margin = UString());
        static void ReportDvbT2ParamInfo(const Dtapi::DtDvbT2ParamInfo&, Report& = CERR, int severity = Severity::Info, const UString& margin = UString());
        static void ReportIpPars(const Dtapi::DtIpPars2&, Report& = CERR, int severity = Severity::Info, const UString& margin = UString());

    private:
        // Append a name to a string if a condition is true.
        static void OneCap(UString& str, bool condition, const UString& name);
        static void OneCap(UString& str, Dtapi::DtCaps cap, const UString& name);
    };
}
