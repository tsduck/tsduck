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
//!
//!  @file
//!  Declare the class ts::DektecDevice.
//!
//-----------------------------------------------------------------------------

#pragma once
#if !defined(TS_NO_DTAPI) || defined(DOXYGEN)

#include "tsDektec.h"
#include "tsCerrReport.h"
#include "tsEnumeration.h"

namespace ts {

    class DektecDevice;
    //!
    //! Vector of ts::DektecDevice.
    //!
    typedef std::vector<DektecDevice> DektecDeviceVector;
    //!
    //! Vector of Dtapi::DtDeviceDesc.
    //!
    typedef std::vector<Dtapi::DtDeviceDesc> DektecDeviceDescVector;
    //!
    //! Vector of Dtapi::DtHwFuncDesc.
    //!
    typedef std::vector<Dtapi::DtHwFuncDesc> DektecPortDescVector;

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
        UString              model;    //!< Device model name.
        Dtapi::DtDeviceDesc  desc;     //!< Device description, as returned by DTAPI.
        DektecPortDescVector input;    //!< Vector of input ports.
        DektecPortDescVector output;   //!< Vector of output ports.

        //!
        //! Constructor
        //!
        DektecDevice() :
            model(),
            desc(),
            input(),
            output()
        {
        }

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

#endif // TS_NO_DTAPI
